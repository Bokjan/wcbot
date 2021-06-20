#pragma once

#include "Job.h"

#include "../Utility/HttpPackage.h"

namespace wcbot {

class HttpClientJob final : public IOJob {
 public:
  union CurlPrivate {
    void* Ptr;
    uint32_t JobId;
  };

  explicit HttpClientJob(Job* Parent);
  HttpClientJob(const HttpClientJob&) = delete;
  HttpClientJob(const HttpClientJob&&) = delete;

  virtual void Do(Job* Trigger = nullptr) override;
  virtual void OnTimeout() override;

  void DeleteThis() { delete this; }

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
