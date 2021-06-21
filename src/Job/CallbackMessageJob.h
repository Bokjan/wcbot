#pragma once

#include "../Job/Job.h"
#include "../WeCom/ClientMessage.h"
#include "../WeCom/ServerMessage.h"

namespace wcbot {

class CallbackMessageJob : public Job {
 public:
  CallbackMessageJob();
  ~CallbackMessageJob();
  void SetRequest(wecom::ClientMessage *Target);
  wecom::XmlServerMessage *GetResponse() { return Response; }
  void SetResponse(wecom::XmlServerMessage *Target) { Response = Target; }
  virtual void Do(Job *Trigger = nullptr) = 0;

 protected:
  wecom::ClientMessage *Request;

 private:
  wecom::XmlServerMessage *Response;
};

using FN_CreateCallbackHandlerJob = CallbackMessageJob *(*)();

}  // namespace wcbot
