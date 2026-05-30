#include <gtest/gtest.h>

#include <string>

#include "Utility/HttpPackage.h"

using wcbot::HttpRequest;

namespace {

std::string MakeReq(const std::string &Line, const std::string &Headers,
                    const std::string &Body) {
  return Line + "\r\n" + Headers + "\r\n\r\n" + Body;
}

}  // namespace

// -----------------------------------------------------------------------------
// SetUrl / GetUrl
// -----------------------------------------------------------------------------

TEST(HttpRequestUrl, ParsesHttpsWithPathAndQuery) {
  HttpRequest R;
  ASSERT_TRUE(R.SetUrl("https://api.example.com/v1/echo?x=1&y=2"));
  EXPECT_EQ(R.Protocol, HttpRequest::ProtocolEnum::kHttps);
  EXPECT_EQ(R.Headers["Host"], "api.example.com");
  EXPECT_EQ(R.Path, "/v1/echo");
  EXPECT_EQ(R.QueryString, "x=1&y=2");

  // Round-trip
  EXPECT_EQ(R.GetUrl(), "https://api.example.com/v1/echo?x=1&y=2");
}

TEST(HttpRequestUrl, ParsesHttpWithoutQuery) {
  HttpRequest R;
  ASSERT_TRUE(R.SetUrl("http://example.com/"));
  EXPECT_EQ(R.Protocol, HttpRequest::ProtocolEnum::kHttp);
  EXPECT_EQ(R.Headers["Host"], "example.com");
  EXPECT_EQ(R.Path, "/");
  EXPECT_TRUE(R.QueryString.empty());
  EXPECT_EQ(R.GetUrl(), "http://example.com/");
}

TEST(HttpRequestUrl, RejectsUrlWithoutScheme) {
  HttpRequest R;
  EXPECT_FALSE(R.SetUrl("example.com/path"));
}

// -----------------------------------------------------------------------------
// Parse
// -----------------------------------------------------------------------------

TEST(HttpRequestParse, GetWithQuery) {
  std::string Wire = MakeReq("GET /verify?signature=abc HTTP/1.1",
                             "Host: api.example.com", "");
  HttpRequest R;
  ASSERT_TRUE(R.Parse(Wire));
  EXPECT_EQ(R.Method, HttpRequest::MethodEnum::kGet);
  EXPECT_EQ(R.Path, "/verify");
  EXPECT_EQ(R.QueryString, "signature=abc");
  EXPECT_EQ(R.Headers["Host"], "api.example.com");
  EXPECT_TRUE(R.Body.empty());
}

TEST(HttpRequestParse, PostWithBody) {
  const std::string Body = R"({"hello":"world"})";
  std::string Wire = MakeReq(
      "POST /verify HTTP/1.1",
      "Host: api.example.com\r\nContent-Type: application/json\r\nContent-Length: " +
          std::to_string(Body.size()),
      Body);
  HttpRequest R;
  ASSERT_TRUE(R.Parse(Wire));
  EXPECT_EQ(R.Method, HttpRequest::MethodEnum::kPost);
  EXPECT_EQ(R.Body, Body);
  EXPECT_EQ(R.Headers["Content-Type"], "application/json");
}

TEST(HttpRequestParse, RejectsUnknownMethod) {
  std::string Wire = MakeReq("FOO / HTTP/1.1", "Host: x", "");
  HttpRequest R;
  EXPECT_FALSE(R.Parse(Wire));
}

TEST(HttpRequestParse, RejectsTruncatedHeaders) {
  // Missing the blank line that terminates the headers.
  std::string Wire = "GET / HTTP/1.1\r\nHost: x\r\n";
  HttpRequest R;
  EXPECT_FALSE(R.Parse(Wire));
}

TEST(HttpRequestParse, ContentLengthMismatchFails) {
  // Header advertises 100 bytes but only 5 are present.
  std::string Wire = MakeReq("POST / HTTP/1.1",
                             "Host: x\r\nContent-Length: 100", "hello");
  HttpRequest R;
  EXPECT_FALSE(R.Parse(Wire));
}
