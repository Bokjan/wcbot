#include "MemoryBuffer.h"

#include <cstdlib>
#include <cstring>

#include <algorithm>

#include "../Utility/Logger.h"

namespace wcbot {

MemoryBuffer::MemoryBuffer() : BasePtr(nullptr), Length(0), Capacity(0) {
  BasePtr = reinterpret_cast<char*>(malloc(MemoryBuffer::kInitialSize));
  if (BasePtr == nullptr) {
    LOG_FATAL("MemoryBuffer initial malloc failed, size=%zu", MemoryBuffer::kInitialSize);
    abort();
  }
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
  size_t NewCapacity = Capacity * 2;
  void* NewPtr = realloc(BasePtr, NewCapacity);
  if (NewPtr == nullptr) {
    // realloc failure: original buffer is still valid; bail out without
    // overwriting `BasePtr` with nullptr (which would leak the original).
    LOG_FATAL("MemoryBuffer realloc failed, oldCap=%zu newCap=%zu", Capacity, NewCapacity);
    abort();
  }
  BasePtr = reinterpret_cast<char*>(NewPtr);
  Capacity = NewCapacity;
}

void MemoryBuffer::Allocate(size_t SuggestedLength) {
  // test if remaining capacity is sufficient
  if (this->Capacity - this->Length >= SuggestedLength) {
    return;
  }
  // grow geometrically until the request is satisfied; this also keeps
  // `Capacity` correctly in sync (the previous implementation forgot to update
  // it, which silently broke `MaxRecvBuffLength` size-guarding).
  size_t Required = this->Length + SuggestedLength;
  size_t NewCapacity = this->Capacity == 0 ? kInitialSize : this->Capacity;
  while (NewCapacity < Required) {
    NewCapacity *= 2;
  }
  void* NewPtr = realloc(this->BasePtr, NewCapacity);
  if (NewPtr == nullptr) {
    LOG_FATAL("MemoryBuffer realloc failed, oldCap=%zu newCap=%zu", Capacity, NewCapacity);
    abort();
  }
  this->BasePtr = reinterpret_cast<char*>(NewPtr);
  this->Capacity = NewCapacity;
}

void MemoryBuffer::IncreaseLength(size_t Size) {
  if (Size == 0) {
    return;
  }
  this->Length += Size;
  // this->SetNullTerminated();
}

MemoryBuffer* MemoryBuffer::Append(const void* Source, size_t Length) {
  while (this->Length + Length > Capacity) {
    this->DoubleCapacity();
  }
  memcpy(this->BasePtr + this->Length, Source, Length);
  this->Length += Length;
  return this;
}

void MemoryBuffer::SwapMemory(MemoryBuffer& Other) {
  std::swap(BasePtr, Other.BasePtr);
  std::swap(Length, Other.Length);
  std::swap(Capacity, Other.Capacity);
}

void MemoryBuffer::SetNullTerminated() {
  this->Append("", 1);
  --this->Length;
}

}  // namespace wcbot
