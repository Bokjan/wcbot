#pragma once

#include "../Utility/HttpPackage.h"
#include "../Utility/Logger.h"
#include "TcpHandlerJob.h"

namespace wcbot {

class HttpHandlerJob final : public TcpHandlerJob {
 public:
  explicit HttpHandlerJob(TcpMemoryBuffer* RB);
  HttpHandlerJob(const HttpHandlerJob&) = delete;
  HttpHandlerJob(const HttpHandlerJob&&) = delete;
  virtual void Do(Job* Trigger = nullptr) override;
  virtual void OnTimeout(Job* Trigger) override;
  void DeleteThis() { delete this; }

 private:
  enum class StateEnum : int {
    kStart,
    kParseTcpPackage,
    kDispatchRequest,
    kVerifyCallbackSetting,
    kFinish
  };
  StateEnum State;
  HttpRequest Request;
  void DoStart();
  void DoFinish();
  void DoParseTcpPackage();
  void DoDispatchRequest();
  void DoVerifyCallbackSetting();
  void Response400BadRequest();
  void Response504GatewayTimeout();
  void ResponseVerifyEchoString(const std::string &Body);
};

}  // namespace wcbot
