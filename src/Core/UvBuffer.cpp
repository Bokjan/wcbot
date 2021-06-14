#include "UvBuffer.h"

#include <cstdlib>
#include <cstring>

namespace wcbot {

UvBuffer::~UvBuffer() {
  if (this->BasePtr != nullptr) {
    free(this->BasePtr);
  }
}

void UvBuffer::Allocate(size_t SuggestedLength) {
  // test if current length is sufficient
  if (this->Capacity - this->Length >= SuggestedLength) {
    return;
  }
  size_t NewSize = SuggestedLength + this->Length;
  // try to realloc
  if (this->BasePtr != nullptr) {
    void *MemPtr = realloc(this->BasePtr, NewSize);
    if (MemPtr != nullptr) {
      this->Capacity = NewSize;
      return;
    }
  }
  // no... malloc, and copy
  void *MemPtr = malloc(NewSize);
  memmove(MemPtr, this->BasePtr, this->Length);
  free(this->BasePtr);
  this->BasePtr = reinterpret_cast<char *>(MemPtr);
}

void UvBuffer::IncreaseLength(size_t Size) {
  if (Size == 0) {
    return;
  }
  this->Length += Size;
  this->BasePtr[this->Length] = '\0';
}

void UvBuffer::Dequeue(size_t Size) {
  if (Size >= Length) {
    this->Length = 0;
    return;
  }
  memmove(this->BasePtr, this->BasePtr + Size, this->Length - Size);
  this->Length -= Size;
}

}  // namespace wcbot
