#pragma once

#include <sys/types.h>

#include "../Core/MemoryBuffer.h"

namespace wcbot {

class Job;
class MemoryBuffer;
class ThreadContext;

class Codec {
 public:
  // return value <= 0 for not complete,
  // or specific value representing the valid length
  virtual ssize_t IsComplete(const MemoryBuffer *Buffer) = 0;
  virtual ~Codec() = default;
};

}  // namespace wcbot
