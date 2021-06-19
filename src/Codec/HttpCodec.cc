#include "HttpCodec.h"

#include <cstdio>
#include <cstring>

#include <string>

#include "../Utility/Common.h"

namespace wcbot {

static ssize_t HttpProtocolCheck(const MemoryBuffer *Buffer) {
  char *Search =
      utility::StrNStr(Buffer->GetBase(), Buffer->GetLength(), "\r\n\r\n", sizeof("\r\n\r\n") - 1);
  if (Search == nullptr) {
    return 0;
  }
  size_t HeaderLength = Search - Buffer->GetBase() + sizeof("\r\n\r\n") - 1;
  Search = utility::StrNStr(Buffer->GetBase(), Buffer->GetLength(),
                            "Content-Length:", sizeof("Content-Length:") - 1);
  if (Search == nullptr) {
    Search = utility::StrNStr(Buffer->GetBase(), Buffer->GetLength(),
                              "content-length:", sizeof("content-length:") - 1);
  }
  if (Search == nullptr) {
    // buffer length == header length, w/o `Content-Length`?
    if (HeaderLength == Buffer->GetLength()) {
      return HeaderLength;
    } else {
      return 0;
    }
  }
  char *ContentLengthStr = Search + sizeof("Content-Length:") - 1;
  Search = utility::StrNStr(ContentLengthStr,
                            Buffer->GetLength() - (ContentLengthStr - Buffer->GetBase()), "\r\n",
                            sizeof("\r\n") - 1);
  if (Search == nullptr) {
    return 0;
  }
  size_t ContentLength;
  if (!utility::CStrToUInt64(ContentLengthStr, ContentLength)) {
    return 0;
  }
  return HeaderLength + ContentLength;
}

static std::string HttpMethodStrings[] = {"GET",     "POST",    "PUT",   "DELETE", "HEAD",
                                          "CONNECT", "OPTIONS", "TRACE", "PATCH"};

ssize_t HttpRequestCodec::IsComplete(const MemoryBuffer *Buffer) {
  bool IsHttpMethod = false;
  for (const auto &Str : HttpMethodStrings) {
    if (strncmp(Str.c_str(), Buffer->GetBase(), Str.length()) == 0) {
      IsHttpMethod = true;
      break;
    }
  }
  if (!IsHttpMethod) {
    return 0;
  }
  return HttpProtocolCheck(Buffer);
}

ssize_t HttpResponseCodec::IsComplete(const MemoryBuffer *Buffer) {
  if (strncmp("HTTP/1", Buffer->GetBase(), sizeof("HTTP/1") - 1) != 0) {
    return 0;
  }
  return HttpProtocolCheck(Buffer);
}

}  // namespace wcbot
