#include <cstdio>
#include <cstdlib>

#include "wcbot/Core/Engine.h"
#include "wcbot/Utility/CronTrigger.h"

#include "Job/EchoCallbackJob.h"
#include "Job/QBJob.h"

static void RegisterQBJob();

int main(int argc, char *argv[]) {
  wcbot::Engine &Engine = wcbot::Engine::Get();
  if (!Engine.ParseArguments(argc, argv)) {
    fprintf(stderr,
            "Usage: %s <config.json> [nofork]\n"
            "  - config.json: path to the JSON config file\n"
            "  - nofork: any 3rd argument disables daemonization\n",
            argv[0]);
    return EXIT_FAILURE;
  }
  if (!Engine.Initialize()) {
    fprintf(stderr, "Engine initialize failed; check log for details\n");
    return EXIT_FAILURE;
  }

  RegisterQBJob();
  Engine.RegisterCallbackHandler(
      []() -> wcbot::MessageCallbackJob * { return new EchoCallbackJob(); });

  int Ret = wcbot::Engine::Get().Run();
  LOG_ALL("%d", Ret);
  return Ret;
}

void RegisterQBJob() {
  wcbot::CronTrigger Trigger;
  Trigger.SetMonth(wcbot::CronTrigger::kEvery);
  Trigger.SetDayOfMonth(16);
  Trigger.SetHour(10);
  Trigger.SetMinute(0);
  wcbot::Engine::Get().RegisterCronJob(Trigger, []() -> wcbot::Job * { return new QBJob(); });
}
