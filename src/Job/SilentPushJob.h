#pragma once

#include "Job.h"

namespace wcbot {

namespace wecom {
class ServerMessage;
}

class HttpClientJob;

class SilentPushJob final : public Job {
 public:
  explicit SilentPushJob(ThreadContext *Worker, const wecom::ServerMessage &Message)
      : Job(Worker), State(StateEnum::kSendReq), SendJob(nullptr), Message(&Message) {}
  SilentPushJob(const SilentPushJob&) = delete;
  SilentPushJob(const SilentPushJob&&) = delete;
  void Do(Job *Trigger = nullptr);
  void OnTimeout(Job *Trigger);
  void DeleteThis() { delete this; }

 private:
  enum class StateEnum {
    kSendReq,
    kSendRsp
  };
  StateEnum State;
  HttpClientJob *SendJob;
  const wecom::ServerMessage *Message;

  void DoSendReq();
  void DoSendRsp();
};

}  // namespace wcbot
