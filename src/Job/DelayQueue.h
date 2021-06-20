#pragma once

#include <chrono>

namespace wcbot {

class IOJob;
class DelayQueueImpl;

class DelayQueue {
 public:
  DelayQueue();
  ~DelayQueue();
  void Join(IOJob* J, int Millisecond);
  IOJob* Remove(uint32_t Id);
  IOJob* Dequeue(std::chrono::time_point<std::chrono::steady_clock> Now);

 protected:
  DelayQueueImpl* PImpl;
};

}  // namespace wcbot
