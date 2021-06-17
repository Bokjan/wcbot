#pragma once

#include "Core/TcpMemoryBuffer.h"
#include "Job.h"

namespace wcbot {

class TcpHandlerJob : public Job {
 public:
  explicit TcpHandlerJob(ThreadContext* Worker, TcpMemoryBuffer* RB)
      : Job(Worker), ReceiveBuffer(RB) {}
  virtual ~TcpHandlerJob() { delete ReceiveBuffer; }
  static constexpr bool kDisconnect = true;
  void SendData(MemoryBuffer* Buffer, bool CloseConnection = false);

 protected:
  TcpMemoryBuffer* ReceiveBuffer;
};

}  // namespace wcbot
