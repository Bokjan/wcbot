#pragma once

#include <string>

#include "Codec/Codec.h"
#include "Utility/Logger.h"

namespace wcbot {

class EngineImpl;

class Engine {
 public:
  static Engine& Get();
  EngineImpl* GetImpl() { return PImpl; }
  int Run();
  bool ParseArguments(int argc, char* argv[]);
  const std::string& GetCustomConfigPath() const;
  void Stop();
  bool Initialize();

  void RegisterServerCodec(Codec* CodecPtr);
  void RegisterClientCodec(Codec* CodecPtr);

 private:
  Engine();
  ~Engine();
  Engine(const Engine&) = delete;
  Engine(const Engine&&) = delete;

  EngineImpl* PImpl;
};

}  // namespace wcbot
