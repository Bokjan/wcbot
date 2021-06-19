#include "Common.h"

#include <cstdio>
#include <cinttypes>

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

bool CStrToUInt64(const char* CStr, uint64_t& Out) { return sscanf(CStr, "%" PRIu64, &Out) == 1; }

}  // namespace utility
}  // namespace wcbot
