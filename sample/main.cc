#include <cstdio>
#include <cstdlib>

#include "wcbot/Core/Engine.h"
#include "wcbot/Utility/CronTrigger.h"

#include "Job/DemoCallbackJob.h"
#include "Job/EchoCallbackJob.h"
#include "Job/QBJob.h"

namespace {

// Process exit codes — keeps the call sites self-explanatory.
constexpr int kExitOk = EXIT_SUCCESS;
constexpr int kExitUsage = EXIT_FAILURE;
constexpr int kExitInitFailed = EXIT_FAILURE;

void PrintUsage(const char* ArgV0) {
  std::fprintf(stderr,
               "Usage: %s <config.json> [nofork]\n"
               "  - config.json: path to the JSON config file\n"
               "  - nofork: any 3rd argument disables daemonization\n",
               ArgV0);
}

// Demonstrate the cron-driven "fire and forget" path:
// every month on the 16th at 10:00 local time, post a reminder to the
// configured webhook via SilentPushJob (see Job/QBJob.cc for the body).
void RegisterCronJobs() {
  wcbot::CronTrigger Trigger;
  Trigger.SetMonth(wcbot::CronTrigger::kEvery);
  Trigger.SetDayOfMonth(16);
  Trigger.SetHour(10);
  Trigger.SetMinute(0);
  // The factory returns a fresh Job* on each fire; ownership transfers to
  // the framework, which deletes the job once it reaches kDone or is
  // cancelled. The bare `new` here mirrors that contract.
  wcbot::Engine::Get().RegisterCronJob(Trigger,
                                        []() -> wcbot::Job* { return new QBJob(); });
}

// Two ready-made callback factories. Pick exactly one to register at
// runtime via `RegisterCallbackHandlers` below.
//   * DemoCallbackJob — feature-rich showcase: command dispatch, async HTTP
//                       child job, sleep / cancel propagation. Recommended.
//   * EchoCallbackJob — minimal echo, kept around as the smallest possible
//                       MessageCallbackJob example.
//
// The factories return a fresh Job* on each inbound callback; ownership
// transfers to the framework, which deletes the job at kDone or on
// cancel. The bare `new` mirrors that contract.
wcbot::MessageCallbackJob* CreateDemoCallback() { return new DemoCallbackJob(); }
[[maybe_unused]] wcbot::MessageCallbackJob* CreateEchoCallback() {
  return new EchoCallbackJob();
}

void RegisterCallbackHandlers() {
  wcbot::Engine::Get().RegisterCallbackHandler(&CreateDemoCallback);
  // To revert to the minimal echo sample, swap the line above for:
  //   wcbot::Engine::Get().RegisterCallbackHandler(&CreateEchoCallback);
}

}  // namespace

int main(int argc, char* argv[]) {
  wcbot::Engine& Engine = wcbot::Engine::Get();

  if (!Engine.ParseArguments(argc, argv)) {
    PrintUsage(argv[0]);
    return kExitUsage;
  }
  if (!Engine.Initialize()) {
    std::fprintf(stderr, "Engine initialize failed; check log for details\n");
    return kExitInitFailed;
  }

  RegisterCronJobs();
  RegisterCallbackHandlers();

  const int Ret = Engine.Run();
  LOG_ALL("engine exited with code %d", Ret);
  return (Ret == 0) ? kExitOk : Ret;
}
