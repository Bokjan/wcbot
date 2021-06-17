#pragma once

#include <vector>

namespace wcbot {

class ThreadContext;

class Job {
 public:
  explicit Job(ThreadContext *Worker) : Worker(Worker), Parent(nullptr) {}
  virtual ~Job() = default;
  virtual void Do() = 0;

 public:
  ThreadContext *Worker;

 protected:
  Job *Parent;
  std::vector<Job *> Children;
};

}  // namespace wcbot
