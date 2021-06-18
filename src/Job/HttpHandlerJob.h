#pragma once

#include "TcpHandlerJob.h"
#include "Utility/Logger.h"

#include "HttpClientJob.h"  // todo: remove

namespace wcbot {

class HttpHandlerJob : public TcpHandlerJob {
 public:
  explicit HttpHandlerJob(ThreadContext* Worker, TcpMemoryBuffer* RB) : TcpHandlerJob(Worker, RB) {}
  virtual void Do(Job* Trigger = nullptr) override {
    if (Trigger == nullptr) {
      HCJ = new HttpClientJob(this);
      HCJ->Request.SetUrl("https://www.baidu.com");
      HCJ->DoRequest(5000);
    } else {
      LOG_DEBUG("code=%d", HCJ->Response.StatusCode);
      for (const auto& KV : HCJ->Response.Headers) {
        LOG_DEBUG("%s: %s", KV.first.c_str(), KV.second.c_str());
      }
      MemoryBuffer* MemBuf = MemoryBuffer::Create();
      MEMBUF_APP(MemBuf, "HTTP/1.1 200 OK\r\nContent-Length:11\r\n\r\nhello world");
      this->SendData(MemBuf, TcpHandlerJob::kDisconnect);
      DeleteThis();
    }
  }
  virtual void OnTimeout(Job* Trigger) override { DeleteThis(); }
  void DeleteThis() { delete this; }

 protected:
  HttpClientJob* HCJ;
};

}  // namespace wcbot
