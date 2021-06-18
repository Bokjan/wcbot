#pragma once

#include "Job.h"

#include "Utility/HttpPackage.h"

namespace wcbot {

class HttpClientJob : public Job {
 public:
  union CurlPrivate {
    void* Ptr;
    uint32_t JobId;
  };

  explicit HttpClientJob(Job* Parent) : Job(Parent->Worker) {
    this->Parent = Parent;
  }

  virtual bool DoRequest(int TimeoutMS);
  virtual void Do(Job* Trigger = nullptr) override;
  virtual void OnTimeout(Job* Trigger) override;

  void DeleteThis() { delete this; }

  HttpRequest Request;
  HttpResponse Response;

 protected:
  void* CurlEasy;
};

}  // namespace wcbot
