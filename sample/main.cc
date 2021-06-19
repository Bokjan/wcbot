#include "wcbot/Core/Engine.h"

int main(int argc, char *argv[]) {
  wcbot::Engine &Engine = wcbot::Engine::Get();
  Engine.ParseArguments(argc, argv);
  Engine.Initialize();

  int Ret = wcbot::Engine::Get().Run();
  LOG_ALL("%d\n", Ret);
}
