#pragma once

#include <uv.h>

#include "ITC.h"

namespace wcbot {

class ThreadContext {
public:
  int ThreadIndex;
  uv_loop_t UvLoop;
  uv_thread_t UvThread;
  uv_signal_t UvSignal;
  ItcQueue MainToWorkerQueue;
  ItcQueue WorkerToMainQueue;
  uv_async_t MainToWorkerAsync;
  uv_async_t WorkerToMainAsync;
};

namespace worker_impl {

void EntryPoint(void *Argument);
void DispatchTcp(TcpUvBuffer *Buffer, ThreadContext *Worker);

}

}  // namespace wcbot
