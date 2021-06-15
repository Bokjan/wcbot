#include "WorkerThread.h"

namespace wcbot {
namespace workerthread {

static void OnAsyncSend(uv_async_t *Async) {
  ThreadContext *Self = reinterpret_cast<ThreadContext *>(Async->data);
  // todo
}

static void BeforeLoop(ThreadContext *Self) {
  uv_loop_init(&Self->UvLoop);
  Self->MainToWorkerAsync.data = Self;
  uv_async_init(&Self->UvLoop, &Self->MainToWorkerAsync, OnAsyncSend);
}

static void AfterLoop(ThreadContext *Self) { uv_loop_close(&Self->UvLoop); }

void EntryPoint(void *Argument) {
  ThreadContext *Self = reinterpret_cast<ThreadContext *>(Argument);
  BeforeLoop(Self);

  // thread loop

  AfterLoop(Self);
}

}  // namespace workerthread
}  // namespace wcbot
