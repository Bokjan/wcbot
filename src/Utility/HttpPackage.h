#pragma once

#include <map>
#include <string>

namespace wcbot {

class HttpRequest {
 public:
  enum HttpMethod : int {
    kUnknown = 0,
    kGet,
    kHead,
    kPost,
    kPut,
    kDelete,
    kConnect,
    kOptions,
    kTrace,
    kPatch
  };
  HttpMethod Method;
  std::map<std::string, std::string> Headers;
  std::string Path;
  std::string QueryString;  // w/o question mark
  std::string Body;

  HttpRequest() : Method(kUnknown) {}
};

class HttpResponse {
 public:
  int StatusCode;
  std::map<std::string, std::string> Headers;
  std::string Body;

  HttpResponse() : StatusCode(0) {}
};

}  // namespace wcbot
