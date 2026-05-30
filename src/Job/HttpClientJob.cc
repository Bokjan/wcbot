#include "HttpClientJob.h"

#include <curl/curl.h>

#include "../Core/WorkerThread.h"
#include "../Utility/Logger.h"

namespace wcbot {

namespace http_client_impl {
size_t WriteFunction(char *Ptr, size_t Size, size_t NMemB, void *UserData) {
  HttpClientJob *Job = reinterpret_cast<HttpClientJob *>(UserData);
  auto Total = Size * NMemB;
  Job->Response.Body.append(Ptr, Total);
  return Total;
}
size_t HeaderFunction(char *Ptr, size_t Size, size_t NItems, void *UserData) {
  HttpClientJob *Job = reinterpret_cast<HttpClientJob *>(UserData);
  auto Total = Size * NItems;
  do {
    // extract key
    size_t KeyStart, KeyEnd;
    for (KeyStart = 0; KeyStart < Total; ++KeyStart) {
      if (Ptr[KeyStart] != ' ') {
        break;
      }
    }
    if (KeyStart == Total) {
      break;
    }
    for (KeyEnd = KeyStart + 1; KeyEnd < Total; ++KeyEnd) {
      if (Ptr[KeyEnd] == ' ' || Ptr[KeyEnd] == ':') {
        --KeyEnd;
        break;
      }
    }
    if (KeyEnd == Total) {
      break;
    }
    // find colon
    size_t Colon;
    for (Colon = KeyEnd + 1; Colon < Total; ++Colon) {
      if (Ptr[Colon] == ':') {
        break;
      }
    }
    if (Colon == Total) {
      break;
    }
    // extract value
    size_t ValueStart;
    for (ValueStart = Colon + 1; ValueStart < Total; ++ValueStart) {
      if (Ptr[ValueStart] != ' ') {
        break;
      }
    }
    // insert
    Job->Response.Headers[std::string(Ptr + KeyStart, KeyEnd - KeyStart + 1)] =
        std::string(Ptr + ValueStart, Total - ValueStart - 2);
  } while (false);
  return Total;
}
}  // namespace http_client_impl

HttpClientJob::HttpClientJob()
    : IOJob(),
      TimeoutMS(1000),
      State(StateEnum::kIssue),
      CurlEasy(nullptr),
      CurlAttached(false) {}

Job::Step HttpClientJob::OnStep(Job *Trigger) {
  switch (State) {
    case StateEnum::kIssue:
      return DoIssue();

    case StateEnum::kAwaitResult:
      // Reached via either:
      //   * cURL completion: `WorkerThread::CurlMultiStatusCheck` will detach
      //     and cleanup the easy handle right after this driver call returns.
      //   * Timeout from DealDelayQueue: ErrCode == kErrTimeout. The easy
      //     handle is still attached; cURL's own CURLOPT_TIMEOUT_MS will
      //     produce a CURLMSG_DONE shortly after, at which point the worker
      //     thread cleanup path runs (DQueue.Remove returns null since we
      //     already left the queue, so the cleanup is the only side effect).
      // In neither case do we touch the easy handle here, to avoid a double
      // cleanup with `WorkerThread::CurlMultiStatusCheck`.
      CurlEasy = nullptr;
      CurlAttached = false;
      return Step::kDone;

    case StateEnum::kError:
      return Step::kDone;
  }
  return Step::kDone;
}

void HttpClientJob::OnCancel() {
  // Parent finished early — release the cURL easy handle if we still own it.
  if (CurlAttached && CurlEasy != nullptr) {
    curl_multi_remove_handle(worker_impl::g_ThisThread->CurlMultiHandle, CurlEasy);
    curl_easy_cleanup(CurlEasy);
  }
  CurlEasy = nullptr;
  CurlAttached = false;
}

Job::Step HttpClientJob::DoIssue() {
  CurlEasy = curl_easy_init();
  if (CurlEasy == nullptr) {
    State = StateEnum::kError;
    return Step::kContinue;
  }
  curl_easy_setopt(CurlEasy, CURLOPT_TIMEOUT_MS, static_cast<long>(TimeoutMS));
  curl_easy_setopt(CurlEasy, CURLOPT_HEADERDATA, this);
  curl_easy_setopt(CurlEasy, CURLOPT_HEADERFUNCTION, http_client_impl::HeaderFunction);
  curl_easy_setopt(CurlEasy, CURLOPT_WRITEDATA, this);
  curl_easy_setopt(CurlEasy, CURLOPT_WRITEFUNCTION, http_client_impl::WriteFunction);
  curl_easy_setopt(CurlEasy, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(CurlEasy, CURLOPT_URL, Request.GetUrl().c_str());
  switch (Request.Method) {
    case HttpRequest::MethodEnum::kGet:
      curl_easy_setopt(CurlEasy, CURLOPT_HTTPGET, 1L);
      break;
    case HttpRequest::MethodEnum::kPost: {
      curl_easy_setopt(CurlEasy, CURLOPT_POST, 1L);
      curl_easy_setopt(CurlEasy, CURLOPT_POSTFIELDS, Request.Body.data());
      curl_easy_setopt(CurlEasy, CURLOPT_POSTFIELDSIZE, Request.Body.size());
      // avoid cURL's default `Content-Type: application/x-www-form-urlencoded`
      auto MapIt = Request.Headers.find("Content-Type");
      if (MapIt == Request.Headers.end()) {
        Request.Headers.insert(std::make_pair("Content-Type", ""));
      }
      break;
    }
    default:
      break;
  }
  curl_slist *CurlHeaderList = nullptr;
  for (const auto &PV : Request.Headers) {
    thread_local std::string Line;
    Line.clear();
    Line.append(PV.first).append(": ").append(PV.second);
    CurlHeaderList = curl_slist_append(CurlHeaderList, Line.c_str());
  }
  curl_easy_setopt(CurlEasy, CURLOPT_HTTPHEADER, CurlHeaderList);
  this->ArmTimeout(this->TimeoutMS);
  CurlPrivate PrivateUnion;
  PrivateUnion.JobId = GetJobId();
  curl_easy_setopt(CurlEasy, CURLOPT_PRIVATE, PrivateUnion.Ptr);
  curl_multi_add_handle(worker_impl::g_ThisThread->CurlMultiHandle, CurlEasy);
  CurlAttached = true;
  State = StateEnum::kAwaitResult;
  return Step::kWaiting;
}

}  // namespace wcbot