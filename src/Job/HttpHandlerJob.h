#pragma once

#include "TcpHandlerJob.h"
#include "Utility/HttpPackage.h"
#include "Utility/Logger.h"

namespace wcbot {

class HttpHandlerJob final : public TcpHandlerJob {
 public:
  explicit HttpHandlerJob(ThreadContext* Worker, TcpMemoryBuffer* RB) : TcpHandlerJob(Worker, RB) {}
  virtual void Do(Job* Trigger = nullptr) override;
  virtual void OnTimeout(Job* Trigger) override;
  void DeleteThis() { delete this; }

 private:
  enum class StateEnum : int { kStart, kParseTcpPackage, kDispatchRequest, kFinish };
  StateEnum State;
  HttpRequest Request;
  void DoStart();
  void DoFinish();
  void DoParseTcpPackage();
  void DoDispatchRequest();
  void ResponseBadRequest();
};

}  // namespace wcbot
