#pragma once

#include <uv.h>

#include "ITC.h"

namespace wcbot {

class EngineImpl;

class ThreadContext final {
 public:
  EngineImpl *EImpl;
  int ThreadIndex;
  uv_loop_t UvLoop;
  uv_thread_t UvThread;
  uv_signal_t UvSignal;
  ItcQueue MainToWorkerQueue;
  ItcQueue WorkerToMainQueue;
  uv_async_t MainToWorkerAsync;
  uv_async_t WorkerToMainAsync;
  void *CurlMultiHandle;
  uv_timer_t UvCurlTimer;

  void NotifyMain();
  void NotifyWorker();

  void Finalize();

 private:
  void InitializeCurlMulti();
  friend class EngineImpl;
};

namespace worker_impl {

void EntryPoint(void *Argument);
void DispatchTcp(TcpMemoryBuffer *Buffer, ThreadContext *Worker);

}  // namespace worker_impl

}  // namespace wcbot
