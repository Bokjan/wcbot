#pragma once

#include "Job.h"

#include "Core/TcpUvBuffer.h"

namespace wcbot {

class TcpHandlerJob : public Job {
 public:
  explicit TcpHandlerJob(ThreadContext *Worker, TcpUvBufferPtr RB)
      : Job(Worker), ReceiveBuffer(RB) {}
  virtual void Do() = 0;

 protected:
  TcpUvBufferPtr ReceiveBuffer;
};

}  // namespace wcbot
