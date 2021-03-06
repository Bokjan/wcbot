#include "wcbot/Core/Engine.h"
#include "wcbot/Utility/CronTrigger.h"

#include "Job/EchoCallbackJob.h"
#include "Job/QBJob.h"

static void RegisterQBJob();

int main(int argc, char *argv[]) {
  wcbot::Engine &Engine = wcbot::Engine::Get();
  Engine.ParseArguments(argc, argv);
  Engine.Initialize();

  RegisterQBJob();
  Engine.RegisterCallbackHandler(
      []() -> wcbot::MessageCallbackJob * { return new EchoCallbackJob(); });

  int Ret = wcbot::Engine::Get().Run();
  LOG_ALL("%d", Ret);
}

void RegisterQBJob() {
  wcbot::CronTrigger Trigger;
  Trigger.SetMonth(wcbot::CronTrigger::kEvery);
  Trigger.SetDayOfMonth(16);
  Trigger.SetHour(10);
  Trigger.SetMinute(0);
  wcbot::Engine::Get().RegisterCronJob(Trigger, []() -> wcbot::Job * { return new QBJob(); });
}
