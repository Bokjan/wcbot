#include "Job.h"

#include <algorithm>
#include <cassert>

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

void Job::InvokeChild(Job *Child) {
  if (Child == nullptr) {
    return;
  }
  Child->Parent = this;
  Children.push_back(Child);
  // The child's first activation observes `Trigger = self` so it can perform
  // any setup that needs to happen on the worker thread context.
  job_impl::Driver::Run(Child, Child);
}

void Job::Sleep(int Millisecond) {
  worker_impl::g_ThisThread->JoinSleepQueue(this, Millisecond);
}

void Job::Cancel() { CancelPending = true; }

void Job::CancelChildrenAndSelf() {
  // Walk a snapshot — `OnCancel` implementations are not allowed to mutate
  // the children vector (it's not part of the public API anyway), but the
  // framework's recursive cleanup will pop entries as it goes.
  while (!Children.empty()) {
    Job *Child = Children.back();
    Children.pop_back();
    Child->Parent = nullptr;            // detach to avoid the dtor reach-back
    Child->CancelChildrenAndSelf();
  }
  this->OnCancel();
  delete this;
}

// =============================================================================
// IOJob
// =============================================================================

void IOJob::ArmTimeout(int Millisecond) {
  worker_impl::g_ThisThread->JoinDelayQueue(this, Millisecond);
}

// =============================================================================
// job_impl::Driver — the only entity that drives Jobs forward
// =============================================================================

namespace job_impl {

void Driver::Run(Job *J, Job *Trigger) {
  while (true) {
    // Honor a Cancel request that may have been raised before re-entry.
    if (J->CancelPending) {
      Job *Parent = J->Parent;
      if (Parent != nullptr) {
        auto &Vec = Parent->Children;
        Vec.erase(std::remove(Vec.begin(), Vec.end(), J), Vec.end());
        J->Parent = nullptr;
      }
      J->CancelChildrenAndSelf();
      // Cancelled jobs do NOT notify the parent — by definition the parent
      // either initiated the cancel (no callback needed) or finished already.
      return;
    }

    Job::Step S = J->OnStep(Trigger);
    Trigger = nullptr;

    if (S == Job::Step::kContinue) {
      continue;
    }
    if (S == Job::Step::kWaiting) {
      return;
    }
    // S == kDone
    Job *Parent = J->Parent;
    if (Parent != nullptr) {
      auto &Vec = Parent->Children;
      Vec.erase(std::remove(Vec.begin(), Vec.end(), J), Vec.end());
      J->Parent = nullptr;
    }
    // Cancel any still-running children (defensive — well-behaved jobs
    // should not have outstanding children when returning kDone).
    while (!J->Children.empty()) {
      Job *Child = J->Children.back();
      J->Children.pop_back();
      Child->Parent = nullptr;
      Child->CancelChildrenAndSelf();
    }
    Job *DeadJ = J;
    J = nullptr;
    if (Parent != nullptr) {
      // Re-drive the parent on the same call stack with the just-finished
      // child as Trigger. The parent reads any results from the child here
      // (and ONLY here), then we delete the child below.
      Driver::Run(Parent, DeadJ);
    }
    delete DeadJ;
    return;
  }
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