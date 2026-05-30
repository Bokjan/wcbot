#pragma once

#include <cstdint>

#include <string>

namespace wcbot {
namespace utility {

enum class HexStringCase : int { kLower, kUpper };

bool ReadFile(const std::string &Path, std::string &ReceiveBuffer);
bool CStrToInt64(const char *CStr, int64_t &Out);
bool CStrToUInt64(const char *CStr, uint64_t &Out);
char *StrNStr(const char *Big, const uint64_t BigLength, const char *Little,
              const uint64_t LittleLength);
std::string Md5String(const void *Data, const uint64_t Length,
                      HexStringCase Case = HexStringCase::kLower);
std::string Base64Encode(const void *Data, const uint64_t Length);
std::string Base64Decode(const void *Data, const uint64_t Length);
std::string UrlDecode(const std::string &Plain);

// Thread-safe pseudo-random integer generator.
// Each thread owns its own `std::mt19937` instance, seeded once on first use,
// avoiding the data race that plain `rand()`/`srand()` would introduce when
// called from multiple worker threads.
uint32_t ThreadLocalRand();

}  // namespace utility
}  // namespace wcbot
