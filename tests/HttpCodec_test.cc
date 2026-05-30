#include <gtest/gtest.h>

#include <string>

#include "Codec/HttpCodec.h"
#include "Utility/MemoryBuffer.h"

using wcbot::HttpRequestCodec;
using wcbot::HttpResponseCodec;
using wcbot::MemoryBuffer;

namespace {

void Fill(MemoryBuffer &Buf, const std::string &Data) {
  Buf.Append(Data.data(), Data.size());
}

}  // namespace

// -----------------------------------------------------------------------------
// HttpRequestCodec::IsComplete
// -----------------------------------------------------------------------------

TEST(HttpRequestCodec, IncompleteHeadersReturnsZero) {
  HttpRequestCodec C;
  MemoryBuffer Buf;
  Fill(Buf, "GET / HTTP/1.1\r\nHost: x\r\n");  // missing terminating CRLFCRLF
  EXPECT_EQ(C.IsComplete(&Buf), 0);
}

TEST(HttpRequestCodec, NoBodyHeaderOnlyComplete) {
  HttpRequestCodec C;
  MemoryBuffer Buf;
  std::string Wire = "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
  Fill(Buf, Wire);
  ssize_t N = C.IsComplete(&Buf);
  EXPECT_EQ(static_cast<size_t>(N), Wire.size());
}

TEST(HttpRequestCodec, BodyDeclaredButShort) {
  HttpRequestCodec C;
  MemoryBuffer Buf;
  // 10 bytes promised, only 5 present — codec should return total length
  // header+10, signaling "wait for more" by being > buffer size.
  Fill(Buf, "POST / HTTP/1.1\r\nContent-Length: 10\r\n\r\nhello");
  ssize_t N = C.IsComplete(&Buf);
  EXPECT_GT(static_cast<size_t>(N), Buf.GetLength());
}

TEST(HttpRequestCodec, BodyFullyPresent) {
  HttpRequestCodec C;
  MemoryBuffer Buf;
  std::string Body = "hello12345";  // 10 bytes
  std::string Wire = "POST / HTTP/1.1\r\nContent-Length: 10\r\n\r\n" + Body;
  Fill(Buf, Wire);
  ssize_t N = C.IsComplete(&Buf);
  EXPECT_EQ(static_cast<size_t>(N), Wire.size());
}

TEST(HttpRequestCodec, NonHttpMethodRejected) {
  HttpRequestCodec C;
  MemoryBuffer Buf;
  Fill(Buf, "FOOBAR / HTTP/1.1\r\nHost: x\r\n\r\n");
  EXPECT_EQ(C.IsComplete(&Buf), 0);
}

// -----------------------------------------------------------------------------
// HttpResponseCodec::IsComplete
// -----------------------------------------------------------------------------

TEST(HttpResponseCodec, RecognizesHttp1Prefix) {
  HttpResponseCodec C;
  MemoryBuffer Buf;
  std::string Wire = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
  Fill(Buf, Wire);
  ssize_t N = C.IsComplete(&Buf);
  EXPECT_EQ(static_cast<size_t>(N), Wire.size());
}

TEST(HttpResponseCodec, RejectsNonHttpPrefix) {
  HttpResponseCodec C;
  MemoryBuffer Buf;
  Fill(Buf, "HELLO/1.1 200 OK\r\n\r\n");
  EXPECT_EQ(C.IsComplete(&Buf), 0);
}
