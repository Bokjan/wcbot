#pragma once

#include "Core/MemoryBuffer.h"

#include <sys/types.h>

namespace wcbot {

class Job;
class MemoryBuffer;
class ThreadContext;

class Codec {
 public:
  // return value <= 0 for not complete,
  // or specific value representing the valid length
  virtual ssize_t IsComplete(const MemoryBuffer &Buffer) = 0;
  virtual ~Codec() = default;
};
using CodecPtr = Codec *;

}  // namespace wcbot
