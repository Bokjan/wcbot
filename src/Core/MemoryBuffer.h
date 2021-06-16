#pragma once

#include <cstddef>
#include <string>

namespace wcbot {

class MemoryBuffer {
 public:
  MemoryBuffer();
  ~MemoryBuffer();

  void DoubleCapacity();
  void Allocate(size_t SuggestedLength);
  void IncreaseLength(size_t Size);
  void Append(const void* Source, size_t Length);
  void Append(const std::string& Str) { this->Append(Str.data(), Str.length()); }
  void SwapMemory(MemoryBuffer& Other);
  char* GetBase() const { return BasePtr; }
  char* GetCurrent() const { return BasePtr + Length; }
  size_t GetLength() const { return Length; }
  size_t GetCapacity() const { return Capacity; }

 protected:
  size_t kInitialSize = 8192;
  char* BasePtr;
  size_t Length;
  size_t Capacity;
};
using MemoryBufferPtr = MemoryBuffer*;

#define MEMBUF_APP(pmb, sl) ((pmb)->Append(sl, sizeof(sl) - 1))

}  // namespace wcbot
