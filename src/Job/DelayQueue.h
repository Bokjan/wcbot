#pragma once

#include <chrono>

namespace wcbot {

class Job;
class DelayQueueImpl;

class DelayQueue {
 public:
  DelayQueue();
  ~DelayQueue();
  void Join(Job* J, int Millisecond);
  Job* Remove(uint32_t Id);
  Job* Dequeue(std::chrono::time_point<std::chrono::steady_clock> Now);

 protected:
  DelayQueueImpl* PImpl;
};

}  // namespace wcbot
