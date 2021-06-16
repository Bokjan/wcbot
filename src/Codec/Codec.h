#pragma once

#include "Core/UvBuffer.h"

#include <sys/types.h>

namespace wcbot {

class Codec {
 public:
  // returns 0 for not complete,
  // or specific value representing the valid length
  virtual ssize_t IsComplete(const UvBuffer &Buffer) = 0;
  virtual ~Codec() = default;
};
using CodecPtr = Codec *;

}  // namespace wcbot
