#pragma once

#include "Core/UvBuffer.h"

namespace wcbot {

class Codec {
 public:
  // returns 0 for not complete,
  // or specific value representing the valid length
  virtual size_t Check(const UvBuffer &Buffer) = 0;
  virtual ~Codec() = default;
};
using CodecPtr = Codec *;

}  // namespace wcbot
