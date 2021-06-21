#pragma once

#include <map>
#include <string>
#include <vector>

#include <uv.h>

#include "../Codec/Codec.h"
#include "../Core/ITC.h"
#include "../Core/TimeWheel.h"
#include "../Job/CallbackMessageJob.h"

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
    std::string CallbackVerifyPath;
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
    std::string LogLevel;
    uint32_t WorkerThread;
  } Framework;
  std::string CustomConfig;
  BotConfig() : ParseOk(false) {}
};

class MemoryBuffer;
class ThreadContext;
class ThreadDispatcher;

class EngineImpl final {
 public:
  bool IsFork;
  uv_loop_t* UvLoop;
  uv_signal_t UvSignal_SIGINT;
  uv_signal_t UvSignal_SIGTERM;

  BotConfig Config;
  std::vector<Codec*> ServerCodecs;
  std::vector<Codec*> ClientCodecs;
  std::vector<ThreadContext*> Threads;
  ThreadDispatcher* Dispatcher;
  uint64_t TcpConnectionId;
  std::map<uint64_t, uv_tcp_t*> TcpIdToConn;

  uv_timer_t UvCronTimer;  // 1 minute
  TimeWheel CronTimeWheel;
  FN_CreateCallbackHandlerJob CbHandlerCreator;

  Tencent::WXBizMsgCrypt* Cryptor;

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
  void Finalize();
};

namespace main_impl {
extern EngineImpl* g_EImpl;
void SendTcpToClient(MemoryBuffer* Buffer, uint64_t ConnId, bool Close);
}  // namespace main_impl

}  // namespace wcbot
