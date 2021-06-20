#pragma once

#include <cstdint>

#include <vector>

namespace wcbot {

class Job {
 public:
  explicit Job(Job *Parent = nullptr);
  Job(const Job &) = delete;
  Job(const Job &&) = delete;
  virtual ~Job();
  virtual void Do(Job *Trigger = nullptr);
  void SetParent(Job *P) { Parent = P; }
  void ResetParent() { Parent = nullptr; }
  void RemoveChild(Job *J);
  Job *SafeParent();

  void InvokeChild(Job *Child, Job *DoArgument = nullptr);

 public:
  enum ErrCodeEnum { kErrCodeTimeout = 9999 };
  int ErrCode;

 protected:
  Job *Parent;
  std::vector<Job *> Children;
};

class IOJob : public Job {
 public:
  IOJob(Job *Parent = nullptr) : Job(Parent) {}
  void JoinDelayQueue(int TimeoutMS);
  virtual void OnTimeout() = 0;
  uint32_t GetJobId() { return JobId; }
  void SetJobId(uint32_t Id) { JobId = Id; }

 protected:
  uint32_t JobId;
};

using FN_CreateJob = Job *(*)();

}  // namespace wcbot
