#pragma once

#include <cstdint>

#include <vector>

namespace wcbot {

class Job {
 public:
  Job();
  Job(const Job &) = delete;
  Job(const Job &&) = delete;
  virtual ~Job();
  // The `Job` state machine is driven by `Do`
  virtual void Do(Job *Trigger = nullptr) = 0;
  // Call `DeleteThis` before your state machine exit
  void DeleteThis() { delete this; }
  void SetParent(Job *P) { Parent = P; }
  void ResetParent() { Parent = nullptr; }
  void RemoveChild(Job *J);
  // Always use `SafeParent()->Do(this)`
  // if you want to notify parent something
  Job *SafeParent();
  // Always start a child job by `InvokeChild`
  // Don't directly call `Child->Do()`
  void InvokeChild(Job *Child, Job *DoArgument = nullptr);
  void NotifyParent();

 public:
  // ErrCode = 0, success
  // ErrCode < 0, framework error
  // ErrCode > 0, user-defined error
  int ErrCode;
  enum ErrEnum { kErrTimeout = -9999 };

 private:
  Job *Parent;
  std::vector<Job *> Children;
};

class IOJob : public Job {
 public:
  IOJob(Job *Parent = nullptr) : Job() {}
  void JoinDelayQueue(int TimeoutMS);
  virtual void OnTimeout() = 0;
  uint32_t GetJobId() { return JobId; }
  void SetJobId(uint32_t Id) { JobId = Id; }

 protected:
  uint32_t JobId;
};

using FN_CreateJob = Job *(*)();

}  // namespace wcbot
