#pragma once

#include "Codec.h"

namespace wcbot {

class CodecHttpRequest : public Codec {
 public:
  virtual size_t Check(const UvBuffer &Buffer);
};

class CodecHttpResponse : public Codec {
 public:
  virtual size_t Check(const UvBuffer &Buffer);
};

}  // namespace wcbot
