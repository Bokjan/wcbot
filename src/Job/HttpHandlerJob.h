#pragma once

#include "TcpHandlerJob.h"

#include "Utility/Logger.h"

namespace wcbot {

class HttpHandlerJob : public TcpHandlerJob {
 public:
  explicit HttpHandlerJob(ThreadContext* Worker, TcpMemoryBufferPtr RB)
      : TcpHandlerJob(Worker, RB) {}
  virtual void Do() {
    MemoryBufferPtr MemBuf = new MemoryBuffer;
    MEMBUF_APP(MemBuf, "HTTP/1.1 200 OK\r\nContent-Length:11\r\n\r\nhello world");
    this->SendData(MemBuf, true);
    DeleteSelf();
  }
  void DeleteSelf() {
    delete this;
  }

 private:
};

}  // namespace wcbot
