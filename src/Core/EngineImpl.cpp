#include "EngineImpl.h"
#include "TcpUvBuffer.h"
#include "Utility/Common.h"
#include "Utility/Logger.h"
#include "WorkerThread.h"

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

namespace wcbot {

namespace main_impl {
static void OnTcpRead(uv_stream_t *Handle, ssize_t NRead, const uv_buf_t *Buffer);
static void OnTcpClose(uv_handle_t *Handle);
static void OnNewConnection(uv_stream_t *Handle, int Status);
static void AllocateBuffer(uv_handle_t *Handle, size_t SuggestedSize, uv_buf_t *Buffer);
static void OnItcAsyncSend(uv_async_t *Async);
static void OnSignalInterrupt(uv_signal_t *Signal, int SigNum);
}  // namespace main_impl

#ifdef CHECK_INT_NOT_ZERO_RET
#undef CHECK_INT_NOT_ZERO_RET
#endif
#define CHECK_INT_NOT_ZERO_RET(x) \
  do {                            \
    int Check = x;                \
    if (Check != 0) {             \
      LOG_ERROR("%s", #x);        \
      return Check;               \
    }                             \
  } while (false);

static inline bool RapidJsonGetString(const rapidjson::Value &Value, const char *Member,
                                      std::string &Destination) {
  if (!Value[Member].IsString()) {
    return false;
  }
  Destination = std::string(Value[Member].GetString(), Value[Member].GetStringLength());
  return true;
}

static inline bool RapidJsonGetUInt64(const rapidjson::Value &Value, const char *Member,
                                      uint64_t &Destination) {
  if (!Value[Member].IsUint64()) {
    return false;
  }
  Destination = Value[Member].GetUint64();
  return true;
}

static inline bool RapidJsonGetUInt32(const rapidjson::Value &Value, const char *Member,
                                      uint32_t &Destination) {
  if (!Value[Member].IsUint()) {
    return false;
  }
  Destination = Value[Member].GetUint();
  return true;
}

static inline bool RapidJsonGetInt32(const rapidjson::Value &Value, const char *Member,
                                     int32_t &Destination) {
  if (!Value[Member].IsInt()) {
    return false;
  }
  Destination = Value[Member].GetInt();
  return true;
}

#define BREAK_ON_FALSE(x) \
  if (!x) {               \
    LOG_ERROR("%s", #x);  \
    break;                \
  }

static bool InternalParseConfig(BotConfig &Config, const rapidjson::Document &Json) {
  Config.ParseOk = false;
  do {
    // Bot
    if (!Json["Bot"].IsObject()) {
      break;
    }
    const rapidjson::Value &Bot = Json["Bot"];
    BREAK_ON_FALSE(RapidJsonGetString(Bot, "WebHook", Config.Bot.WebHook));
    BREAK_ON_FALSE(RapidJsonGetString(Bot, "Token", Config.Bot.Token));
    BREAK_ON_FALSE(RapidJsonGetString(Bot, "EncodingAesKey", Config.Bot.EncodingAesKey));

    // Http
    if (!Json["Http"].IsObject()) {
      break;
    }
    const rapidjson::Value &Http = Json["Http"];
    BREAK_ON_FALSE(RapidJsonGetString(Http, "BindIpv4", Config.Http.BindIpv4));
    if (Http.HasMember("BindPort") && Http["BindPort"].IsInt()) {
      Config.Http.BindPort = Http["BindPort"].GetInt();
    } else {
      break;
    }

    // Network
    if (!Json["Network"].IsObject()) {
      break;
    }
    const rapidjson::Value &Network = Json["Network"];
    BREAK_ON_FALSE(
        RapidJsonGetUInt64(Network, "MaxRecvBuffLength", Config.Network.MaxRecvBuffLength));
    BREAK_ON_FALSE(
        RapidJsonGetUInt64(Network, "MaxSendBuffLength", Config.Network.MaxSendBuffLength));

    // Framework
    const rapidjson::Value &Framework = Json["Framework"];
    BREAK_ON_FALSE(RapidJsonGetUInt32(Framework, "WorkerThread", Config.Framework.WorkerThread));
    if (RapidJsonGetString(Framework, "LogLevel", Config.Framework.LogLevel)) {
      BREAK_ON_FALSE(logger_internal::g_Logger->SetLevel(Config.Framework.LogLevel));
    }

    // CustomConfig
    if (Json.HasMember("CustomConfig")) {
      Config.CustomConfig = Json["CustomConfig"].GetString();
    }

    // Finish
    Config.ParseOk = true;
  } while (false);
  return Config.ParseOk;
}

class ThreadDispatcher {
 public:
  explicit ThreadDispatcher(ssize_t Count) : ThreadCount(Count) {}
  virtual ~ThreadDispatcher() {}
  virtual ssize_t NextThreadIndex() = 0;

 protected:
  ssize_t ThreadCount;
};

class RoundRobinThreadDispatcher final : public ThreadDispatcher {
 public:
  explicit RoundRobinThreadDispatcher(ssize_t Count) : ThreadDispatcher(Count), Current(0) {}
  ~RoundRobinThreadDispatcher() {}
  ssize_t NextThreadIndex() {
    ssize_t Ret = Current;
    ++Current;
    if (Current == ThreadCount) {
      Current = 0;
    }
    return Ret;
  }

 private:
  ssize_t Current;
};

EngineImpl::EngineImpl() : IsFork(true), UvLoop(uv_default_loop()), Dispatcher(nullptr) {}

EngineImpl::~EngineImpl() {
  // server codec
  for (auto Ptr : ServerCodecs) {
    delete Ptr;
  }
  ServerCodecs.clear();
  // client codec
  for (auto Ptr : ClientCodecs) {
    delete Ptr;
  }
  ClientCodecs.clear();
  // worker thread
  for (auto Ptr : Threads) {
    delete Ptr;
  }
  Threads.clear();
  // dispatcher
  if (Dispatcher == nullptr) {
    delete Dispatcher;
  }
}

bool EngineImpl::ParseConfig(const std::string &Path) {
  bool Ret = false;
  do {
    std::string ConfigContent;
    if (!utility::ReadFile(Path, ConfigContent)) {
      break;
    }
    rapidjson::Document Json;
    Json.Parse(ConfigContent.c_str());
    if (!Json.IsObject()) {
      break;
    }
    Ret = InternalParseConfig(this->Config, Json);
  } while (false);
  return Ret;
}

int EngineImpl::Run() {
  // start threads
  for (ThreadContext *Ptr : Threads) {
    uv_thread_create(&Ptr->UvThread, worker_impl::EntryPoint, Ptr);
  }
  // start server
  uv_tcp_t UvServerTcp;
  sockaddr_in SockAddr;
  CHECK_INT_NOT_ZERO_RET(uv_tcp_init(this->UvLoop, &UvServerTcp));
  CHECK_INT_NOT_ZERO_RET(
      uv_ip4_addr(Config.Http.BindIpv4.c_str(), Config.Http.BindPort, &SockAddr));
  CHECK_INT_NOT_ZERO_RET(uv_tcp_bind(&UvServerTcp, reinterpret_cast<sockaddr *>(&SockAddr), 0));
  constexpr int kDefaultBacklog = 128;
  CHECK_INT_NOT_ZERO_RET(uv_listen(reinterpret_cast<uv_stream_t *>(&UvServerTcp), kDefaultBacklog,
                                   main_impl::OnNewConnection));
  UvServerTcp.data = this;  // bind self
  int Ret = uv_run(this->UvLoop, UV_RUN_DEFAULT);
  LOG_TRACE("uv_run ret=%d", Ret);
  // join worker threads
  for (ThreadContext *Ptr : Threads) {
    uv_thread_join(&Ptr->UvThread);
  }
  return Ret;
}

bool EngineImpl::Initialize() {
  bool Ret = false;
  do {
    BREAK_ON_FALSE(this->InitializeInterThreadCommunication());
    BREAK_ON_FALSE(this->InitializeSignalHandler());
    this->Dispatcher = new RoundRobinThreadDispatcher(Config.Framework.WorkerThread);
    Ret = true;
  } while (false);
  return Ret;
}

bool EngineImpl::InitializeInterThreadCommunication() {
  for (uint32_t i = 0; i < Config.Framework.WorkerThread; ++i) {
    ThreadContext *Worker = new ThreadContext();
    Worker->ThreadIndex = i;
    uv_async_init(this->UvLoop, &Worker->WorkerToMainAsync, main_impl::OnItcAsyncSend);
    Worker->WorkerToMainAsync.data = Worker;
    Threads.push_back(Worker);
  }
  return true;
}

bool EngineImpl::InitializeSignalHandler() {
  uv_signal_init(this->UvLoop, &this->UvSignal);
  this->UvSignal.data = this;
  uv_signal_start(&this->UvSignal, main_impl::OnSignalInterrupt, SIGINT);
  return true;
}

namespace main_impl {

static void OnNewConnection(uv_stream_t *Tcp, int Status) {
  LOG_TRACE("enter");
  if (Status < 0) {
    LOG_ERROR("TCP new connection error, status=%d", Status);
    return;
  }
  EngineImpl *Impl = reinterpret_cast<EngineImpl *>(Tcp->data);
  TcpUvBufferPtr Buffer = new TcpUvBuffer();
  Buffer->ServerTcp = reinterpret_cast<uv_tcp_t *>(Tcp);
  uv_tcp_init(Impl->UvLoop, &Buffer->ClientTcp);
  Buffer->ClientTcp.data = Buffer;
  if (uv_accept(Tcp, reinterpret_cast<uv_stream_t *>(&Buffer->ClientTcp)) == 0) {
    uv_read_start(reinterpret_cast<uv_stream_t *>(&Buffer->ClientTcp), main_impl::AllocateBuffer,
                  main_impl::OnTcpRead);
  } else {
    uv_close(reinterpret_cast<uv_handle_t *>(&Buffer->ClientTcp), main_impl::OnTcpClose);
  }
}

static void OnTcpClose(uv_handle_t *Handle) {
  LOG_TRACE("enter");
  void *UserData = Handle->data;
  delete reinterpret_cast<TcpUvBufferPtr>(UserData);
}

static void AllocateBuffer(uv_handle_t *Handle, size_t SuggestedSize, uv_buf_t *Buffer) {
  LOG_TRACE("enter");
  TcpUvBufferPtr BufferObj = reinterpret_cast<TcpUvBufferPtr>(Handle->data);
  BufferObj->Allocate(SuggestedSize);
  Buffer->base = BufferObj->GetCurrent();
  Buffer->len = SuggestedSize;
}

static void OnTcpRead(uv_stream_t *Handle, ssize_t NRead, const uv_buf_t *Buffer) {
  LOG_TRACE("enter");
  TcpUvBufferPtr BufferObj = reinterpret_cast<TcpUvBufferPtr>(Handle->data);
  EngineImpl *PImpl = reinterpret_cast<EngineImpl *>(BufferObj->ServerTcp->data);
  // currently used buffer size is larger than configured value
  if (BufferObj->GetCapacity() > PImpl->Config.Network.MaxRecvBuffLength) {
    LOG_TRACE("enter");
    uv_close(reinterpret_cast<uv_handle_t *>(Handle), main_impl::OnTcpClose);
    return;
  }
  // check size is ok
  if (NRead < 0) {
    if (NRead != UV_EOF) {
      LOG_ERROR("TCP read error, nread=%ld", NRead);
    }
    uv_close(reinterpret_cast<uv_handle_t *>(Handle), main_impl::OnTcpClose);
    return;
  }
  // just increase the buffer length value
  BufferObj->IncreaseLength(NRead);
  // run codec
  for (auto *CodecPtr : PImpl->ServerCodecs) {
    ssize_t ValidLength = CodecPtr->IsComplete(*BufferObj);
    if (ValidLength > 0) {
      // dispatch a `TcpMainToWorker` async ITC event
      ssize_t Index = PImpl->Dispatcher->NextThreadIndex();
      ThreadContext *Worker = PImpl->Threads[Index];
      ItcEvent *Event = new itc::TcpMainToWorker(BufferObj, Worker);
      Worker->MainToWorkerQueue.Enqueue(Event);
      // fire an async notification
      uv_async_send(&Worker->MainToWorkerAsync);
      break;
    }
  }
}

static void OnItcAsyncSend(uv_async_t *Async) {
  ThreadContext *Worker = reinterpret_cast<ThreadContext *>(Async->data);
  int i;
  for (i = 0; i < ItcQueue::kMaxBatchCount; ++i) {
    ItcEvent *Event = Worker->WorkerToMainQueue.Dequeue();
    if (Event == nullptr) {
      break;
    }
    Event->Process();
  }
  LOG_TRACE("main thread processed %d ITC event(s) sent from #%d", i, Worker->ThreadIndex);
}

static void OnSignalInterrupt(uv_signal_t *Signal, int SigNum) {
  EngineImpl *Self = reinterpret_cast<EngineImpl *>(Signal->data);
  uv_stop(Self->UvLoop);
  LOG_ALL("SIGINT captured, main thread");
}

}  // namespace main_impl

}  // namespace wcbot
