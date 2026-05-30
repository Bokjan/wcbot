#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <uv.h>

#include "../Codec/Codec.h"
#include "../Core/ITC.h"
#include "../Core/TimeWheel.h"
#include "../Job/MessageCallbackJob.h"
#include "../Utility/Logger.h"

namespace Tencent {
class WXBizMsgCrypt;
}

namespace wcbot {

struct BotConfig final {
  bool ParseOk;
  struct {
    std::string WebHookKey;
    std::string WebHookPrefix;
    std::string WebHookSend;         // internal
    std::string WebHookUploadMedia;  // internal
    std::string Token;
    std::string EncodingAesKey;
    std::string CallbackPath;
  } Bot;
  struct {
    std::string BindIpv4;
    int BindPort;
  } Http;
  struct {
    uint64_t MaxRecvBuffLength;
    uint64_t MaxSendBuffLength;
  } Network;
  struct {
    uint32_t WorkerThread;
  } Framework;
  struct {
    std::string LogLevel;
    std::string Type;
    std::string FilePath;
  } Log;
  std::string CustomConfig;
  BotConfig() : ParseOk(false) {}
};

class MemoryBuffer;
class ThreadContext;
class ThreadDispatcher;

class EngineImpl final {
 public:
  bool IsFork;
  // Owned by libuv, not us — uv_default_loop() returns a singleton.
  uv_loop_t* UvLoop;
  uv_signal_t UvSignal_SIGINT;
  uv_signal_t UvSignal_SIGTERM;

  BotConfig Config;
  // All four collections own their elements. Public access stays via
  // `obj.get()` in hot paths or transparent `->` through the unique_ptr.
  std::vector<std::unique_ptr<Codec>> ServerCodecs;
  std::vector<std::unique_ptr<Codec>> ClientCodecs;
  std::vector<std::unique_ptr<ThreadContext>> Threads;
  std::unique_ptr<ThreadDispatcher> Dispatcher;
  uint64_t TcpConnectionId;
  // libuv tcp handles live until their `uv_close` callback fires; ownership
  // is bound to the libuv close protocol and stays as raw pointers here.
  std::map<uint64_t, uv_tcp_t*> TcpIdToConn;

  uv_timer_t UvCronTimer;  // 1 minute
  TimeWheel CronTimeWheel;
  FN_CreateCallbackHandlerJob CbHandlerCreator;

  std::unique_ptr<Tencent::WXBizMsgCrypt> Cryptor;

  // Optional dynamically-allocated logger (e.g. `SyncFileLogger`). When set,
  // it is also installed as the global `logger_internal::g_Logger`. The
  // engine owns it; on destruction we restore the static stderr default
  // before letting the unique_ptr free this one to avoid a dangling global
  // pointer.
  std::unique_ptr<Logger> OwnedLogger;

  EngineImpl();
  ~EngineImpl();
  EngineImpl(const EngineImpl&) = delete;
  EngineImpl(const EngineImpl&&) = delete;

  int Run();
  bool ParseConfig(const std::string& Path);
  bool Initialize();
  uint64_t NextTcpConnectionId() { return TcpConnectionId++; }

 private:
  void RegisterGlobals();
  bool InitializeCron();
  bool InitializeCryptor();
  bool InitializeWorkerThreads();
  bool InitializeSignalHandler();
  void ActivateDaemon();
  void Finalize();
};

namespace main_impl {
extern EngineImpl* g_EImpl;
void SendTcpToClient(MemoryBuffer* Buffer, uint64_t ConnId, bool Close);
}  // namespace main_impl

}  // namespace wcbot
