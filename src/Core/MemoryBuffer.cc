#include "MemoryBuffer.h"

#include <cstdlib>
#include <cstring>

#include <algorithm>

#include "Utility/Logger.h"

namespace wcbot {

MemoryBuffer::MemoryBuffer() : BasePtr(nullptr), Length(0), Capacity(0) {
  BasePtr = reinterpret_cast<char *>(malloc(MemoryBuffer::kInitialSize));
  Capacity = MemoryBuffer::kInitialSize;
}

MemoryBuffer::~MemoryBuffer() {
  if (this->BasePtr != nullptr) {
    LOG_TRACE("baseptr=%p", BasePtr);
    free(this->BasePtr);
    this->BasePtr = nullptr;
  }
}

void MemoryBuffer::DoubleCapacity() {
  BasePtr = reinterpret_cast<char*>(realloc(BasePtr, Capacity * 2));
  Capacity *= 2;
}

void MemoryBuffer::Allocate(size_t SuggestedLength) {
  // test if current length is sufficient
  if (this->Capacity - this->Length >= SuggestedLength) {
    return;
  }
  size_t NewSize = SuggestedLength + this->Length;
  this->BasePtr = reinterpret_cast<char*>(realloc(this->BasePtr, NewSize));
}

void MemoryBuffer::IncreaseLength(size_t Size) {
  if (Size == 0) {
    return;
  }
  this->Length += Size;
  this->BasePtr[this->Length] = '\0';
}

void MemoryBuffer::Append(const void *Source, size_t Length) {
  while (this->Length + Length > Capacity) {
    this->DoubleCapacity();
  }
  memcpy(this->BasePtr + this->Length, Source, Length);
  this->Length += Length;
}

void MemoryBuffer::SwapMemory(MemoryBuffer& Other) {
  std::swap(BasePtr, Other.BasePtr);
  std::swap(Length, Other.Length);
  std::swap(Capacity, Other.Capacity);
}

}  // namespace wcbot
