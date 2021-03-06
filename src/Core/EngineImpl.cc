#include "EngineImpl.h"

#include <cstdlib>
#include <ctime>

#include <curl/curl.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <unistd.h>

#include "../Codec/HttpCodec.h"
#include "../ThirdParty/WXBizMsgCrypt/WXBizMsgCrypt.h"
#include "../Utility/Common.h"
#include "../Utility/SyncFileLogger.h"
#include "../Utility/TcpMemoryBuffer.h"
#include "WorkerThread.h"

namespace wcbot {

namespace main_impl {
static void OnTcpRead(uv_stream_t *Handle, ssize_t NRead, const uv_buf_t *Buffer);
static void OnTcpWrite(uv_write_t *UvWriteBase, int Status);
static void OnTcpClose(uv_handle_t *Handle);
static void OnNewConnection(uv_stream_t *Handle, int Status);
static void AllocateBuffer(uv_handle_t *Handle, size_t SuggestedSize, uv_buf_t *Buffer);
static void OnItcAsyncSend(uv_async_t *Async);
static void OnSignal_Exit(uv_signal_t *Signal, int SigNum);
static void OnCronTimerTick(uv_timer_t *Timer);
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
  if (!Value.HasMember(Member) || !Value[Member].IsString()) {
    return false;
  }
  Destination = std::string(Value[Member].GetString(), Value[Member].GetStringLength());
  return true;
}

static inline bool RapidJsonGetUInt64(const rapidjson::Value &Value, const char *Member,
                                      uint64_t &Destination) {
  if (!Value.HasMember(Member) || !Value[Member].IsUint64()) {
    return false;
  }
  Destination = Value[Member].GetUint64();
  return true;
}

static inline bool RapidJsonGetUInt32(const rapidjson::Value &Value, const char *Member,
                                      uint32_t &Destination) {
  if (!Value.HasMember(Member) || !Value[Member].IsUint()) {
    return false;
  }
  Destination = Value[Member].GetUint();
  return true;
}

#define BREAK_ON_FALSE(x) \
  if (!(x)) {             \
    LOG_ERROR("%s", #x);  \
    break;                \
  }

static bool InternalParseConfig(BotConfig &Config, const rapidjson::Document &Json) {
  Config.ParseOk = false;
  do {
    // Bot
    BREAK_ON_FALSE(Json.HasMember("Bot") && Json["Bot"].IsObject());
    const rapidjson::Value &Bot = Json["Bot"];
    BREAK_ON_FALSE(RapidJsonGetString(Bot, "WebHookKey", Config.Bot.WebHookKey));
    BREAK_ON_FALSE(RapidJsonGetString(Bot, "WebHookPrefix", Config.Bot.WebHookPrefix));
    BREAK_ON_FALSE(RapidJsonGetString(Bot, "Token", Config.Bot.Token));
    BREAK_ON_FALSE(RapidJsonGetString(Bot, "EncodingAesKey", Config.Bot.EncodingAesKey));
    BREAK_ON_FALSE(RapidJsonGetString(Bot, "CallbackPath", Config.Bot.CallbackPath));
    Config.Bot.WebHookSend.assign(Config.Bot.WebHookPrefix)
        .append("/send?key=")
        .append(Config.Bot.WebHookKey);
    Config.Bot.WebHookUploadMedia.assign(Config.Bot.WebHookPrefix)
        .append("/upload_media?key=")
        .append(Config.Bot.WebHookKey)
        .append("&type=file");

    // Http
    BREAK_ON_FALSE(Json.HasMember("Http") && Json["Http"].IsObject());
    const rapidjson::Value &Http = Json["Http"];
    BREAK_ON_FALSE(RapidJsonGetString(Http, "BindIpv4", Config.Http.BindIpv4));
    if (Http.HasMember("BindPort") && Http["BindPort"].IsInt()) {
      Config.Http.BindPort = Http["BindPort"].GetInt();
    } else {
      break;
    }

    // Network
    BREAK_ON_FALSE(Json.HasMember("Network") && Json["Network"].IsObject());
    const rapidjson::Value &Network = Json["Network"];
    BREAK_ON_FALSE(
        RapidJsonGetUInt64(Network, "MaxRecvBuffLength", Config.Network.MaxRecvBuffLength));
    BREAK_ON_FALSE(
        RapidJsonGetUInt64(Network, "MaxSendBuffLength", Config.Network.MaxSendBuffLength));

    // Framework
    BREAK_ON_FALSE(Json.HasMember("Framework") && Json["Framework"].IsObject());
    const rapidjson::Value &Framework = Json["Framework"];
    BREAK_ON_FALSE(RapidJsonGetUInt32(Framework, "WorkerThread", Config.Framework.WorkerThread));

    // Log
    if (Json.HasMember("Framework") && Json["Framework"].IsObject()) {
      const rapidjson::Value &Log = Json["Log"];
      do {
        bool Check;
        Check = RapidJsonGetString(Log, "Type", Config.Log.Type);
        if (!Check) {
          break;
        }
        if (Config.Log.Type != "sync_file") {
          break;
        }
        BREAK_ON_FALSE(RapidJsonGetString(Log, "FilePath", Config.Log.FilePath));
        auto *SFL = new SyncFileLogger;
        BREAK_ON_FALSE(SFL->SetFile(Config.Log.FilePath) == 0);
        logger_internal::SetLogger(SFL);
      } while (false);
      if (RapidJsonGetString(Log, "LogLevel", Config.Log.LogLevel)) {
        BREAK_ON_FALSE(logger_internal::g_Logger->SetLevel(Config.Log.LogLevel));
      }
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
  virtual ~ThreadDispatcher() = default;
  virtual ssize_t NextThreadIndex() = 0;

 protected:
  ssize_t ThreadCount;
};

class RoundRobinThreadDispatcher final : public ThreadDispatcher {
 public:
  explicit RoundRobinThreadDispatcher(ssize_t Count) : ThreadDispatcher(Count), Current(0) {}
  ~RoundRobinThreadDispatcher() = default;
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

EngineImpl::EngineImpl()
    : IsFork(true),
      UvLoop(uv_default_loop()),
      Dispatcher(nullptr),
      TcpConnectionId(0),
      CbHandlerCreator(nullptr),
      Cryptor(nullptr) {
  main_impl::g_EImpl = this;
  srand(time(nullptr));
}

EngineImpl::~EngineImpl() { this->Finalize(); }

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
  // daemon
  if (IsFork) {
    this->ActivateDaemon();
  }
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
  // main uv loop run
  int Ret = uv_run(this->UvLoop, UV_RUN_DEFAULT);
  LOG_TRACE("uv_run ret=%d", Ret);
  // join worker threads
  for (ThreadContext *Ptr : Threads) {
    uv_thread_join(&Ptr->UvThread);
  }
  return Ret;
}

void EngineImpl::RegisterGlobals() {
  // server codec
  ServerCodecs.push_back(new HttpRequestCodec());
}

bool EngineImpl::InitializeCron() {
  uv_timer_init(UvLoop, &UvCronTimer);
  UvCronTimer.data = this;
  constexpr int kImmediately = 0;
  constexpr int kRepeat = 60 * 1000;
  uv_timer_start(&UvCronTimer, main_impl::OnCronTimerTick, kImmediately, kRepeat);
  return true;
}

bool EngineImpl::InitializeCryptor() {
  Cryptor = new Tencent::WXBizMsgCrypt(Config.Bot.Token, Config.Bot.EncodingAesKey, "");
  return true;
}

bool EngineImpl::Initialize() {
  bool Ret = false;
  do {
    this->RegisterGlobals();
    BREAK_ON_FALSE(this->InitializeCryptor());
    BREAK_ON_FALSE(this->InitializeWorkerThreads());
    BREAK_ON_FALSE(this->InitializeSignalHandler());
    this->Dispatcher = new RoundRobinThreadDispatcher(Config.Framework.WorkerThread);
    if (curl_global_init(CURL_GLOBAL_ALL)) {
      LOG_ERROR("%s", "Could not init cURL");
      break;
    }
    BREAK_ON_FALSE(this->InitializeCron());
    Ret = true;
  } while (false);
  return Ret;
}

void EngineImpl::Finalize() {
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
  if (Dispatcher != nullptr) {
    delete Dispatcher;
  }
  // cryptor
  if (Cryptor != nullptr) {
    delete Cryptor;
  }
  // logger
  if (dynamic_cast<SyncFileLogger *>(logger_internal::g_Logger) != nullptr) {
    delete dynamic_cast<SyncFileLogger *>(logger_internal::g_Logger);
  }
}

bool EngineImpl::InitializeWorkerThreads() {
  for (uint32_t i = 0; i < Config.Framework.WorkerThread; ++i) {
    ThreadContext *Worker = new ThreadContext();
    Worker->ThreadIndex = i;
    Worker->EImpl = this;
    uv_async_init(this->UvLoop, &Worker->WorkerToMainAsync, main_impl::OnItcAsyncSend);
    Worker->WorkerToMainAsync.data = Worker;
    Threads.push_back(Worker);
  }
  return true;
}

bool EngineImpl::InitializeSignalHandler() {
  // SIGINT
  uv_signal_init(this->UvLoop, &this->UvSignal_SIGINT);
  this->UvSignal_SIGINT.data = this;
  uv_signal_start(&this->UvSignal_SIGINT, main_impl::OnSignal_Exit, SIGINT);
  // SIGTERM
  uv_signal_init(this->UvLoop, &this->UvSignal_SIGTERM);
  this->UvSignal_SIGTERM.data = this;
  uv_signal_start(&this->UvSignal_SIGTERM, main_impl::OnSignal_Exit, SIGTERM);
  return true;
}

void EngineImpl::ActivateDaemon() {
  auto PID = fork();
  if (PID == -1) {
    LOG_ERROR("fork() failed");
    exit(EXIT_FAILURE);
    return;
  }
  // parent exits
  if (PID != 0) {
    exit(EXIT_SUCCESS);
  }
  // setsid()
  auto Ret = setsid();
  if (Ret == -1) {
    LOG_ERROR("setsid() returns %d", Ret);
    exit(EXIT_FAILURE);
  }
}

namespace main_impl {

EngineImpl *g_EImpl = nullptr;

static void OnNewConnection(uv_stream_t *Tcp, int Status) {
  LOG_TRACE("enter, status=%d", Status);
  if (Status < 0) {
    LOG_ERROR("TCP new connection error, status=%d", Status);
    return;
  }
  EngineImpl *Impl = reinterpret_cast<EngineImpl *>(Tcp->data);
  uint64_t ConnId = Impl->NextTcpConnectionId();
  uv_tcp_t *ClientTcp = new uv_tcp_t;
  Impl->TcpIdToConn[ConnId] = ClientTcp;
  TcpMemoryBuffer *Buffer = new TcpMemoryBuffer(ConnId, reinterpret_cast<uv_tcp_t *>(Tcp));
  uv_tcp_init(Impl->UvLoop, ClientTcp);
  ClientTcp->data = Buffer;
  if (uv_accept(Tcp, reinterpret_cast<uv_stream_t *>(ClientTcp)) == 0) {
    uv_read_start(reinterpret_cast<uv_stream_t *>(ClientTcp), main_impl::AllocateBuffer,
                  main_impl::OnTcpRead);
  } else {
    uv_close(reinterpret_cast<uv_handle_t *>(ClientTcp), main_impl::OnTcpClose);
  }
}

static void OnTcpClose(uv_handle_t *Handle) {
  LOG_TRACE("enter");
  TcpMemoryBuffer *TcpBuf = reinterpret_cast<TcpMemoryBuffer *>(Handle->data);
  EngineImpl *PImpl = reinterpret_cast<EngineImpl *>(TcpBuf->ServerTcp->data);
  do {
    auto FindConn = PImpl->TcpIdToConn.find(TcpBuf->ClientTcpId);
    if (FindConn == PImpl->TcpIdToConn.end()) {
      break;
    }
    PImpl->TcpIdToConn.erase(FindConn);
  } while (false);
  delete TcpBuf;
  delete reinterpret_cast<uv_tcp_t *>(Handle);
}

static void AllocateBuffer(uv_handle_t *Handle, size_t SuggestedSize, uv_buf_t *Buffer) {
  // LOG_TRACE("enter");
  TcpMemoryBuffer *MemBuf = reinterpret_cast<TcpMemoryBuffer *>(Handle->data);
  MemBuf->Allocate(SuggestedSize);
  Buffer->base = MemBuf->GetCurrent();
  Buffer->len = SuggestedSize;
}

static void OnTcpRead(uv_stream_t *Handle, ssize_t NRead, const uv_buf_t *Buffer) {
  LOG_TRACE("enter, nread=%ld", NRead);
  TcpMemoryBuffer *MemBuf = reinterpret_cast<TcpMemoryBuffer *>(Handle->data);
  EngineImpl *PImpl = reinterpret_cast<EngineImpl *>(MemBuf->ServerTcp->data);
  // currently used buffer size is larger than configured value
  if (MemBuf->GetCapacity() > PImpl->Config.Network.MaxRecvBuffLength) {
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
  MemBuf->IncreaseLength(NRead);
  // run codec
  for (auto *CodecPtr : PImpl->ServerCodecs) {
    ssize_t ValidLength = CodecPtr->IsComplete(MemBuf);
    if (ValidLength > 0) {
      // create a new buf for this conn, to receive more pkgs
      TcpMemoryBuffer *NewBuf = new TcpMemoryBuffer(MemBuf->ClientTcpId, MemBuf->ServerTcp);
      NewBuf->Append(MemBuf->GetBase() + ValidLength, MemBuf->GetLength() - ValidLength);
      Handle->data = NewBuf;
      // dispatch a `TcpMainToWorker` async ITC event
      ssize_t Index = PImpl->Dispatcher->NextThreadIndex();
      ThreadContext *Worker = PImpl->Threads[Index];
      ItcEvent *Event = new itc::TcpMainToWorker(MemBuf);
      Worker->MainToWorkerQueue.Enqueue(Event);
      // fire an async notification
      Worker->NotifyWorker();
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
    Event->Process();  // event frees self on finish
  }
  LOG_TRACE("main thread processed %d ITC event(s) sent from #%d", i, Worker->ThreadIndex);
}

static void OnSignal_Exit(uv_signal_t *Signal, int SigNum) {
  EngineImpl *EImpl = reinterpret_cast<EngineImpl *>(Signal->data);
  uv_stop(EImpl->UvLoop);
  LOG_ALL("Signal %d captured, main thread", SigNum);
}

struct UvWriteRequest {
  uv_write_t UvWrite;
  uv_buf_t UvBuffer;
};

void SendTcpToClient(MemoryBuffer *Buffer, uint64_t ConnId, bool Close) {
  LOG_TRACE("enter");
  auto ConnPair = main_impl::g_EImpl->TcpIdToConn.find(ConnId);
  if (ConnPair == main_impl::g_EImpl->TcpIdToConn.end()) {
    LOG_INFO("conn=%lu already closed", ConnId);
    delete Buffer;
    return;
  }
  UvWriteRequest *UvWrite = new UvWriteRequest;
  UvWrite->UvWrite.data = Buffer;
  UvWrite->UvBuffer.base = Buffer->GetBase();
  UvWrite->UvBuffer.len = Buffer->GetLength();
  uv_write(reinterpret_cast<uv_write_t *>(UvWrite),
           reinterpret_cast<uv_stream_t *>(ConnPair->second), &UvWrite->UvBuffer, 1,
           main_impl::OnTcpWrite);
  if (Close) {
    uv_close(reinterpret_cast<uv_handle_t *>(ConnPair->second), main_impl::OnTcpClose);
  }
}

static void OnTcpWrite(uv_write_t *UvWriteBase, int Status) {
  LOG_TRACE("enter, status=%d", Status);
  UvWriteRequest *UvWrite = reinterpret_cast<UvWriteRequest *>(UvWriteBase);
  if (Status < 0) {
    LOG_ERROR("uv_write error, status=%d, desc=%s", Status, uv_err_name(Status));
  }
  MemoryBuffer *MemBuf = reinterpret_cast<MemoryBuffer *>(UvWrite->UvWrite.data);
  delete MemBuf;
  delete UvWrite;
}

static void TimeWheelTickImpl(FN_CreateJob Function, void *UserData) {
  auto EImpl = reinterpret_cast<EngineImpl *>(UserData);
  // dispatch a `JobCreateAndRun` async ITC event
  ssize_t Index = EImpl->Dispatcher->NextThreadIndex();
  ThreadContext *Worker = EImpl->Threads[Index];
  ItcEvent *Event = new itc::JobCreateAndRun(Function);
  Worker->MainToWorkerQueue.Enqueue(Event);
  // fire an async notification
  Worker->NotifyWorker();
}

static void OnCronTimerTick(uv_timer_t *Timer) {
  struct timeval TimeVal;
  gettimeofday(&TimeVal, nullptr);
  struct tm TM = *(localtime(&TimeVal.tv_sec));
  auto Now = mktime(&TM);
  TM.tm_min += 1;
  TM.tm_sec = 0;
  auto Next = mktime(&TM);
  uv_timer_stop(Timer);
  constexpr auto UsecInASec = 1000000;
  int64_t DeltaMS = (UsecInASec - TimeVal.tv_usec) / 1000;
  DeltaMS += (Next - Now - 1) * 1000;
  uv_timer_start(Timer, OnCronTimerTick, DeltaMS, 0);
  LOG_TRACE("now=%ld, next=%ld, deltams=%ld", Now, Next, DeltaMS);
  auto EImpl = reinterpret_cast<EngineImpl *>(Timer->data);
  EImpl->CronTimeWheel.Tick(TimeWheelTickImpl, EImpl);
}

}  // namespace main_impl

}  // namespace wcbot
