#include "WorkerThread.h"

#include "Utility/Logger.h"
#include "Job/HttpHandlerJob.h"

namespace wcbot {
namespace worker_impl {

static void OnItcAsyncSend(uv_async_t *Async) {
  ThreadContext *Self = reinterpret_cast<ThreadContext *>(Async->data);
  int i;
  for (i = 0; i < ItcQueue::kMaxBatchCount; ++i) {
    ItcEvent *Event = Self->MainToWorkerQueue.Dequeue();
    if (Event == nullptr) {
      break;
    }
    Event->Process();
  }
  LOG_TRACE("thread #%d processed %d ITC event(s)", Self->ThreadIndex, i);
}

static void OnSignalInterrupt(uv_signal_t *Signal, int SigNum) {
  ThreadContext *Self = reinterpret_cast<ThreadContext *>(Signal->data);
  uv_stop(&Self->UvLoop);
  LOG_ALL("SIGINT captured, worker thread %02d", Self->ThreadIndex);
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

void DispatchTcp(TcpUvBuffer *Buffer, ThreadContext *Worker) {
  // todo: http handler
  Job *NewJob = new HttpHandlerJob(Worker, Buffer);
  NewJob->Do();
}

}  // namespace worker_impl
}  // namespace wcbot
