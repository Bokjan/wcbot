#pragma once

#include "../Job/Job.h"
#include "../Utility/TcpMemoryBuffer.h"

namespace wcbot {

class TcpHandlerJob : public Job {
 public:
  explicit TcpHandlerJob(TcpMemoryBuffer* RB) : Job(), ReceiveBuffer(RB) {}
  TcpHandlerJob(const TcpHandlerJob&) = delete;
  TcpHandlerJob(const TcpHandlerJob&&) = delete;
  virtual ~TcpHandlerJob();
  static constexpr bool kDisconnect = true;
  void SendData(MemoryBuffer* Buffer, bool CloseConnection = false);

 protected:
  TcpMemoryBuffer* ReceiveBuffer;
};

}  // namespace wcbot
