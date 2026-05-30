#pragma once

#include <functional>
#include <memory>

#include "../Job/Job.h"
#include "../WeCom/ClientMessage.h"
#include "../WeCom/ServerMessage.h"

namespace wcbot {

// MessageCallbackJob — base class for user-supplied callback handlers.
//
// Lifetime contract:
//   * The framework hands ownership of `Request` to this job via SetRequest.
//   * User code populates `Response` via SetResponse if it wants to reply
//     synchronously; null means "no reply, use webhook for the actual push".
//   * The job's `OnStep` returns Step::kDone once it has finished — the
//     framework then cancels any unfinished children and deletes this job.
//     The parent (HttpHandlerJob) reads `Response` via GetResponse() during
//     its own OnStep(Trigger=this_callback_job) before this job is deleted.

class MessageCallbackJob : public Job {
 public:
  MessageCallbackJob();
  ~MessageCallbackJob() override;

  void SetRequest(wecom::ClientMessage *Target);
  void SetResponse(wecom::XmlServerMessage *Target);
  wecom::XmlServerMessage *GetResponse() { return Response.get(); }

  // Each user subclass implements OnStep.
  Step OnStep(Job *Trigger) override = 0;

 protected:
  // Owned. The user subclass may read it; do NOT delete it manually.
  std::unique_ptr<wecom::ClientMessage> Request;

 private:
  std::unique_ptr<wecom::XmlServerMessage> Response;
};

using FN_CreateCallbackHandlerJob = std::function<MessageCallbackJob *()>;

}  // namespace wcbot