#pragma once

#include "../Job/Job.h"
#include "../Utility/HttpPackage.h"

namespace wcbot {

// Do any HTTP/HTTPS request by this job
// Set `TimeoutMS` and `Request` before invoke, 
// and get your response in `Response`

class HttpClientJob final : public IOJob {
 public:
  union CurlPrivate {
    void* Ptr;
    uint32_t JobId;
  };

  HttpClientJob();
  HttpClientJob(const HttpClientJob&) = delete;
  HttpClientJob(const HttpClientJob&&) = delete;

  virtual void Do(Job* Trigger = nullptr) override;
  virtual void OnTimeout() override;

  int TimeoutMS;
  HttpRequest Request;
  HttpResponse Response;

 private:
  enum class StateEnum : int { kCurlStart, kCurlFinish, kError };
  StateEnum State;
  void* CurlEasy;
  void DoCurlStart();
  void DoCurlFinish();
  void DoError();
};

}  // namespace wcbot
