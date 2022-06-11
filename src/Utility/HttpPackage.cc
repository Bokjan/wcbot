#include "HttpPackage.h"

#include "../Utility/Common.h"
#include "../Utility/Logger.h"
#include "../Utility/MemoryBuffer.h"

namespace wcbot {

void HttpRequest::Reset() {
  Method = MethodEnum::kUnknown;
  Protocol = ProtocolEnum::kUnknown;
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
        Protocol = ProtocolEnum::kHttp;
        Start = sizeof("http://") - 1;
        break;
      }
      Pos = Url.find("https://");
      if (Pos == 0) {
        Protocol = ProtocolEnum::kHttps;
        Start = sizeof("https://") - 1;
        break;
      }
    } while (false);
    if (Protocol == ProtocolEnum::kUnknown) {
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
    case ProtocolEnum::kHttp:
      Url.append("http://");
      break;
    case ProtocolEnum::kHttps:
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

bool HttpRequest::Parse(const MemoryBuffer *Data) {
  return Parse(Data->GetBase(), Data->GetLength());
}

bool HttpRequest::Parse(const char *Data, size_t Length) {
  this->Reset();
  thread_local std::string StrBuf;
  StrBuf.clear();
  char *BodyPtr = nullptr;
  char *Search = nullptr;
  char *Current = nullptr;
  // request line
  do {
    Search = utility::StrNStr(Data, Length, "\r\n", sizeof("\r\n") - 1);
    if (Search == nullptr) {
      return false;
    }
    StrBuf.assign(Data + 0, Search - (Data + 0));
    Method = MethodEnum::kUnknown;
    if (StrBuf.find("GET ") == 0) {
      Method = MethodEnum::kGet;
    } else if (StrBuf.find("HEAD ") == 0) {
      Method = MethodEnum::kHead;
    } else if (StrBuf.find("POST ") == 0) {
      Method = MethodEnum::kPost;
    } else if (StrBuf.find("PUT ") == 0) {
      Method = MethodEnum::kPut;
    } else if (StrBuf.find("DELETE ") == 0) {
      Method = MethodEnum::kDelete;
    } else if (StrBuf.find("CONNECT ") == 0) {
      Method = MethodEnum::kConnect;
    } else if (StrBuf.find("OPTIONS ") == 0) {
      Method = MethodEnum::kOptions;
    } else if (StrBuf.find("TRACE ") == 0) {
      Method = MethodEnum::kTrace;
    } else if (StrBuf.find("PATCH ") == 0) {
      Method = MethodEnum::kPatch;
    }
    if (Method == MethodEnum::kUnknown) {
      return false;
    }
    StrBuf = StrBuf.substr(StrBuf.find(' ') + 1);
    auto SecondSpacePos = StrBuf.find(' ');
    if (SecondSpacePos == std::string::npos) {
      return false;
    }
    StrBuf = StrBuf.substr(0, SecondSpacePos);
    auto QuestionMarkPosition = StrBuf.find('?');
    if (QuestionMarkPosition == std::string::npos) {
      Path = StrBuf;
    } else {
      Path = StrBuf.substr(0, QuestionMarkPosition);
      QueryString = StrBuf.substr(QuestionMarkPosition + 1);
    }
  } while (false);

  // headers
  do {
    Current = Search + 2;  // 1st header line
    Search = utility::StrNStr(Current, Length - static_cast<size_t>(Current - Data), "\r\n\r\n",
                              sizeof("\r\n\r\n") - 1);  // end of header
    if (Search == nullptr) {
      return false;
    }
    BodyPtr = Search + 4;  // we'll check the body length later
    auto HeaderEnds = Search + 2;
    char *LineEnds;
    while (Current < HeaderEnds &&
           (LineEnds = utility::StrNStr(Current, Length - static_cast<size_t>(Current - Data),
                                        "\r\n", sizeof("\r\n") - 1)) != nullptr) {
      do {
        char *ColonPos;
        for (ColonPos = Current; ColonPos < LineEnds; ++ColonPos) {
          if (*ColonPos == ':') {
            break;
          }
        }
        // not found?
        if (ColonPos == LineEnds) {
          break;
        }
        // value starts
        char *ValueStarts = ColonPos + 1;
        while (*ValueStarts == ' ' && ValueStarts < LineEnds) {
          ++ValueStarts;
        }
        if (ValueStarts > /* no = here*/ LineEnds) {
          break;
        }
        // key starts & ends
        char *KeyStarts = Current;
        char *KeyEnds = ColonPos - 1;
        while (*KeyStarts == ' ' && KeyStarts < ColonPos) {
          ++KeyStarts;
        }
        while (*KeyEnds == ' ' && KeyEnds > Current) {
          --KeyEnds;
        }
        if (KeyStarts >= KeyEnds) {
          break;
        }
        // insert
        Headers.insert(std::make_pair(std::string(KeyStarts, KeyEnds - KeyStarts + 1),
                                      std::string(ValueStarts, LineEnds - ValueStarts)));
      } while (false);
      // update `Current`
      Current = LineEnds + 2;
    }
  } while (false);
  // body
  auto ActualBodyLength = (Data + Length) - BodyPtr;
  do {
    if (ActualBodyLength < 0) {
      return false;
    }
    auto MapIt = Headers.find("Content-Length");
    if (MapIt == Headers.end()) {
      MapIt = Headers.find("content-length");
    }
    if (MapIt == Headers.end()) {
      break;
    }
    int64_t HeaderContentLength;
    if (!utility::CStrToInt64(MapIt->second.c_str(), HeaderContentLength)) {
      break;
    }
    if (ActualBodyLength != HeaderContentLength) {
      return false;
    }
    Body.assign(BodyPtr, ActualBodyLength);
  } while (false);

  return true;
}

}  // namespace wcbot
