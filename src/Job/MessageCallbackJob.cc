#include "MessageCallbackJob.h"

namespace wcbot {

MessageCallbackJob::MessageCallbackJob() : Request(nullptr), Response(nullptr) {}

MessageCallbackJob::~MessageCallbackJob() {
  // free `Request`
  if (Request != nullptr) {
    delete Request;
  }
  // notify parent handler
  NotifyParent();
  // free `Response`
  if (Response != nullptr) {
    delete Response;
  }
}

void MessageCallbackJob::SetRequest(wecom::ClientMessage *Target) {
  if (Request != nullptr) {
    delete Request;
  }
  Request = Target;
}

}  // namespace wcbot
