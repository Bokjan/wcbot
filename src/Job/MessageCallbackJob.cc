#include "MessageCallbackJob.h"

namespace wcbot {

MessageCallbackJob::MessageCallbackJob() = default;

MessageCallbackJob::~MessageCallbackJob() = default;

void MessageCallbackJob::SetRequest(std::unique_ptr<wecom::ClientMessage> Target) {
  Request = std::move(Target);
}

void MessageCallbackJob::SetResponse(std::unique_ptr<wecom::XmlServerMessage> Target) {
  Response = std::move(Target);
}

}  // namespace wcbot