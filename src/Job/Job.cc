#include "Job.h"

#include "../Core/WorkerThread.h"

namespace wcbot {

namespace job_impl {
class GuardJob final : public Job {
 public:
  GuardJob() : Job(nullptr) {}
  void Do(Job *Trigger) override {}
};
static thread_local GuardJob GuardJobObject;
}  // namespace job_impl

Job::Job(Job *Parent) : ErrCode(0), Parent(Parent) {}

Job::~Job() {
  for (auto ChildJob : this->Children) {
    ChildJob->ResetParent();
  }
}

void Job::Do(Job *Trigger) { RemoveChild(Trigger); }

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

void Job::InvokeChild(Job *J, Job *DoArgument) {
  Children.emplace_back(J);
  J->SetParent(this);
  J->Do(DoArgument);
}

}  // namespace wcbot
