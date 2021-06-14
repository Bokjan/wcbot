#include "CodecHttp.h"

#include <string>
#include <cstdio>
#include <cstring>

namespace wcbot {

static size_t HttpProtocolCheck(const UvBuffer &Buffer) {
  char *Search = strnstr(Buffer.GetBase(), "\r\n\r\n", Buffer.GetLength());
  if (Search == nullptr) {
    return 0;
  }
  size_t HeaderLength = Search - Buffer.GetBase() + sizeof("\r\n\r\n") - 1;
  Search = strnstr(Buffer.GetBase(), "Content-Length:", HeaderLength);
  if (Search == nullptr) {
    Search = strnstr(Buffer.GetBase(), "content-length:", HeaderLength);
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
  Search = strnstr(ContentLengthStr, "\r\n",
                   HeaderLength - (ContentLengthStr - Buffer.GetBase()));
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

static std::string HttpMethodStrings[] = {"GET",     "POST",  "PUT",
                                          "DELETE",  "HEAD",  "CONNECT",
                                          "OPTIONS", "TRACE", "PATCH"};

size_t CodecHttpRequest::Check(const UvBuffer &Buffer) {
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

size_t CodecHttpResponse::Check(const UvBuffer &Buffer) {
  if (strncmp("HTTP/1", Buffer.GetBase(), sizeof("HTTP/1") - 1) != 0) {
    return 0;
  }
  return HttpProtocolCheck(Buffer);
}

}  // namespace wcbot
