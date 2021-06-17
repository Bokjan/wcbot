#pragma once

#include <chrono>
#include <queue>

#include <uv.h>

#include "ITC.h"
#include "Job/DelayQueue.h"

namespace wcbot {

class EngineImpl;

class ThreadContext final {
 public:
  // basic components
  EngineImpl *EImpl;
  int ThreadIndex;
  uv_loop_t UvLoop;
  uv_idle_t UvIdle;
  uv_thread_t UvThread;
  uv_signal_t UvSignal;
  ItcQueue MainToWorkerQueue;
  ItcQueue WorkerToMainQueue;
  uv_async_t MainToWorkerAsync;
  uv_async_t WorkerToMainAsync;
  DelayQueue DQueue;
  // cURL related
  void *CurlMultiHandle;
  uv_timer_t UvCurlTimer;

  void NotifyMain();
  void NotifyWorker();

  void DoIdle();
  void DealDealyQueue();
  bool JoinDelayQueue(Job *J, int Millis) {
    return DQueue.Join(J, Millis);
  }
  void InitializeCurlMulti();
  void Finalize();

 private:
  friend class EngineImpl;
};

namespace worker_impl {

void EntryPoint(void *Argument);
void DispatchTcp(TcpMemoryBuffer *Buffer, ThreadContext *Worker);

}  // namespace worker_impl

}  // namespace wcbot
