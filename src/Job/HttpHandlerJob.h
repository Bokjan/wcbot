#pragma once

#include "TcpHandlerJob.h"

namespace wcbot {

class HttpHandlerJob : public TcpHandlerJob {
 public:
  explicit HttpHandlerJob(ThreadContext* Worker, TcpUvBufferPtr RB) : TcpHandlerJob(Worker, RB) {}
  virtual void Do() {
    char* Data = ReceiveBuffer->GetBase();
    Data[ReceiveBuffer->GetLength()] = '\0';
    LOG_DEBUG("%s", Data);
  }

 private:
};

}  // namespace wcbot
