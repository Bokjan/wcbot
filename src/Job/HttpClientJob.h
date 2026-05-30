#pragma once

#include "../Job/Job.h"
#include "../Utility/HttpPackage.h"

namespace wcbot {

// HttpClientJob — perform an HTTP/HTTPS request via the worker's cURL multi.
//
// Usage (typical parent):
//   case kIssueRequest:
//     auto* C = new HttpClientJob();
//     C->Request = ...; C->TimeoutMS = ...;
//     InvokeChild(C);
//     State = kCollectResponse;
//     return Step::kWaiting;
//   case kCollectResponse:
//     if (auto* C = AsJob<HttpClientJob>(Trigger)) {
//       if (C->ErrCode == kErrTimeout) { ... }
//       else { use C->Response; }
//     }
//     return Step::kContinue;
//
// On cancellation (parent finished early), the framework calls OnCancel which
// detaches the easy handle from the multi and cleans it up if necessary.

class HttpClientJob final : public IOJob {
 public:
  union CurlPrivate {
    void* Ptr;
    uint32_t JobId;
  };

  HttpClientJob();
  HttpClientJob(const HttpClientJob&) = delete;
  HttpClientJob(const HttpClientJob&&) = delete;

  Step OnStep(Job* Trigger) override;
  void OnCancel() override;

  int TimeoutMS;
  HttpRequest Request;
  HttpResponse Response;

 private:
  enum class StateEnum : int { kIssue, kAwaitResult, kError };
  StateEnum State;
  void* CurlEasy;
  bool CurlAttached;  // easy handle is currently inside the multi

  Step DoIssue();
};

}  // namespace wcbot