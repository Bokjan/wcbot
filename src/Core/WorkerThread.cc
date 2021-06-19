#include "WorkerThread.h"

#include <curl/curl.h>

#include "../Job/HttpClientJob.h"
#include "../Job/HttpHandlerJob.h"
#include "../Job/Job.h"
#include "../Utility/Logger.h"

namespace wcbot {

namespace worker_impl {
static thread_local ThreadContext *Self = nullptr;
static void OnTickTimer(uv_timer_t *Timer);
static void OnItcAsyncSend(uv_async_t *Async);
static void OnSignalInterrupt(uv_signal_t *Signal, int SigNum);
namespace curl {
static int SocketFunction(CURL *Easy, curl_socket_t CurlSocket, int Action, void *UserPtr,
                          void *ContextPtr);
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
  curl_multi_setopt(CurlMultiHandle, CURLMOPT_TIMERDATA, this);
}

void ThreadContext::Initialize() {
  // thread_local global pointer
  worker_impl::Self = this;
  // loop
  uv_loop_init(&UvLoop);
  // tick timer
  uv_timer_init(&UvLoop, &UvTickTimer);
  UvTickTimer.data = this;
  constexpr uint64_t kTickRepeat = 100;  // milliseconds
  uv_timer_start(&UvTickTimer, worker_impl::OnTickTimer, 0, kTickRepeat);
  // async
  uv_async_init(&UvLoop, &MainToWorkerAsync, worker_impl::OnItcAsyncSend);
  MainToWorkerAsync.data = this;
  // signal
  uv_signal_init(&UvLoop, &UvSignal);
  UvSignal.data = this;
  uv_signal_start(&UvSignal, worker_impl::OnSignalInterrupt, SIGINT);
  // cURL
  InitializeCurlMulti();
  uv_timer_init(&UvLoop, &UvCurlTimer);
  UvCurlTimer.data = this;
}

void ThreadContext::Finalize() {
  uv_loop_close(&UvLoop);
  curl_multi_cleanup(CurlMultiHandle);
}

void ThreadContext::DealDealyQueue() {
  const auto Now = std::chrono::steady_clock::now();
  Job *JobPtr;
  while ((JobPtr = DQueue.Dequeue(Now)) != nullptr) {
    JobPtr->OnTimeout(nullptr);
  }
}

void ThreadContext::Tick() {
  // delay queue
  this->DealDealyQueue();
}

namespace worker_impl {

static void OnItcAsyncSend(uv_async_t *Async) {
  ThreadContext *Self = reinterpret_cast<ThreadContext *>(Async->data);
  int i;
  for (i = 0; i < ItcQueue::kMaxBatchCount; ++i) {
    ItcEvent *Event = Self->MainToWorkerQueue.Dequeue();
    if (Event == nullptr) {
      break;
    }
    Event->Process();  // event frees self on finish
  }
  LOG_TRACE("thread #%d processed %d ITC event(s)", Self->ThreadIndex, i);
}

static void OnSignalInterrupt(uv_signal_t *Signal, int SigNum) {
  ThreadContext *Self = reinterpret_cast<ThreadContext *>(Signal->data);
  uv_stop(&Self->UvLoop);
  LOG_ALL("SIGINT captured, worker thread %02d", Self->ThreadIndex);
}

static void OnTickTimer(uv_timer_t *Timer) {
  ThreadContext *Worker = reinterpret_cast<ThreadContext *>(Timer->data);
  Worker->Tick();
}

void EntryPoint(void *Argument) {
  ThreadContext *Self = reinterpret_cast<ThreadContext *>(Argument);
  Self->Initialize();
  uv_run(&Self->UvLoop, UV_RUN_DEFAULT);
  Self->Finalize();
}

void DispatchTcp(TcpMemoryBuffer *Buffer, ThreadContext *Worker) {
  // there's only one possible handler
  Job *NewJob = new HttpHandlerJob(Worker, Buffer);
  NewJob->Do();
}

namespace curl {

struct UvCurlContext {
  uv_poll_t UvPoll;
  curl_socket_t SockFd;
};

static UvCurlContext *CreateCurlContext(ThreadContext *Worker, curl_socket_t Fd) {
  UvCurlContext *Ret = new UvCurlContext;
  Ret->SockFd = Fd;
  int InitRet = uv_poll_init_socket(&Worker->UvLoop, &Ret->UvPoll, Fd);
  if (InitRet != 0) {
    LOG_ERROR("uv_poll_init_socket returns %d, abort", InitRet);
    raise(SIGINT);
    return nullptr;
  }
  Ret->UvPoll.data = Worker;
  return Ret;
}

static void CurlMultiStatusCheck(CURLM *Multi) {
  int Pending;
  CURLMsg *Message;
  while ((Message = curl_multi_info_read(Multi, &Pending)) != nullptr) {
    switch (Message->msg) {
      case CURLMSG_DONE: {
        long ResponseCode;
        HttpClientJob::CurlPrivate PrivateUnion;
        curl_easy_getinfo(Message->easy_handle, CURLINFO_RESPONSE_CODE, &ResponseCode);
        curl_easy_getinfo(Message->easy_handle, CURLINFO_PRIVATE, &PrivateUnion.Ptr);
        Job *J = Self->DQueue.Remove(PrivateUnion.JobId);
        // LOG_TRACE("code=%d, id=%u", ResponseCode, PrivateUnion.JobId);
        if (J != nullptr) {
          HttpClientJob *HCJ = dynamic_cast<HttpClientJob *>(J);
          HCJ->Response.StatusCode = static_cast<int>(ResponseCode);
          HCJ->Do();
        }
        curl_multi_remove_handle(Multi, Message->easy_handle);
        curl_easy_cleanup(Message->easy_handle);
        break;
      }
      default:
        LOG_ERROR("invalid msg type %d, abort", Message->msg);
        raise(SIGINT);
        break;
    }
  }
}

static void UvCurlOnPoll(uv_poll_t *UvPoll, int Status, int Events) {
  int RunningHandles;
  UvCurlContext *Context = reinterpret_cast<UvCurlContext *>(UvPoll);
  ThreadContext *Worker = reinterpret_cast<ThreadContext *>(Context->UvPoll.data);
  uv_timer_stop(&Worker->UvCurlTimer);
  int Flags = 0;
  if (Status < 0) {
    Flags = CURL_CSELECT_ERR;
  } else if (Status == 0) {
    if ((Events & UV_READABLE) != 0) {
      Flags |= CURL_CSELECT_IN;
    }
    if ((Events & UV_WRITABLE) != 0) {
      Flags |= CURL_CSELECT_OUT;
    }
  }
  curl_multi_socket_action(Worker->CurlMultiHandle, Context->SockFd, Flags, &RunningHandles);
  CurlMultiStatusCheck(Worker->CurlMultiHandle);
}

static void UvCurlOnClose(uv_handle_t *Handle) {
  UvCurlContext *Context = reinterpret_cast<UvCurlContext *>(Handle);
  delete Context;
}

static int SocketFunction(CURL *Easy, curl_socket_t CurlSocket, int Action, void *UserPtr,
                          void *ContextPtr) {
  ThreadContext *Worker = reinterpret_cast<ThreadContext *>(UserPtr);
  UvCurlContext *CurlContext = reinterpret_cast<UvCurlContext *>(ContextPtr);
  // make sure thar `CurlContext` is valid for IN and OUT
  if (Action == CURL_POLL_IN || Action == CURL_POLL_OUT) {
    if (ContextPtr != nullptr) {
      CurlContext = reinterpret_cast<UvCurlContext *>(ContextPtr);
    } else {
      CurlContext = CreateCurlContext(Worker, CurlSocket);
      curl_multi_assign(Worker->CurlMultiHandle, CurlSocket, CurlContext);
    }
  }
  // handle this call
  switch (Action) {
    case CURL_POLL_IN:
      uv_poll_start(&CurlContext->UvPoll, UV_READABLE, UvCurlOnPoll);
      break;
    case CURL_POLL_OUT:
      uv_poll_start(&CurlContext->UvPoll, UV_WRITABLE, UvCurlOnPoll);
      break;
    case CURL_POLL_REMOVE:
      // caution, `ContextPtr` may be nullptr here
      if (CurlContext != nullptr) {
        uv_poll_stop(&CurlContext->UvPoll);
        uv_close(reinterpret_cast<uv_handle_t *>(&CurlContext->UvPoll), UvCurlOnClose);
        curl_multi_assign(Worker->CurlMultiHandle, CurlSocket, nullptr);
      }
      break;
    default:
      LOG_ERROR("invalid cURL multi action: %d, abort", Action);
      raise(SIGINT);
  }
  return 0;
}

static void UvCurlTimerOnTimeOut(uv_timer_t *Timer) {
  ThreadContext *Worker = reinterpret_cast<ThreadContext *>(Timer->data);
  int RunningHandles;
  constexpr int Flags = 0;
  curl_multi_socket_action(Worker->CurlMultiHandle, CURL_SOCKET_TIMEOUT, Flags, &RunningHandles);
  CurlMultiStatusCheck(Worker->CurlMultiHandle);
}

static void TimerFunction(CURLM *CurlMulti, long TimeoutMS, void *UserPtr) {
  // LOG_DEBUG("TimeoutMS=%ld", TimeoutMS);
  // get thread context
  ThreadContext *Worker = reinterpret_cast<ThreadContext *>(UserPtr);
  // -1 means stop
  if (TimeoutMS == -1) {
    uv_timer_stop(&Worker->UvCurlTimer);
  }
  // call uv
  uv_timer_start(&Worker->UvCurlTimer, UvCurlTimerOnTimeOut, TimeoutMS, 0);
}

}  // namespace curl

}  // namespace worker_impl

}  // namespace wcbot
