#include "CodecHttp.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace wcbot {

static ssize_t HttpProtocolCheck(const UvBuffer &Buffer) {
  char *Search = strstr(Buffer.GetBase(), "\r\n\r\n");
  if (Search == nullptr) {
    return 0;
  }
  size_t HeaderLength = Search - Buffer.GetBase() + sizeof("\r\n\r\n") - 1;
  Search = strstr(Buffer.GetBase(), "Content-Length:");
  if (Search == nullptr) {
    Search = strstr(Buffer.GetBase(), "content-length:");
  }
  if (Search == nullptr) {
    // buffer length == header length, w/o `Content-Length`?
    if (HeaderLength == Buffer.GetLength()) {
      return HeaderLength;
    } else {
      return 0;
    }
  }
  char *ContentLengthStr = Search + sizeof("Content-Length:") - 1;
  Search = strstr(ContentLengthStr, "\r\n");
  if (Search == nullptr) {
    return 0;
  }
  size_t ContentLength;
  sscanf(ContentLengthStr, "%lu", &ContentLength);
  if (Buffer.GetLength() < HeaderLength + ContentLength) {
    return 0;
  }
  return HeaderLength + ContentLength;
}

static std::string HttpMethodStrings[] = {"GET",     "POST",    "PUT",   "DELETE", "HEAD",
                                          "CONNECT", "OPTIONS", "TRACE", "PATCH"};

ssize_t CodecHttpRequest::IsComplete(const UvBuffer &Buffer) {
  bool IsHttpMethod = false;
  for (const auto &Str : HttpMethodStrings) {
    if (strncmp(Str.c_str(), Buffer.GetBase(), Str.length()) == 0) {
      IsHttpMethod = true;
      break;
    }
  }
  if (!IsHttpMethod) {
    return 0;
  }
  return HttpProtocolCheck(Buffer);
}

ssize_t CodecHttpResponse::IsComplete(const UvBuffer &Buffer) {
  if (strncmp("HTTP/1", Buffer.GetBase(), sizeof("HTTP/1") - 1) != 0) {
    return 0;
  }
  return HttpProtocolCheck(Buffer);
}

}  // namespace wcbot
