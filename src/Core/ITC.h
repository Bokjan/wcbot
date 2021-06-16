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

class TcpUvBuffer;
class ThreadContext;
class EngineImpl;

namespace itc {

class Tcp : public ItcEvent {
 public:
  explicit Tcp(TcpUvBuffer* Buffer) : Buffer(Buffer) {}
  void Process() = 0;

 protected:
  TcpUvBuffer* Buffer;
};

class TcpMainToWorker final : public Tcp {
 public:
  explicit TcpMainToWorker(TcpUvBuffer* Buffer, ThreadContext* Worker)
      : Tcp(Buffer), Worker(Worker) {}
  void Process();
  void DeleteSelf() { delete this; }

 private:
  ThreadContext* Worker;
};

class TcpWorkerToMain final : public Tcp {
 public:
  explicit TcpWorkerToMain(TcpUvBuffer* Buffer) : Tcp(Buffer) {}
  void Process();
  void DeleteSelf() { delete this; }
};
}  // namespace itc

}  // namespace wcbot
