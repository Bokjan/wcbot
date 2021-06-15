#include "WorkerThread.h"

namespace wcbot {
namespace workerthread {

static void OnItcAsyncSend(uv_async_t *Async) {
  ThreadContext *Self = reinterpret_cast<ThreadContext *>(Async->data);
  // todo
}

static void OnSignalInterrupt(uv_signal_t *Signal, int SigNum) {
  ThreadContext *Self = reinterpret_cast<ThreadContext *>(Signal->data);
  uv_stop(&Self->UvLoop);
  fprintf(stderr, "SIGINT captured, worker thread %02d\n", Self->ThreadIndex);
}

static void BeforeLoop(ThreadContext *Self) {
  // loop
  uv_loop_init(&Self->UvLoop);
  // async
  uv_async_init(&Self->UvLoop, &Self->MainToWorkerAsync, OnItcAsyncSend);
  Self->MainToWorkerAsync.data = Self;
  // signal
  uv_signal_init(&Self->UvLoop, &Self->UvSignal);
  Self->UvSignal.data = Self;
  uv_signal_start(&Self->UvSignal, OnSignalInterrupt, SIGINT);
}

static void AfterLoop(ThreadContext *Self) { uv_loop_close(&Self->UvLoop); }

void EntryPoint(void *Argument) {
  ThreadContext *Self = reinterpret_cast<ThreadContext *>(Argument);
  BeforeLoop(Self);
  uv_run(&Self->UvLoop, UV_RUN_DEFAULT);
  AfterLoop(Self);
}

}  // namespace workerthread
}  // namespace wcbot
