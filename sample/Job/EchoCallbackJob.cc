#include "EchoCallbackJob.h"

#include <memory>

#include "wcbot/WeCom/TextClientMessage.h"
#include "wcbot/WeCom/TextServerMessage.h"

EchoCallbackJob::Step EchoCallbackJob::OnStep(wcbot::Job* /*Trigger*/) {
  // Echo back the inbound text message verbatim.
  if (auto* TCM = dynamic_cast<wcbot::wecom::TextClientMessage*>(Request.get())) {
    auto TSM = std::unique_ptr<wcbot::wecom::TextServerMessage>(new wcbot::wecom::TextServerMessage());
    TSM->Content = TCM->Content;
    SetResponse(std::move(TSM));
  }
  return Step::kDone;
}