#include <gtest/gtest.h>

#include <set>
#include <string>

#include "Utility/Common.h"

using wcbot::utility::Base64Decode;
using wcbot::utility::Base64Encode;
using wcbot::utility::CStrToInt64;
using wcbot::utility::CStrToUInt64;
using wcbot::utility::HexStringCase;
using wcbot::utility::Md5String;
using wcbot::utility::StrNStr;
using wcbot::utility::ThreadLocalRand;
using wcbot::utility::UrlDecode;

// -----------------------------------------------------------------------------
// Md5String
// -----------------------------------------------------------------------------

TEST(CommonMd5, KnownVectors) {
  // RFC 1321 / Wikipedia test vectors.
  EXPECT_EQ(Md5String("", 0), "d41d8cd98f00b204e9800998ecf8427e");
  EXPECT_EQ(Md5String("abc", 3), "900150983cd24fb0d6963f7d28e17f72");
  const std::string foxBrown =
      "The quick brown fox jumps over the lazy dog";
  EXPECT_EQ(Md5String(foxBrown.data(), foxBrown.size()),
            "9e107d9d372bb6826bd81d3542a419d6");
}

TEST(CommonMd5, UpperCase) {
  auto Hex = Md5String("abc", 3, HexStringCase::kUpper);
  EXPECT_EQ(Hex, "900150983CD24FB0D6963F7D28E17F72");
}

// -----------------------------------------------------------------------------
// Base64
// -----------------------------------------------------------------------------

TEST(CommonBase64, EncodeDecodeRoundTrip) {
  const std::string Input = "wcbot:hello, world! 你好";
  auto Encoded = Base64Encode(Input.data(), Input.size());
  auto Decoded = Base64Decode(Encoded.data(), Encoded.size());
  EXPECT_EQ(Decoded, Input);
}

TEST(CommonBase64, KnownVector) {
  // RFC 4648 examples.
  EXPECT_EQ(Base64Encode("f", 1), "Zg==");
  EXPECT_EQ(Base64Encode("fo", 2), "Zm8=");
  EXPECT_EQ(Base64Encode("foo", 3), "Zm9v");
  EXPECT_EQ(Base64Encode("foob", 4), "Zm9vYg==");
  EXPECT_EQ(Base64Encode("fooba", 5), "Zm9vYmE=");
  EXPECT_EQ(Base64Encode("foobar", 6), "Zm9vYmFy");
}

// -----------------------------------------------------------------------------
// UrlDecode
// -----------------------------------------------------------------------------

TEST(CommonUrlDecode, Basics) {
  EXPECT_EQ(UrlDecode("hello+world"), "hello world");
  EXPECT_EQ(UrlDecode("a%20b"), "a b");
  EXPECT_EQ(UrlDecode("%41%42%43"), "ABC");
  EXPECT_EQ(UrlDecode("plain"), "plain");
}

TEST(CommonUrlDecode, IncompleteEscapeSwallowed) {
  // The current implementation tolerates a trailing incomplete `%` by
  // breaking out of the loop. We pin this behaviour so future refactors are
  // intentional.
  EXPECT_EQ(UrlDecode("a%2"), "a");
  EXPECT_EQ(UrlDecode("a%"), "a");
}

// -----------------------------------------------------------------------------
// CStrToInt64 / CStrToUInt64
// -----------------------------------------------------------------------------

TEST(CommonCStrTo, ParsesValid) {
  int64_t I64 = 0;
  EXPECT_TRUE(CStrToInt64("12345", I64));
  EXPECT_EQ(I64, 12345);
  EXPECT_TRUE(CStrToInt64("-7", I64));
  EXPECT_EQ(I64, -7);

  uint64_t U64 = 0;
  EXPECT_TRUE(CStrToUInt64("9999999999", U64));
  EXPECT_EQ(U64, 9999999999ull);
}

TEST(CommonCStrTo, RejectsGarbage) {
  int64_t I64 = 99;
  EXPECT_FALSE(CStrToInt64("abc", I64));

  uint64_t U64 = 99;
  EXPECT_FALSE(CStrToUInt64("xyz", U64));
}

// -----------------------------------------------------------------------------
// StrNStr
// -----------------------------------------------------------------------------

TEST(CommonStrNStr, FindsNeedle) {
  const char *Hay = "hello world\r\nfoo bar";
  const char *Found = StrNStr(Hay, strlen(Hay), "\r\n", 2);
  ASSERT_NE(Found, nullptr);
  EXPECT_EQ(*Found, '\r');
  EXPECT_EQ(*(Found + 2), 'f');
}

TEST(CommonStrNStr, ReturnsNullWhenAbsent) {
  const char *Hay = "no separators here";
  EXPECT_EQ(StrNStr(Hay, strlen(Hay), "\r\n", 2), nullptr);
}

// -----------------------------------------------------------------------------
// ThreadLocalRand
// -----------------------------------------------------------------------------

TEST(CommonThreadLocalRand, ProducesVariety) {
  // A trivial "is non-degenerate" check: across 1k draws we expect the
  // generator to emit more than a handful of distinct values. This catches
  // the "always returns 0" failure mode without making the test flaky.
  std::set<uint32_t> Seen;
  for (int i = 0; i < 1000; ++i) {
    Seen.insert(ThreadLocalRand());
  }
  EXPECT_GT(Seen.size(), 100u);
}
