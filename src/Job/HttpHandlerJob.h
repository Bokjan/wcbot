#pragma once

#include "TcpHandlerJob.h"
#include "Utility/Logger.h"

namespace wcbot {

class HttpHandlerJob : public TcpHandlerJob {
 public:
  explicit HttpHandlerJob(ThreadContext* Worker, TcpMemoryBuffer* RB) : TcpHandlerJob(Worker, RB) {}
  virtual void Do() {
    MemoryBuffer* MemBuf = new MemoryBuffer;
    MEMBUF_APP(MemBuf, "HTTP/1.1 200 OK\r\nContent-Length:11\r\n\r\nhello world");
    this->SendData(MemBuf, TcpHandlerJob::kDisconnect);
    DeleteThis();
  }
  void DeleteThis() { delete this; }

 private:
};

}  // namespace wcbot
