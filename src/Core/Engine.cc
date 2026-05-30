#include "Engine.h"

#include <cstdlib>
#include <memory>
#include <utility>

#include <unistd.h>

#include "EngineImpl.h"

namespace wcbot {

Engine &Engine::Get() {
  static Engine Instance;
  return Instance;
}

Engine::Engine() : PImpl(new EngineImpl()) {}

// Defined out-of-line so the compiler sees the full `EngineImpl` definition
// when generating the destructor for `std::unique_ptr<EngineImpl>`.
Engine::~Engine() = default;

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

void Engine::RegisterServerCodec(Codec *CodecPtr) {
  // Engine takes ownership immediately to keep public API source-compatible
  // while internally using `std::unique_ptr<Codec>` containers.
  PImpl->ServerCodecs.emplace_back(std::unique_ptr<Codec>(CodecPtr));
}

void Engine::RegisterClientCodec(Codec *CodecPtr) {
  PImpl->ClientCodecs.emplace_back(std::unique_ptr<Codec>(CodecPtr));
}

void Engine::RegisterCallbackHandler(FN_CreateCallbackHandlerJob Function) {
  PImpl->CbHandlerCreator = std::move(Function);
}

void Engine::RegisterCronJob(const CronTrigger &Trigger, FN_CreateJob Function) {
  PImpl->CronTimeWheel.AddCron(Trigger, std::move(Function));
}

}  // namespace wcbot
