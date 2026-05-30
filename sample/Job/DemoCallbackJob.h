#pragma once

#include <cstdint>

#include <string>

#include "wcbot/Job/MessageCallbackJob.h"

namespace wcbot {
class HttpClientJob;
}

// DemoCallbackJob — a feature-rich showcase callback handler.
//
// This job demonstrates the full v2 Job protocol on top of the inbound
// WeCom text-message callback. It dispatches by leading slash command:
//
//   /help            — synchronous reply listing supported commands
//   /ping            — synchronous reply "pong"
//   /uuid            — async: spawn an HttpClientJob to fetch a UUID
//                      from https://httpbin.org/uuid, parse it from the
//                      JSON body, and return it as the synchronous reply.
//                      Demonstrates: InvokeChild + cooperative wait,
//                      HttpClientJob ArmTimeout / kErrTimeout handling,
//                      and a hand-rolled (zero-dependency) JSON extractor.
//   /sleep <ms>      — sleep for the given number of milliseconds before
//                      replying "slept Nms". If the parent HttpHandlerJob
//                      is cancelled (e.g. peer disconnect / shutdown) the
//                      framework propagates Cancel into us; OnCancel logs
//                      the event so the cancellation path is observable.
//   (anything else)  — echo the original text verbatim.
//
// Anything that's not a TextClientMessage produces no synchronous body
// (the framework will then 200-OK with an empty body, the WeCom-blessed
// way to say "I'll push the real reply later via the webhook").

class DemoCallbackJob final : public wcbot::MessageCallbackJob {
 public:
  DemoCallbackJob();
  ~DemoCallbackJob() override;

  Step OnStep(wcbot::Job* Trigger) override;
  void OnCancel() override;

 private:
  enum class StateEnum : int {
    kStart,        // first activation: classify the inbound message
    kIssueHttp,    // /uuid path: HttpClientJob has been spawned
    kAwaitSleep,   // /sleep path: framework Sleep() armed
    kFinish,       // produce reply (if any) and complete
  };

  enum class CommandEnum : int { kEcho, kHelp, kPing, kUuid, kSleep, kIgnore };

  // Decided in kStart, consumed in later states / kFinish.
  StateEnum State;
  CommandEnum Command;
  std::string Argument;       // tail after the command keyword (raw)
  std::string ReplyContent;   // body of the synchronous TextServerMessage
  uint32_t SleepMs;           // valid iff Command == kSleep

  // /uuid only — borrowed; framework owns and deletes the child after we
  // observe it once via OnStep(Trigger=Child).
  wcbot::HttpClientJob* HttpChild;

  Step DoStart(wcbot::Job* Trigger);
  Step DoIssueHttp(wcbot::Job* Trigger);
  Step DoAwaitSleep(wcbot::Job* Trigger);
  Step DoFinish();

  // Pull a top-level string field out of `Body`, e.g. ExtractJsonString(B,
  // "uuid", Out). Handles whitespace and basic backslash escapes; returns
  // false on malformed input. Intentionally minimal — sample-grade only.
  static bool ExtractJsonString(const std::string& Body, const std::string& Key,
                                std::string& Out);
};
