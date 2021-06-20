#include "Common.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>

#include <fstream>
#include <sstream>

#include <openssl/evp.h>
#include <openssl/md5.h>

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

std::string Md5String(const void* Data, const uint64_t Length, HexStringCase Case) {
  char StringBuffer[MD5_DIGEST_LENGTH * 3];  // hex string
  unsigned char BinaryBuffer[MD5_DIGEST_LENGTH];
  MD5_CTX Context;
  MD5_Init(&Context);
  MD5_Update(&Context, Data, Length);
  MD5_Final(BinaryBuffer, &Context);
  const char* FmtCStr = nullptr;
  if (Case == HexStringCase::kLower) {
    FmtCStr = "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x";
  } else {
    FmtCStr = "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X";
  }
  snprintf(StringBuffer, sizeof(StringBuffer), FmtCStr, BinaryBuffer[0], BinaryBuffer[1],
           BinaryBuffer[2], BinaryBuffer[3], BinaryBuffer[4], BinaryBuffer[5], BinaryBuffer[6],
           BinaryBuffer[7], BinaryBuffer[8], BinaryBuffer[9], BinaryBuffer[10], BinaryBuffer[11],
           BinaryBuffer[12], BinaryBuffer[13], BinaryBuffer[14], BinaryBuffer[15]);
  return std::string(StringBuffer, MD5_DIGEST_LENGTH * 2);
}

std::string Base64Encode(const void* Data, const uint64_t Length) {
  const size_t Base64Length = ((Length + 2) / 3) * 4 + 1;
  std::string Buffer(Base64Length, '\0');
  int FinalLength = 0;
  int OutLength;
  EVP_ENCODE_CTX* Context;
#if !defined(OPENSSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER < 0x10100000L
  EVP_ENCODE_CTX ContextStackObj;
  Context = &ContextStackObj;
#else
  Context = EVP_ENCODE_CTX_new();
#endif

  EVP_EncodeInit(Context);
  EVP_EncodeUpdate(Context, reinterpret_cast<unsigned char*>(&Buffer[0]), &OutLength,
                   reinterpret_cast<const unsigned char*>(Data), static_cast<int>(Length));
  FinalLength += OutLength;
  EVP_EncodeFinal(Context, reinterpret_cast<unsigned char*>(&Buffer[0]) + OutLength, &OutLength);
  FinalLength += OutLength;
#if !defined(OPENSSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER < 0x10100000L
  // OpenSSL 1.0, do nothing
#else
  EVP_ENCODE_CTX_free(Context);
#endif
  while (Buffer.length() > Base64Length - 1) {
    Buffer.pop_back();
  }
  return Buffer;
}

std::string Base64Decode(const void* Data, const uint64_t Length) {
  const size_t MaxPlainLength = Length / 4 * 3;
  std::string Buffer(MaxPlainLength, '\0');
  int FinalLength = 0;
  int OutLength;
  EVP_ENCODE_CTX* Context;
#if !defined(OPENSSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER < 0x10100000L
  EVP_ENCODE_CTX ContextStackObj;
  Context = &ContextStackObj;
#else
  Context = EVP_ENCODE_CTX_new();
#endif
  EVP_DecodeInit(Context);
  EVP_DecodeUpdate(Context, reinterpret_cast<unsigned char*>(&Buffer[0]), &OutLength,
                   reinterpret_cast<const unsigned char*>(Data), static_cast<int>(Length));
  FinalLength += OutLength;
  EVP_DecodeFinal(Context, reinterpret_cast<unsigned char*>(&Buffer[0]) + OutLength, &OutLength);
  FinalLength += OutLength;
#if !defined(OPENSSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER < 0x10100000L
  // OpenSSL 1.0, do nothing
#else
  EVP_ENCODE_CTX_free(Context);
#endif
  while (Buffer.length() > static_cast<std::string::size_type>(FinalLength)) {
    Buffer.pop_back();
  }
  return Buffer;
}

static const char UrlDecodeMap[1 << (sizeof(char) * 8)] =
    "000000000000000000000000000000000000000000000000"  // 0~48
    "\x0\x1\x2\x3\x4\x5\x6\x7\x8\x9"
    "0000000"
    "\xA\xB\xC\xD\xE\xF"
    "00000000000000000000000000"
    "\xA\xB\xC\xD\xE\xF";

#define IS_HEX_DIGIT(x) (UrlDecodeMap[static_cast<size_t>(x)] != '0')

std::string UrlDecode(const std::string& Plain) {
  std::string Buffer;
  Buffer.reserve(Plain.size());
  for (size_t i = 0; i < Plain.size(); ++i) {
    // +
    if (Plain[i] == '+') {
      Buffer.push_back(' ');
      continue;
    }
    // %xx
    if (Plain[i] == '%') {
      if (i >= Plain.size() - 2) {
        break;
      }
      if (!IS_HEX_DIGIT(Plain[i + 1]) || !IS_HEX_DIGIT(Plain[i + 1])) {
        continue;
      }
      Buffer.push_back((UrlDecodeMap[static_cast<size_t>(Plain[i + 1])] << 4) |
                       UrlDecodeMap[static_cast<size_t>(Plain[i + 2])]);
      i += 2;
      continue;
    }
    // others
    Buffer.push_back(Plain[i]);
  }
  return Buffer;
}

}  // namespace utility
}  // namespace wcbot
