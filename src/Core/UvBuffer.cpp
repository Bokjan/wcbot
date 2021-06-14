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

void UvBuffer::IncreaseLength(size_t Size) { this->Length += Size; }

}  // namespace wcbot
