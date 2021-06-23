#include "Engine.h"

#include <cstdlib>

#include <unistd.h>

#include "EngineImpl.h"

namespace wcbot {

Engine &Engine::Get() {
  static Engine Instance;
  return Instance;
}

Engine::Engine() : PImpl(new EngineImpl()) {}

Engine::~Engine() { delete PImpl; }

bool Engine::ParseArguments(int argc, char *argv[]) {
  if (argc < 2 || argc > 3) {
    return false;
  }
  if (argc >= 2) {
    if (!PImpl->ParseConfig(argv[1])) {
      return false;
    }
  }
  PImpl->IsFork = (argc >= 3) ? false : true;
  return true;
}

const std::string &Engine::GetCustomConfigPath() const { return PImpl->Config.CustomConfig; }

int Engine::Run() {
  if (!PImpl->Config.ParseOk) {
    LOG_FATAL("Config not ok");
    return EXIT_FAILURE;
  }
  return PImpl->Run();
}

bool Engine::Initialize() { return PImpl->Initialize(); }

void Engine::Stop() { raise(SIGINT); }

void Engine::RegisterServerCodec(Codec *CodecPtr) { PImpl->ServerCodecs.push_back(CodecPtr); }

void Engine::RegisterClientCodec(Codec *CodecPtr) { PImpl->ClientCodecs.push_back(CodecPtr); }

void Engine::RegisterCallbackHandler(FN_CreateCallbackHandlerJob Function) {
  PImpl->CbHandlerCreator = Function;
}

void Engine::RegisterCronJob(const CronTrigger &Trigger, FN_CreateJob Function) {
  PImpl->CronTimeWheel.AddCron(Trigger, Function);
}

}  // namespace wcbot
