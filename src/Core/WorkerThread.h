#pragma once

#include <chrono>
#include <queue>

#include <uv.h>

#include "ITC.h"
#include "../Core/TimeQueue.h"

namespace wcbot {

class EngineImpl;

class ThreadContext final {
 public:
  // basic components
  EngineImpl *EImpl;       // the `EngineImpl` which holds self
  int ThreadIndex;         // index of thread, starts from 0
  uv_loop_t UvLoop;        // uv_loop for this thread
  uv_timer_t UvTickTimer;  // uv_timer to do tick
  uv_thread_t UvThread;    // the thread structure
  uv_signal_t UvSignal_SIGINT;    // signal handler
  uv_signal_t UvSignal_SIGTERM;   // signal handler
  ItcQueue MainToWorkerQueue;
  ItcQueue WorkerToMainQueue;
  uv_async_t MainToWorkerAsync;
  uv_async_t WorkerToMainAsync;
  DelayQueue DQueue;
  SleepQueue SQueue;
  // cURL related
  void *CurlMultiHandle;
  uv_timer_t UvCurlTimer;

  ThreadContext() = default;
  ThreadContext(const ThreadContext&) = delete;
  ThreadContext(const ThreadContext&&) = delete;

  void Initialize();
  void Finalize();

  void NotifyMain();
  void NotifyWorker();

  void Tick();
  void DealDealyQueue();
  void JoinDelayQueue(IOJob *J, int Millis) { DQueue.Join(J, Millis); }
  void DealSleepQueue();
  void JoinSleepQueue(Job *J, int Millis) { SQueue.Join(J, Millis); }
  void InitializeCurlMulti();

 private:
  friend class EngineImpl;
};

namespace worker_impl {
extern thread_local ThreadContext *g_ThisThread;
void EntryPoint(void *Argument);
void DispatchTcp(TcpMemoryBuffer *Buffer);
}  // namespace worker_impl

}  // namespace wcbot
