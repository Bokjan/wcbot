#pragma once

#include "../Job/Job.h"

namespace wcbot {

namespace wecom {
class ServerMessage;
}

class HttpClientJob;

// `SilentPushJob` will send out the message passed to constructor
// No matter success/fail, this job will never notify parent
// `Message` is not given as a pointer, that means the job won't `free` it
// You can construct `Message` on stack safely

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
