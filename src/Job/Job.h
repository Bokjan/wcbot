#pragma once

#include <cstdint>

#include <vector>

namespace wcbot {

class ThreadContext;

class Job {
 public:
  explicit Job(ThreadContext *Worker);
  Job(const Job&) = delete;
  Job(const Job&&) = delete;
  virtual ~Job();
  virtual void Do(Job *Trigger = nullptr) = 0;
  virtual void OnTimeout(Job *Trigger) = 0;
  void SetParent(Job *P) { Parent = P; }
  void ResetParent() { Parent = nullptr; }
  void RemoveChild(Job *J);
  Job *SafeParent();
  uint32_t GetJobId() { return JobId; }
  void SetJobId(uint32_t Id) { JobId = Id; }
  void JoinDelayQueue(int TimeoutMS);

 public:
  int ErrCode;
  ThreadContext *Worker;

 protected:
  uint32_t JobId;
  Job *Parent;
  std::vector<Job *> Children;
};

}  // namespace wcbot
