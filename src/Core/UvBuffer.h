#pragma once

#include <cstddef>

namespace wcbot {

class UvBuffer {
 public:
  UvBuffer() : BasePtr(nullptr), Length(0), Capacity(0) {}
  ~UvBuffer();

  void Allocate(size_t SuggestedLength);
  void IncreaseLength(size_t Size);
  char* GetBase() const { return BasePtr; }
  char* GetCurrent() const { return BasePtr + Length; }
  size_t GetLength() const { return Length; }
  size_t GetCapacity() const { return Capacity; }

 private:
  char* BasePtr;
  size_t Length;
  size_t Capacity;
};

}  // namespace wcbot
