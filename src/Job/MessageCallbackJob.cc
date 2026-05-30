#include "MessageCallbackJob.h"

namespace wcbot {

MessageCallbackJob::MessageCallbackJob() = default;

MessageCallbackJob::~MessageCallbackJob() = default;

void MessageCallbackJob::SetRequest(wecom::ClientMessage *Target) {
  Request.reset(Target);
}

void MessageCallbackJob::SetResponse(wecom::XmlServerMessage *Target) {
  Response.reset(Target);
}

}  // namespace wcbot