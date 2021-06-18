#include "HttpPackage.h"

namespace wcbot {

void HttpRequest::Reset() {
  Method = HttpMethod::kGet;
  Protocol = TransferProtocol::kUnknown;
  Path.clear();
  Path.push_back('/');
  Headers.clear();
  QueryString.clear();
  Body.clear();
}

bool HttpRequest::SetUrl(const std::string &Url) {
  this->Reset();
  bool Ret = false;
  do {
    std::string::size_type Pos;
    std::string::size_type Start = 0;
    // transfer protocol
    do {
      Pos = Url.find("http://");
      if (Pos == 0) {
        Protocol = TransferProtocol::kHttp;
        Start = sizeof("http://") - 1;
        break;
      }
      Pos = Url.find("https://");
      if (Pos == 0) {
        Protocol = TransferProtocol::kHttps;
        Start = sizeof("https://") - 1;
        break;
      }
    } while (false);
    if (Protocol == TransferProtocol::kUnknown) {
      break;
    }
    // host
    Pos = Url.find("/", Start);
    if (Pos == std::string::npos) {
      Headers["Host"] = Url.substr(Start);
      break;
    } else {
      Headers["Host"] = Url.substr(Start, Pos - Start);
    }
    // path
    Start = Pos;
    Pos = Url.find("?", Start);
    if (Pos == std::string::npos) {
      Path = Url.substr(Start);
    } else {
      Path = Url.substr(Start, Pos - Start);
    }
    // query string
    if (Pos != std::string::npos) {
      QueryString = Url.substr(Pos + 1);
    }
    // success
    Ret = true;
  } while (false);
  return Ret;
}

std::string HttpRequest::GetUrl() {
  std::string Url;
  // protocol
  switch (Protocol) {
    case TransferProtocol::kHttp:
      Url.append("http://");
      break;
    case TransferProtocol::kHttps:
      Url.append("https://");
      break;
    default:
      break;
  }
  // host
  Url.append(Headers["Host"]);
  // path
  Url.append(Path);
  // query string
  if (!QueryString.empty()) {
    Url.push_back('?');
    Url.append(QueryString);
  }
  return Url;
}

}  // namespace wcbot
