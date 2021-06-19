#pragma once

#include <cstdint>

#include <string>

namespace wcbot {
namespace utility {

bool ReadFile(const std::string& Path, std::string& ReceiveBuffer);

bool CStrToUInt64(const char *CStr, uint64_t &Out);

}
}  // namespace wcbot
