#pragma once

#include <chrono>

namespace wcbot {

class Job;
class DelayQueueImpl;

class DelayQueue {
 public:
  DelayQueue();
  ~DelayQueue();
  bool Join(Job* J, int Millisecond);
  bool Remove(Job* J);
  Job* Dequeue(std::chrono::time_point<std::chrono::steady_clock> Now);

 protected:
  DelayQueueImpl* PImpl;
};

}  // namespace wcbot
