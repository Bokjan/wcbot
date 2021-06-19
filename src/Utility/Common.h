#pragma once

#include <cstdint>

#include <string>

namespace wcbot {
namespace utility {

bool ReadFile(const std::string &Path, std::string &ReceiveBuffer);
bool CStrToInt64(const char *CStr, int64_t &Out);
bool CStrToUInt64(const char *CStr, uint64_t &Out);
char *StrNStr(const char *Big, const uint64_t BigLength, const char *Little,
              const uint64_t LittleLength);

}  // namespace utility
}  // namespace wcbot
