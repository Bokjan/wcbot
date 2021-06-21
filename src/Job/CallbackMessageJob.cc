#include "CallbackMessageJob.h"

namespace wcbot {

CallbackMessageJob::CallbackMessageJob() : Request(nullptr), Response(nullptr) {}

CallbackMessageJob::~CallbackMessageJob() {
  // free `Request`
  if (Request != nullptr) {
    delete Request;
  }
  // notify parent handler
  SafeParent()->Do(this);
  // free `Response`
  if (Response != nullptr) {
    delete Response;
  }
}

void CallbackMessageJob::SetRequest(wecom::ClientMessage *Target) {
  if (Request != nullptr) {
    delete Request;
  }
  Request = Target;
}

}  // namespace wcbot
