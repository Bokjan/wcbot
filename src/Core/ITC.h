#pragma once

#include <cstdint>

#include <memory>
#include <utility>

#include "../Job/Job.h"

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
  std::unique_ptr<ItcQueueImpl> PImpl;
};

class ItcEvent {
 public:
  virtual ~ItcEvent() = default;
  virtual void Process() = 0;
};

// ----------------------------------------------------------------------------
// ItcEvent ownership convention
//
// `ItcEvent` instances are deliberately owned via raw pointers across thread
// boundaries: the producer `new`s an event and `Enqueue`s it; the consumer
// `Dequeue`s it, calls `Process()`, and then `Process()` itself calls
// `DeleteThis()` to free the event. This single-owner relay-baton model is
// the canonical idiom for cross-thread message passing and is intentionally
// kept as raw pointers here.
//
// Embedded payload fields (e.g. `TcpMemoryBuffer*`, `MemoryBuffer*`) follow
// the same single-ownership baton: `Process()` hands the payload off to the
// receiving thread's owner (worker job / main loop write) and that owner is
// then responsible for the eventual `delete`.
// ----------------------------------------------------------------------------

class MemoryBuffer;
class TcpMemoryBuffer;
class ThreadContext;
class EngineImpl;

namespace itc {

class TcpMainToWorker final : public ItcEvent {
 public:
  explicit TcpMainToWorker(TcpMemoryBuffer* Buffer) : Buffer(Buffer) {}
  ~TcpMainToWorker() = default;
  void Process() override;
  void DeleteThis() { delete this; }

 private:
  TcpMemoryBuffer* Buffer;
};

class TcpWorkerToMain final : public ItcEvent {
 public:
  explicit TcpWorkerToMain(MemoryBuffer* Buffer, uint64_t ConnId)
      : Buffer(Buffer), ConnId(ConnId), CloseConnection(false) {}
  ~TcpWorkerToMain() = default;
  void SetCloseConnection() { CloseConnection = true; }
  void Process() override;
  void DeleteThis() { delete this; }

 private:
  MemoryBuffer* Buffer;
  uint64_t ConnId;
  bool CloseConnection;
};

class JobCreateAndRun final : public ItcEvent {
 public:
  explicit JobCreateAndRun(FN_CreateJob Function) : Function(std::move(Function)) {}
  ~JobCreateAndRun() = default;
  void Process() override;
  void DeleteThis() { delete this; }

 private:
  FN_CreateJob Function;
};

}  // namespace itc

}  // namespace wcbot
