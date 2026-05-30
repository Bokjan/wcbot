// Job v2 protocol tests.
//
// We deliberately exercise only the subset of the framework that does NOT
// depend on `worker_impl::g_ThisThread` (namely: no Sleep, no ArmTimeout,
// no SendData). That subset still covers the core contract:
//   * OnStep advancement: kContinue / kWaiting / kDone
//   * Lifetime: framework deletes Job exactly once on kDone
//   * Parent observes finished child via OnStep(Trigger=child) before delete
//   * InvokeChild establishes parent/child linkage
//   * Cancel / parent kDone propagates OnCancel to outstanding children
//   * Children of cancelled jobs also receive OnCancel (recursive)
//
// All Jobs allocated via `new` are owned by the framework — never delete
// them manually in the test; they self-destruct through Driver::Run.

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "Job/Job.h"

using wcbot::Job;
using wcbot::job_impl::Driver;

namespace {

// Per-test global counters; reset in TestEvents::Reset().
struct TestEvents {
  int OnStepCalls = 0;
  int OnCancelCalls = 0;
  int Destructions = 0;
  std::vector<std::string> Trace;
  void Reset() {
    OnStepCalls = 0;
    OnCancelCalls = 0;
    Destructions = 0;
    Trace.clear();
  }
};
TestEvents g_Events;

// A linear job with `N` kContinue steps before kDone.
class LinearJob : public Job {
 public:
  explicit LinearJob(int N) : Total(N), Current(0) {}
  ~LinearJob() override { ++g_Events.Destructions; }
  Step OnStep(Job * /*Trigger*/) override {
    ++g_Events.OnStepCalls;
    if (Current >= Total) {
      return Step::kDone;
    }
    ++Current;
    return Step::kContinue;
  }

 private:
  int Total;
  int Current;
};

// A leaf job whose only purpose is to be invoked as a child and produce a
// result that the parent can read in OnStep(Trigger=this_finished_child).
class ChildResultJob : public Job {
 public:
  std::string Result;
  bool DidCancel = false;
  ~ChildResultJob() override { ++g_Events.Destructions; }
  Step OnStep(Job *Trigger) override {
    ++g_Events.OnStepCalls;
    // First entry is self-driven (Trigger == this); produce the result and
    // exit immediately.
    if (Trigger == this) {
      Result = "the answer is 42";
      return Step::kDone;
    }
    return Step::kDone;
  }
  void OnCancel() override {
    ++g_Events.OnCancelCalls;
    DidCancel = true;
  }
};

// A parent that spawns one ChildResultJob, captures its result, then exits.
class ParentJob : public Job {
 public:
  std::string CapturedResult;
  Step OnStep(Job *Trigger) override {
    ++g_Events.OnStepCalls;
    switch (State) {
      case 0:
        // Update state BEFORE InvokeChild: InvokeChild drives the child
        // synchronously and, if the child finishes immediately, the framework
        // re-enters our OnStep with Trigger==child on the same call stack.
        State = 1;
        InvokeChild(new ChildResultJob());
        return Step::kWaiting;
      case 1: {
        auto *Child = AsJob<ChildResultJob>(Trigger);
        if (Child != nullptr) {
          CapturedResult = Child->Result;
        }
        return Step::kDone;
      }
    }
    return Step::kDone;
  }
  ~ParentJob() override { ++g_Events.Destructions; }

 private:
  int State = 0;
};

// A child that hands control back to the parent without finishing — used to
// drive cancellation tests by having the parent return kDone while the child
// is still in flight.
class StuckChildJob : public Job {
 public:
  ~StuckChildJob() override { ++g_Events.Destructions; }
  Step OnStep(Job *Trigger) override {
    ++g_Events.OnStepCalls;
    // Always remain in kWaiting after the first activation.
    if (Trigger == this) {
      return Step::kWaiting;
    }
    return Step::kWaiting;
  }
  void OnCancel() override {
    ++g_Events.OnCancelCalls;
    g_Events.Trace.emplace_back("StuckChild::OnCancel");
  }
};

// Parent that spawns a StuckChildJob, then immediately returns kDone in the
// next step — leaving the child outstanding so the framework must cancel it.
class AbandoningParentJob : public Job {
 public:
  ~AbandoningParentJob() override { ++g_Events.Destructions; }
  Step OnStep(Job *Trigger) override {
    ++g_Events.OnStepCalls;
    switch (State) {
      case 0:
        InvokeChild(new StuckChildJob());
        // Child is now sitting in kWaiting. Returning kDone here forces the
        // framework to cancel the still-running child before deleting us.
        return Step::kDone;
    }
    return Step::kDone;
  }

