#include "Common.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>

#include <fstream>
#include <sstream>

namespace wcbot {
namespace utility {

bool ReadFile(const std::string& Path, std::string& ReceiveBuffer) {
  std::ifstream Stream;
  Stream.open(Path);
  if (!Stream.is_open()) {
    return false;
  }
  std::stringstream SSBuffer;
  SSBuffer << Stream.rdbuf();
  ReceiveBuffer = SSBuffer.str();
  return true;
}

bool CStrToInt64(const char* CStr, int64_t& Out) { return sscanf(CStr, "%" PRId64, &Out) == 1; }

bool CStrToUInt64(const char* CStr, uint64_t& Out) { return sscanf(CStr, "%" PRIu64, &Out) == 1; }

char* StrNStr(const char* Big, const uint64_t BigLength, const char* Little,
              const uint64_t LittleLength) {
  uint64_t Idx = 0;
  for (Idx = 0; Idx <= BigLength - LittleLength; ++Idx) {
    if (*(Big + Idx) != *Little) {
      continue;
    }
    if (memcmp(Big + Idx, Little, LittleLength) == 0) {
      return const_cast<char*>(Big + Idx);
    }
  }
  return nullptr;
}

}  // namespace utility
}  // namespace wcbot
