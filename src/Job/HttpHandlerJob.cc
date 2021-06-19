#include "HttpHandlerJob.h"

#include "SilentPushJob.h"
#include "../WeCom/TextServerMessage.h"

namespace wcbot {

HttpHandlerJob::HttpHandlerJob(ThreadContext* Worker, TcpMemoryBuffer* RB)
    : TcpHandlerJob(Worker, RB), State(StateEnum::kStart) {}

void HttpHandlerJob::OnTimeout(Job* Trigger) {
  this->Response504GatewayTimeout();
  DeleteThis();
}

void HttpHandlerJob::Do(Job* Trigger) {
  switch (State) {
    case StateEnum::kStart:
      this->DoStart();
      break;
    case StateEnum::kParseTcpPackage:
      this->DoParseTcpPackage();
      break;
    case StateEnum::kDispatchRequest:
      this->DoDispatchRequest();
      break;
    case StateEnum::kFinish:
      this->DoFinish();
      break;
    default:
      break;
  }
}

void HttpHandlerJob::DoStart() {
  State = StateEnum::kParseTcpPackage;
  this->Do();
}

void HttpHandlerJob::DoFinish() { DeleteThis(); }

void HttpHandlerJob::DoParseTcpPackage() {
  bool Success = Request.Parse(ReceiveBuffer->GetBase(), ReceiveBuffer->GetLength());
  if (!Success) {
    this->Response400BadRequest();
    State = StateEnum::kFinish;
  } else {
    State = StateEnum::kDispatchRequest;
  }
  this->Do();
}

void HttpHandlerJob::DoDispatchRequest() {
  // todo
  wecom::TextServerMessage TSM;
  TSM.Content = "hello world from C++";
  auto J = new SilentPushJob(this->Worker, TSM);
  J->Do();
  this->Response400BadRequest();
  State = StateEnum::kFinish;
  this->Do();
}

void HttpHandlerJob::Response400BadRequest() {
  MemoryBuffer* MB = MemoryBuffer::Create();
  MEMBUF_APP(MB, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
  this->SendData(MB, kDisconnect);
}

void HttpHandlerJob::Response504GatewayTimeout() {
  MemoryBuffer* MB = MemoryBuffer::Create();
  MEMBUF_APP(MB, "HTTP/1.1 504 Gateway Timeout\r\nContent-Length: 0\r\n\r\n");
  this->SendData(MB, kDisconnect);
}

}  // namespace wcbot
