#pragma once

// Inter-Thread Communication

namespace wcbot {

class ItcEvent;
class ItcQueueImpl;

class ItcQueue {
 public:
  static constexpr int kMaxBatchCount = 100;

  ItcQueue();
  ~ItcQueue();
  void Enqueue(ItcEvent* Event);
  bool TryEnqueue(ItcEvent* Event);
  ItcEvent* Dequeue();
  ItcEvent* TryDequeue();
  bool IsEmpty();
  void Clear();

 protected:
  ItcQueueImpl* PImpl;
};

class ItcEvent {
 public:
  virtual void Process() = 0;
};

}  // namespace wcbot
