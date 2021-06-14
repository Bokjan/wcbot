#pragma once

#include "Codec.h"

namespace wcbot {

class CodecHttpRequest : public Codec {
 public:
  virtual size_t Check(const UvBuffer &Buffer);
};
using CodecHttpRequestPtr = CodecHttpRequest *;

class CodecHttpResponse : public Codec {
 public:
  virtual size_t Check(const UvBuffer &Buffer);
};
using CodecHttpResponsePtr = CodecHttpResponse *;

}  // namespace wcbot
