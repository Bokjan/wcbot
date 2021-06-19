#include "HttpHandlerJob.h"

namespace wcbot {

void HttpHandlerJob::OnTimeout(Job* Trigger) { DeleteThis(); }

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
  // todo: parse pkg
  State = StateEnum::kDispatchRequest;
  this->Do();
}

void HttpHandlerJob::DoDispatchRequest() {
  // todo
  this->ResponseBadRequest();
  State = StateEnum::kFinish;
  this->Do();
}

void HttpHandlerJob::ResponseBadRequest() {
  MemoryBuffer *MB = MemoryBuffer::Create();
  MEMBUF_APP(MB, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
  this->SendData(MB, kDisconnect);
}

}  // namespace wcbot
