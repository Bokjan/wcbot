#include "Job.h"

#include "../Core/WorkerThread.h"
#include "../Utility/Logger.h"

namespace wcbot {

namespace job_impl {
class GuardJob final : public Job {
 public:
  GuardJob() : Job() {}
  void Do(Job *Trigger) override {
    // LOG_TRACE("%s", "job_impl::GuardJob::Do()");
  }
};
static GuardJob GuardJobObject;
}  // namespace job_impl

Job::Job() : ErrCode(0), Parent(nullptr) {}

Job::~Job() {
  for (auto ChildJob : this->Children) {
    ChildJob->ResetParent();
  }
}

void Job::RemoveChild(Job *J) {
  if (J == nullptr) {
    return;
  }
  for (auto It = Children.begin(); It != Children.end(); ++It) {
    if (*It != J) {
      continue;
    }
    (*It)->ResetParent();
    Children.erase(It);
    break;
  }
}

Job *Job::SafeParent() { return Parent == nullptr ? &job_impl::GuardJobObject : Parent; }

void IOJob::JoinDelayQueue(int TimeoutMS) {
  worker_impl::g_ThisThread->JoinDelayQueue(this, TimeoutMS);
}

void Job::Sleep(int Millisecond) { worker_impl::g_ThisThread->JoinSleepQueue(this, Millisecond); }

void Job::InvokeChild(Job *J, Job *DoArgument) {
  Children.emplace_back(J);
  J->SetParent(this);
  J->Do(DoArgument);
}

void Job::NotifyParent() {
  Job *Parent = SafeParent();
  Parent->RemoveChild(this);
  Parent->Do(this);
}

}  // namespace wcbot
