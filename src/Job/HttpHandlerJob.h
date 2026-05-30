#pragma once

#include "../Job/TcpHandlerJob.h"
#include "../Utility/HttpPackage.h"
#include "../Utility/Logger.h"

namespace wcbot {

class HttpHandlerJob final : public TcpHandlerJob {
 public:
  explicit HttpHandlerJob(TcpMemoryBuffer* RB);
  HttpHandlerJob(const HttpHandlerJob&) = delete;
  HttpHandlerJob(const HttpHandlerJob&&) = delete;

  Step OnStep(Job* Trigger) override;

 private:
  enum class StateEnum : int {
    kStart,
    kParseTcpPackage,
    kDispatchRequest,
    kVerifyCallbackSetting,
    kInvokeCallbackJobStart,
    kInvokeCallbackJobFinish,
    kFinish,
  };
  StateEnum State;
  HttpRequest Request;

  Step DoParseTcpPackage();
  Step DoDispatchRequest();
  Step DoVerifyCallbackSetting();
  Step DoInvokeCallbackJobStart();
  Step DoInvokeCallbackJobFinish(Job* Child);

  void Response200OK(const std::string& Body);
  void Response400BadRequest();
  void Response500InternalServerError();
  void Response501NotImplemented();
  void Response504GatewayTimeout();
};

}  // namespace wcbot