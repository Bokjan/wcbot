#pragma once

#include <chrono>
#include <memory>

namespace wcbot {

class Job;
class IOJob;
class DelayQueueImpl;
class SleepQueueImpl;

class DelayQueue {
 public:
  DelayQueue();
  ~DelayQueue();
  void Join(IOJob* J, int Millisecond);
  IOJob* Remove(uint32_t Id);
  IOJob* Dequeue(std::chrono::time_point<std::chrono::steady_clock> Now);

 protected:
  std::unique_ptr<DelayQueueImpl> PImpl;
};

class SleepQueue final {
 public:
  SleepQueue();
  ~SleepQueue();
  void Join(Job* J, int Millisecond);
  Job* Dequeue(std::chrono::time_point<std::chrono::steady_clock> Now);

 private:
  std::unique_ptr<SleepQueueImpl> PImpl;
};

}  // namespace wcbot
