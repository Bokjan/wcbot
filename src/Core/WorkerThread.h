#pragma once

#include <uv.h>

#include "ITC.h"

namespace wcbot {

class ThreadContext {
public:
  int ThreadIndex;
  uv_loop_t UvLoop;
  uv_thread_t UvThread;
  ItcQueue MainToWorkerQueue;
  ItcQueue WorkerToMainQueue;
  uv_async_t MainToWorkerAsync;
  uv_async_t WorkerToMainAsync;
};

namespace workerthread {

void EntryPoint(void *Argument);

}
}  // namespace wcbot
