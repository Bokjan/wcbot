#include "Job.h"

namespace wcbot {

namespace job_impl {
class GuardJob final : public Job {
 public:
  GuardJob() : Job(nullptr) {}
  void Do(Job *Trigger) override {}
  void OnTimeout() override {}
};
thread_local GuardJob GuardJobObject;
}  // namespace job_impl

Job::~Job() {
  for (auto ChildJob : this->Children) {
    ChildJob->ResetParent();
  }
}

void Job::RemoveChild(Job *J) {
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

}  // namespace wcbot
