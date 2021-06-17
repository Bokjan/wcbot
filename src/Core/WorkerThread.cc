#include "WorkerThread.h"

#include <curl/curl.h>

#include "Job/HttpHandlerJob.h"
#include "Utility/Logger.h"

namespace wcbot {

namespace worker_impl {
namespace curl {
static int SocketFunction(CURL *Easy, curl_socket_t CurlSocket, int Action, void *UserPtr,
                          void *SocketPtr);
static void TimerFunction(CURLM *CurlMulti, long TimeoutMS, void *UserPtr);
}  // namespace curl
}  // namespace worker_impl

void ThreadContext::NotifyMain() { uv_async_send(&this->WorkerToMainAsync); }

void ThreadContext::NotifyWorker() { uv_async_send(&this->MainToWorkerAsync); }

void ThreadContext::InitializeCurlMulti() {
  CurlMultiHandle = curl_multi_init();
  curl_multi_setopt(CurlMultiHandle, CURLMOPT_SOCKETFUNCTION, worker_impl::curl::SocketFunction);
  curl_multi_setopt(CurlMultiHandle, CURLMOPT_SOCKETDATA, this);
  curl_multi_setopt(CurlMultiHandle, CURLMOPT_TIMERFUNCTION, worker_impl::curl::TimerFunction);
}

void ThreadContext::Finalize() { curl_multi_cleanup(CurlMultiHandle); }

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
  // cURL timer
  uv_timer_init(&Self->UvLoop, &Self->UvCurlTimer);
}

static void AfterLoop(ThreadContext *Self) {
  uv_loop_close(&Self->UvLoop);
  Self->Finalize();
}

void EntryPoint(void *Argument) {
  ThreadContext *Self = reinterpret_cast<ThreadContext *>(Argument);
  BeforeLoop(Self);
  uv_run(&Self->UvLoop, UV_RUN_DEFAULT);
  AfterLoop(Self);
}

void DispatchTcp(TcpMemoryBuffer *Buffer, ThreadContext *Worker) {
  // todo: http handler
  Job *NewJob = new HttpHandlerJob(Worker, Buffer);
  NewJob->Do();
}

namespace curl {

struct curl_context_t {
  uv_poll_t poll_handle;
  curl_socket_t sockfd;
};

static curl_context_t *CreateCurlContext(ThreadContext *Worker, curl_socket_t Fd) {
  curl_context_t *Ret = new curl_context_t;
  Ret->sockfd = Fd;
  int InitRet = uv_poll_init_socket(&Worker->UvLoop, &Ret->poll_handle, Fd);
  if (InitRet != 0) {
    LOG_ERROR("uv_poll_init_socket returns %d, abort", InitRet);
    raise(SIGINT);
    return nullptr;
  }
  Ret->poll_handle.data = Ret;
  return Ret;
}

static int SocketFunction(CURL *Easy, curl_socket_t CurlSocket, int Action, void *UserPtr,
                          void *SocketPtr) {
  ThreadContext *Worker = reinterpret_cast<ThreadContext *>(UserPtr);
  curl_context_t *CurlContext;
  // make sure thar `CurlContext` is valid
  if (Action == CURL_POLL_IN || Action == CURL_POLL_OUT) {
    if (SocketPtr != nullptr) {
      CurlContext = reinterpret_cast<curl_context_t *>(SocketPtr);
    } else {
      CurlContext = CreateCurlContext(Worker, CurlSocket);
      curl_multi_assign(Worker->CurlMultiHandle, CurlSocket, CurlContext);
    }
  }
  // handle this call
  switch (Action) {
    case CURL_POLL_IN:
    // uv_poll_start();
    break;
    case CURL_POLL_OUT:
    break;
    case CURL_POLL_REMOVE:
    break;
    default:
    LOG_ERROR("invalid cURL multi action: %d, abort", Action);
    raise(SIGINT);
  }
  return 0;
}

static void TimerFunction(CURLM *CurlMulti, long TimeoutMS, void *UserPtr);

}  // namespace curl

}  // namespace worker_impl

}  // namespace wcbot
