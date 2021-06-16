#pragma once

#include "Codec.h"

namespace wcbot {

class CodecHttpRequest : public Codec {
 public:
  virtual ssize_t IsComplete(const MemoryBuffer &Buffer);
};
using CodecHttpRequestPtr = CodecHttpRequest *;

class CodecHttpResponse : public Codec {
 public:
  virtual ssize_t IsComplete(const MemoryBuffer &Buffer);
};
using CodecHttpResponsePtr = CodecHttpResponse *;

}  // namespace wcbot
