#pragma once

#include "../Job/Job.h"

namespace wcbot {

namespace wecom {
class ServerMessage;
}

// SilentPushJob — fire a webhook send request and discard the result.
//
// The job copies nothing from `Message`; the caller must keep the
// referenced object alive only until SilentPushJob is constructed (the body
// is serialized into a child HttpClientJob during the first OnStep).

class SilentPushJob final : public Job {
 public:
  explicit SilentPushJob(const wecom::ServerMessage &Message);
  SilentPushJob(const SilentPushJob &) = delete;
  SilentPushJob(const SilentPushJob &&) = delete;

  Step OnStep(Job *Trigger) override;

 private:
  enum class StateEnum { kSendReq, kSendRsp };
  StateEnum State;
  const wecom::ServerMessage *Message;
};

}  // namespace wcbot