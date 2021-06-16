#pragma once

#include <vector>

namespace wcbot {

class ThreadContext;

class Job {
 public:
  explicit Job(ThreadContext *Worker) : Worker(Worker), Parent(nullptr) {}
  virtual ~Job() {}
  virtual void Do() = 0;

 private:
  ThreadContext *Worker;
  Job *Parent;
  std::vector<Job *> Children;
};

}  // namespace wcbot
