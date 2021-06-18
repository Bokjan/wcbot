#pragma once

#include <map>
#include <string>

namespace wcbot {

class HttpRequest {
 public:
  enum class HttpMethod : int {
    kUnknown = 0,
    kGet = 1,
    // kHead = 2,
    kPost = 3,
    // kPut = 4,
    // kDelete = 5,
    // kConnect = 6,
    // kOptions = 7,
    // kTrace = 8,
    // kPatch = 9
  };
  enum class TransferProtocol : int { kUnknown, kHttp, kHttps };

  HttpMethod Method;
  TransferProtocol Protocol;
  std::map<std::string, std::string> Headers;
  std::string Path;
  std::string QueryString;  // w/o question mark
  std::string Body;

  HttpRequest() { this->Reset(); }
  ~HttpRequest() = default;

  void Reset();
  bool SetUrl(const std::string &Url);
  std::string GetUrl();
};

class HttpResponse {
 public:
  int StatusCode;
  std::map<std::string, std::string> Headers;
  std::string Body;

  HttpResponse() : StatusCode(0) {}
};

}  // namespace wcbot
