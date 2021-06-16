#pragma once

#include "Core/TcpMemoryBuffer.h"
#include "Job.h"

namespace wcbot {

class TcpHandlerJob : public Job {
 public:
  explicit TcpHandlerJob(ThreadContext *Worker, TcpMemoryBufferPtr RB)
      : Job(Worker), ReceiveBuffer(RB) {}
  virtual ~TcpHandlerJob() { delete ReceiveBuffer; }
  virtual void Do() = 0;
  static constexpr bool kDisconnect = true;
  void SendData(MemoryBufferPtr Buffer, bool CloseConnection = false);

 protected:
  TcpMemoryBufferPtr ReceiveBuffer;
};

}  // namespace wcbot
