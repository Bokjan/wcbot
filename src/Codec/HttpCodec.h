#pragma once

#include "Codec.h"

namespace wcbot {

class HttpRequestCodec : public Codec {
 public:
  virtual ssize_t IsComplete(const MemoryBuffer *Buffer);
};

class HttpResponseCodec : public Codec {
 public:
  virtual ssize_t IsComplete(const MemoryBuffer *Buffer);
};

}  // namespace wcbot
