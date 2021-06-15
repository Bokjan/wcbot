#pragma once

#include <uv.h>

#include <cstdint>
#include <string>
#include <vector>

#include "Codec/Codec.h"
#include "ITC.h"

namespace wcbot {

struct BotConfig {
  bool ParseOk;
  struct {
    std::string WebHook;
    std::string Token;
    std::string EncodingAesKey;
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
  std::string CustomConfig;
  BotConfig() : ParseOk(false) {}
};

class ThreadContext;

class EngineImpl {
 public:
  bool IsFork;
  bool StopSign;
  uv_loop_t* UvMainLoop;

  BotConfig Config;
  std::vector<Codec*> ServerCodecs;
  std::vector<Codec*> ClientCodecs;
  std::vector<ThreadContext*> Threads;

  EngineImpl();
  ~EngineImpl();

  int Run();
  bool ParseConfig(const std::string& Path);
  bool Initialize();
  bool InitializeInterThreadCommunication();
};

}  // namespace wcbot
