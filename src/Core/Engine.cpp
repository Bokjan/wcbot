#include "Engine.h"
#include "EngineImpl.h"

#include <unistd.h>

#include <cstdlib>

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
  PImpl->IsFork = (argc >= 3) ? true : false;
  return true;
}

const std::string &Engine::GetCustomConfigPath() const {
  return PImpl->Config.CustomConfig;
}

int Engine::Run() {
  if (!PImpl->Config.ParseOk) {
    // todo: log
    fprintf(stderr, "Config not ok\n");
    return EXIT_FAILURE;
  }
  return PImpl->Run();
}

bool Engine::Initialize() { return PImpl->Initialize(); }

void Engine::Stop() { raise(SIGINT); }

void Engine::RegisterServerCodec(Codec *CodecPtr) {
  PImpl->ServerCodecs.push_back(CodecPtr);
}

void Engine::RegisterClientCodec(Codec *CodecPtr) {
  PImpl->ClientCodecs.push_back(CodecPtr);
}

}  // namespace wcbot
