#include "Job.h"

#include <algorithm>
#include <cassert>
#include <deque>

#include "../Core/WorkerThread.h"
#include "../Utility/Logger.h"

namespace wcbot {

// =============================================================================
// Job
// =============================================================================

Job::Job() : ErrCode(0), Parent(nullptr), CancelPending(false) {}

Job::~Job() {
  // The framework owns lifetime, but defensively detach from any leftover
  // parent/children. Reaching here with a non-empty `Children` vector means
  // the framework's cancellation path missed a child — which is a bug.
  assert(Children.empty() && "Job destroyed while children are still alive");
  if (Parent != nullptr) {
    auto &Vec = Parent->Children;
    Vec.erase(std::remove(Vec.begin(), Vec.end(), this), Vec.end());
    Parent = nullptr;
  }
}

namespace job_impl {

// Per-thread pending activation queue. `InvokeChild` and the `kDone` ->
// "drive parent" path both append entries here; the top-level `Driver::Run`
// drains the queue iteratively. This decouples "spawn child / notify parent"
// from synchronous re-entrancy and turns what used to be a deep recursion
// into a flat loop.
//
// `DeleteAfterStep`, when non-null, names a Job that must be `delete`d
// AFTER `J->OnStep(Trigger=DeleteAfterStep)` returns. This lets the parent
// observe the just-finished child (read result fields) while the child is
// still alive, and lets the framework remain the sole owner of lifetime.
struct Pending {
  Job *J;
  Job *Trigger;
  Job *DeleteAfterStep;
};
struct PendingQueue {
  std::deque<Pending> Q;
  bool Active = false;  // true while Driver::Run is draining
};
static thread_local PendingQueue g_Pending;

void Driver::Schedule(Job *J, Job *Trigger) {
  g_Pending.Q.push_back({J, Trigger, /*DeleteAfterStep=*/nullptr});
}

// Drop every pending entry that involves `Victim` (as J, Trigger, or
// DeleteAfterStep). Called when a Job is cancelled / deleted out of band so
// the outer Run loop never reaches a stale pointer.
void Driver::PurgePending(Job *Victim) {
  auto &Q = g_Pending.Q;
  Q.erase(std::remove_if(Q.begin(), Q.end(),
                         [Victim](const Pending &P) {
                           return P.J == Victim || P.Trigger == Victim ||
                                  P.DeleteAfterStep == Victim;
                         }),
          Q.end());
}

}  // namespace job_impl

void Job::InvokeChild(Job *Child) {
  if (Child == nullptr) {
    return;
  }
  Child->Parent = this;
  Children.push_back(Child);
  // Activation is deferred to the top-level Driver loop so that synchronous
  // child completion does NOT recursively re-enter our OnStep before we have
  // had a chance to advance our own state and return.
  job_impl::Driver::Schedule(Child, Child);
}

void Job::Sleep(int Millisecond) {
  worker_impl::g_ThisThread->JoinSleepQueue(this, Millisecond);
}

void Job::Cancel() { CancelPending = true; }

void Job::CancelChildrenAndSelf() {
  while (!Children.empty()) {
    Job *Child = Children.back();
    Children.pop_back();
    Child->Parent = nullptr;
    Child->CancelChildrenAndSelf();
  }
  this->OnCancel();
  // Make sure no stale pending entries reference us before delete.
  job_impl::Driver::PurgePending(this);
  delete this;
}

// =============================================================================
// IOJob
// =============================================================================

void IOJob::ArmTimeout(int Millisecond) {
  worker_impl::g_ThisThread->JoinDelayQueue(this, Millisecond);
}

// =============================================================================
// job_impl::Driver
// =============================================================================

namespace job_impl {

// Drive a single job until it yields (kWaiting / kDone). Any spawned
// children or "notify parent" continuations are pushed onto the per-thread
// pending queue and processed by the outer Run loop iteratively.
//
// `DeleteAfterStep`, if non-null, is a Job whose lifetime is tied to the
// FIRST OnStep call below: the parent observes the child via Trigger and
// then we delete the child as soon as the OnStep returns. This is the only
// place in the framework that frees a Job that finished by returning kDone.
void Driver::DriveOne(Job *J, Job *Trigger, Job *DeleteAfterStep) {
  while (true) {
    if (J->CancelPending) {
      Job *Parent = J->Parent;
      if (Parent != nullptr) {
        auto &Vec = Parent->Children;
        Vec.erase(std::remove(Vec.begin(), Vec.end(), J), Vec.end());
        J->Parent = nullptr;
      }
      J->CancelChildrenAndSelf();
      // Even on the cancellation fast-path we still owe the caller a delete
      // of the just-finished child if one was attached.
      delete DeleteAfterStep;
      return;
    }

    Job::Step S = J->OnStep(Trigger);
    Trigger = nullptr;
    // The just-finished child (if any) is no longer needed after the first
    // OnStep — its result was either copied out or ignored by the parent.
    if (DeleteAfterStep != nullptr) {
      delete DeleteAfterStep;
      DeleteAfterStep = nullptr;
    }

    if (S == Job::Step::kContinue) {
      continue;
    }
    if (S == Job::Step::kWaiting) {
      return;
    }
    // kDone
    Job *Parent = J->Parent;
    if (Parent != nullptr) {
      auto &Vec = Parent->Children;
      Vec.erase(std::remove(Vec.begin(), Vec.end(), J), Vec.end());
      J->Parent = nullptr;
    }
    while (!J->Children.empty()) {
      Job *Child = J->Children.back();
      J->Children.pop_back();
      Child->Parent = nullptr;
      Child->CancelChildrenAndSelf();
    }
    if (Parent != nullptr) {
      // Parent observes this finished child through Trigger=J, then we
      // delete J once that OnStep returns. Both halves are scheduled on the
      // same pending entry to keep ordering and ownership tight.
      g_Pending.Q.push_back({Parent, J, /*DeleteAfterStep=*/J});
    } else {
      // No parent to observe us — free immediately. Sanitize the queue to
      // make sure no straggling entries reference J as Trigger before delete.
      Driver::PurgePending(J);
      delete J;
    }
    return;
  }
}

void Driver::Run(Job *J, Job *Trigger) {
  // If we're already inside a running drain, just append and return — the
  // outer loop will pick this entry up.
  if (g_Pending.Active) {
    g_Pending.Q.push_back({J, Trigger, /*DeleteAfterStep=*/nullptr});
    return;
  }
  g_Pending.Active = true;
  // Seed the queue with the initial entry, then drain until empty.
  g_Pending.Q.push_back({J, Trigger, /*DeleteAfterStep=*/nullptr});
  while (!g_Pending.Q.empty()) {
    Pending P = g_Pending.Q.front();
    g_Pending.Q.pop_front();
    DriveOne(P.J, P.Trigger, P.DeleteAfterStep);
  }
  g_Pending.Active = false;
}

void Driver::Cancel(Job *J) {
  if (J == nullptr) {
    return;
  }
  Job *Parent = J->Parent;
  if (Parent != nullptr) {
    auto &Vec = Parent->Children;
    Vec.erase(std::remove(Vec.begin(), Vec.end(), J), Vec.end());
    J->Parent = nullptr;
  }
  J->CancelChildrenAndSelf();
}

}  // namespace job_impl

}  // namespace wcbot