#include "wcbot/Core/Engine.h"
#include "wcbot/Utility/CronTrigger.h"

#include "Job/QBJob.h"

static void RegisterQBJob();

int main(int argc, char *argv[]) {
  wcbot::Engine &Engine = wcbot::Engine::Get();
  Engine.ParseArguments(argc, argv);
  Engine.Initialize();

  RegisterQBJob();

  int Ret = wcbot::Engine::Get().Run();
  LOG_ALL("%d\n", Ret);
}

void RegisterQBJob() {
  wcbot::CronTrigger Trigger;
  Trigger.SetMonth(wcbot::CronTrigger::kEvery);
  Trigger.SetDayOfMonth(16);
  Trigger.SetHour(10);
  Trigger.SetMinute(0);
  wcbot::Engine::Get().RegisterCronJob(
      Trigger, [](wcbot::ThreadContext *Worker) -> wcbot::Job * { return new QBJob(Worker); });
}
