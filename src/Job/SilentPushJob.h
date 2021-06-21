#pragma once

#include "../Job/Job.h"

namespace wcbot {

namespace wecom {
class ServerMessage;
}

class HttpClientJob;

class SilentPushJob final : public Job {
 public:
  explicit SilentPushJob(const wecom::ServerMessage &Message);
  SilentPushJob(const SilentPushJob &) = delete;
  SilentPushJob(const SilentPushJob &&) = delete;
  void Do(Job *Trigger = nullptr);

 private:
  enum class StateEnum { kSendReq, kSendRsp };
  StateEnum State;
  const wecom::ServerMessage *Message;

  void DoSendReq();
  void DoSendRsp(Job *Rsp);
};

}  // namespace wcbot
