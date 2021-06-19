#pragma once

#include <cstddef>

#include <string>

namespace wcbot {

class MemoryBuffer {
 public:
  MemoryBuffer();
  ~MemoryBuffer();
  MemoryBuffer(const MemoryBuffer&) = delete;
  MemoryBuffer(const MemoryBuffer&&) = delete;
  static MemoryBuffer* Create() { return new MemoryBuffer; }
  void Destroy() { delete this; }

  void DoubleCapacity();
  void Allocate(size_t SuggestedLength);
  void IncreaseLength(size_t Size);
  MemoryBuffer* Append(const void* Source, size_t Length);
  MemoryBuffer* Append(const std::string& Str) { return this->Append(Str.data(), Str.length()); }
  void SwapMemory(MemoryBuffer& Other);
  void SetNullTerminated();
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

#define MEMBUF_APP(pmb, sl) ((pmb)->Append(sl, sizeof(sl) - 1))

}  // namespace wcbot
