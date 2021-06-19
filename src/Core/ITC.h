#pragma once

#include <cstdint>

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
  virtual ~ItcEvent() = default;
  virtual void Process() = 0;
};

class MemoryBuffer;
class TcpMemoryBuffer;
class ThreadContext;
class EngineImpl;

namespace itc {

class TcpMainToWorker final : public ItcEvent {
 public:
  explicit TcpMainToWorker(TcpMemoryBuffer* Buffer, ThreadContext* Worker)
      : Buffer(Buffer), Worker(Worker) {}
  ~TcpMainToWorker() = default;
  void Process() override;
  void DeleteThis() { delete this; }

 private:
  TcpMemoryBuffer* Buffer;
  ThreadContext* Worker;
};

class TcpWorkerToMain final : public ItcEvent {
 public:
  explicit TcpWorkerToMain(EngineImpl* EImpl, MemoryBuffer* Buffer, uint64_t ConnId)
      : EImpl(EImpl), Buffer(Buffer), ConnId(ConnId), CloseConnection(false) {}
  ~TcpWorkerToMain() = default;
  void SetCloseConnection() { CloseConnection = true; }
  void Process() override;
  void DeleteThis() { delete this; }

 private:
  EngineImpl* EImpl;
  MemoryBuffer* Buffer;
  uint64_t ConnId;
  bool CloseConnection;
};

}  // namespace itc

}  // namespace wcbot