 private:
  int State = 0;
};

// Two-level hierarchy: GrandparentJob spawns ParentForHierarchyJob which
// spawns StuckChildJob. We then cancel the grandparent externally, expecting
// OnCancel to fan out to both descendants.
class ParentForHierarchyJob : public Job {
 public:
  ~ParentForHierarchyJob() override { ++g_Events.Destructions; }
  Step OnStep(Job *Trigger) override {
    ++g_Events.OnStepCalls;
    if (Trigger == this) {
      InvokeChild(new StuckChildJob());
      return Step::kWaiting;  // wait for grandchild that never finishes
    }
    return Step::kWaiting;
  }
  void OnCancel() override {
    ++g_Events.OnCancelCalls;
    g_Events.Trace.emplace_back("ParentForHierarchy::OnCancel");
  }
};

class GrandparentJob : public Job {
 public:
  ~GrandparentJob() override { ++g_Events.Destructions; }
  Step OnStep(Job *Trigger) override {
    ++g_Events.OnStepCalls;
    if (Trigger == this) {
      InvokeChild(new ParentForHierarchyJob());
      // Stay alive — let the descendants reach their kWaiting state, then
      // the test calls Driver::Cancel(this) to exercise recursive cancel.
      return Step::kWaiting;
    }
    return Step::kWaiting;
  }
  void OnCancel() override {
    ++g_Events.OnCancelCalls;
    g_Events.Trace.emplace_back("Grandparent::OnCancel");
  }
};

}  // namespace

// -----------------------------------------------------------------------------
// Lifetime & step transitions
// -----------------------------------------------------------------------------

TEST(JobV2, KContinueAdvancesUntilDone) {
  g_Events.Reset();
  auto *J = new LinearJob(3);
  Driver::Run(J, J);
  // 1 self-trigger entry + 3 kContinue advances + 1 kDone observation = 5 calls.
  // Implementation re-enters OnStep after each kContinue, so we expect Total+1
  // calls (3 advances + 1 final kDone return).
  EXPECT_EQ(g_Events.OnStepCalls, 4);
  EXPECT_EQ(g_Events.Destructions, 1);
}

TEST(JobV2, ParentReadsChildResultBeforeChildDelete) {
  g_Events.Reset();
  auto *P = new ParentJob();
  Driver::Run(P, P);
  // Both parent and child should have been deleted.
  EXPECT_EQ(g_Events.Destructions, 2);
  // Child's OnCancel should NOT have been called — it finished naturally.
  EXPECT_EQ(g_Events.OnCancelCalls, 0);
}

TEST(JobV2, ParentCapturesChildResult) {
  g_Events.Reset();
  // Allocate one we can observe via a stack-side handle. We can't keep the
  // pointer after Driver::Run since the framework deletes it; instead we use
  // a small thread-local trick: capture into a static.
  struct Capture {
    static std::string &Slot() {
      static std::string s;
      return s;
    }
  };
  class CapturingParent : public Job {
   public:
    Step OnStep(Job *Trigger) override {
      switch (State) {
        case 0:
          State = 1;  // advance BEFORE InvokeChild (synchronous re-entry safe)
          InvokeChild(new ChildResultJob());
          return Step::kWaiting;
        case 1:
          if (auto *C = AsJob<ChildResultJob>(Trigger)) {
            Capture::Slot() = C->Result;
          }
          return Step::kDone;
      }
      return Step::kDone;
    }

   private:
    int State = 0;
  };
  Capture::Slot().clear();
  auto *P = new CapturingParent();
  Driver::Run(P, P);
  EXPECT_EQ(Capture::Slot(), "the answer is 42");
}

// -----------------------------------------------------------------------------
// Cancellation
// -----------------------------------------------------------------------------

TEST(JobV2, ParentDoneCancelsOutstandingChild) {
  g_Events.Reset();
  auto *P = new AbandoningParentJob();
  Driver::Run(P, P);
  // Both parent and the orphaned child must be reclaimed.
  EXPECT_EQ(g_Events.Destructions, 2);
  // The stuck child must receive OnCancel before deletion.
  EXPECT_EQ(g_Events.OnCancelCalls, 1);
  ASSERT_FALSE(g_Events.Trace.empty());
  EXPECT_EQ(g_Events.Trace[0], "StuckChild::OnCancel");
}

TEST(JobV2, CancellationIsRecursive) {
  g_Events.Reset();
  auto *G = new GrandparentJob();
  Driver::Run(G, G);
  // After Run returns, the whole tree (G → PFH → StuckChild) is in kWaiting,
  // nothing was deleted yet.
  EXPECT_EQ(g_Events.Destructions, 0);
  EXPECT_EQ(g_Events.OnCancelCalls, 0);

  // Now cancel the grandparent externally; OnCancel must reach every
  // descendant before each is deleted, with deepest-first ordering.
  Driver::Cancel(G);
  EXPECT_EQ(g_Events.Destructions, 3);
  EXPECT_EQ(g_Events.OnCancelCalls, 3);
  // The framework cancels the deepest descendants first.
  ASSERT_EQ(g_Events.Trace.size(), 3u);
  EXPECT_EQ(g_Events.Trace[0], "StuckChild::OnCancel");
  EXPECT_EQ(g_Events.Trace[1], "ParentForHierarchy::OnCancel");
  EXPECT_EQ(g_Events.Trace[2], "Grandparent::OnCancel");
}

TEST(JobV2, ExternalCancelStopsJob) {
  g_Events.Reset();
  // Drive a never-finishing job, then cancel via `Driver::Cancel`.
  auto *J = new StuckChildJob();
  Driver::Run(J, J);  // settles into kWaiting
  EXPECT_EQ(g_Events.Destructions, 0);  // still alive
  Driver::Cancel(J);
  EXPECT_EQ(g_Events.Destructions, 1);
  EXPECT_EQ(g_Events.OnCancelCalls, 1);
}
