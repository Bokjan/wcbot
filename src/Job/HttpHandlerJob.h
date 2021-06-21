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
  virtual void Do(Job* Trigger = nullptr) override;

 private:
  enum class StateEnum : int {
    kStart,
    kParseTcpPackage,
    kDispatchRequest,
    kVerifyCallbackSetting,
    kInvokeCallbackJobStart,
    kInvokeCallbackJobFinish,
    kFinish
  };
  StateEnum State;
  HttpRequest Request;
  void DoStart();
  void DoFinish();
  void DoParseTcpPackage();
  void DoDispatchRequest();
  void DoVerifyCallbackSetting();
  void DoInvokeCallbackJobStart();
  void DoInvokeCallbackJobFinish(Job* Child);
  void Response200OK(const std::string &Body);
  void Response400BadRequest();
  void Response500InternalServerError();
  void Response501NotImplemented();
  void Response504GatewayTimeout();
};

}  // namespace wcbot
