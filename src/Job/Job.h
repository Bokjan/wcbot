#pragma once

#include <vector>

namespace wcbot {

class ThreadContext;

class Job {
 public:
  explicit Job(ThreadContext *Worker) : Worker(Worker), Parent(nullptr) {}
  virtual ~Job() = default;
  virtual void Do(Job *Trigger = nullptr) = 0;
  virtual void OnTimeout() = 0;

 public:
  ThreadContext *Worker;

 protected:
  Job *Parent;
  std::vector<Job *> Children;
};

}  // namespace wcbot
