#pragma once

#include <map>
#include <string>

namespace wcbot {

class MemoryBuffer;

class HttpRequest {
 public:
  enum class MethodEnum : int {
    kUnknown = 0,
    kGet = 1,  // ok to send
    kHead = 2,
    kPost = 3,  // ok to send
    kPut = 4,
    kDelete = 5,
    kConnect = 6,
    kOptions = 7,
    kTrace = 8,
    kPatch = 9
  };
  enum class ProtocolEnum : int { kUnknown, kHttp, kHttps };

  MethodEnum Method;
  ProtocolEnum Protocol;
  std::map<std::string, std::string> Headers;
  std::string Path;
  std::string QueryString;  // w/o question mark
  std::string Body;

  HttpRequest() { this->Reset(); }
  ~HttpRequest() = default;

  bool Parse(const char *Data, size_t Length);
  bool Parse(const MemoryBuffer *Data);
  bool Parse(const std::string &Data) { return Parse(Data.data(), Data.size()); }

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
