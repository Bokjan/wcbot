#pragma once

#include "Codec.h"

namespace wcbot {

class CodecHttpRequest : public Codec {
 public:
  virtual ssize_t IsComplete(const UvBuffer &Buffer);
};
using CodecHttpRequestPtr = CodecHttpRequest *;

class CodecHttpResponse : public Codec {
 public:
  virtual ssize_t IsComplete(const UvBuffer &Buffer);
};
using CodecHttpResponsePtr = CodecHttpResponse *;

}  // namespace wcbot
