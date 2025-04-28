#include "gtest/gtest.h"
#include "request.h"
#include <string>
#include <map>

class RequestTest : public ::testing::Test {
protected:
  // Helper to create a well-formed HTTP request
  std::string MakeValidRequest() {
    return "GET /path/to/resource HTTP/1.1\r\n"
           "Host: example.com\r\n"
           "User-Agent: Test/1.0\r\n"
           "Accept: */*\r\n"
           "Content-Length: 13\r\n"
           "\r\n"
           "Hello, World!";
  }
};

TEST_F(RequestTest, ParsesValidRequest) {
  std::string raw_request = MakeValidRequest();
  Request request(raw_request);
  
  EXPECT_EQ(request.method(), "GET");
  EXPECT_EQ(request.uri(), "/path/to/resource");
  EXPECT_EQ(request.version(), "HTTP/1.1");
  EXPECT_EQ(request.body(), "Hello, World!");
  EXPECT_EQ(request.raw_request(), raw_request);
}

TEST_F(RequestTest, ParsesHeaders) {
  std::string raw_request = MakeValidRequest();
  Request request(raw_request);
  
  EXPECT_EQ(request.get_header("Host"), "example.com");
  EXPECT_EQ(request.get_header("User-Agent"), "Test/1.0");
  EXPECT_EQ(request.get_header("Accept"), "*/*");
  EXPECT_EQ(request.get_header("Content-Length"), "13");
  
  // Check that non-existent headers return empty string
  EXPECT_EQ(request.get_header("X-Non-Existent"), "");
  
  // Test headers case insensitivity
  EXPECT_EQ(request.get_header("host"), "example.com");
  EXPECT_EQ(request.get_header("HOST"), "example.com");
  EXPECT_EQ(request.get_header("Host"), "example.com");
}

TEST_F(RequestTest, ParsesRequestWithoutBody) {
  std::string raw_request = 
      "GET /index.html HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  EXPECT_EQ(request.method(), "GET");
  EXPECT_EQ(request.uri(), "/index.html");
  EXPECT_EQ(request.version(), "HTTP/1.1");
  EXPECT_EQ(request.body(), "");
}

TEST_F(RequestTest, ParsesRequestWithExtraSpaces) {
  std::string raw_request = 
      "GET  /spaced/path  HTTP/1.1\r\n"
      "Host:   example.com   \r\n"
      "User-Agent:  Test/1.0  \r\n"
      "\r\n";
  
  Request request(raw_request);
  
  EXPECT_EQ(request.method(), "GET");
  EXPECT_EQ(request.uri(), "/spaced/path");
  EXPECT_EQ(request.version(), "HTTP/1.1");
  EXPECT_EQ(request.get_header("Host"), "example.com   ");
  EXPECT_EQ(request.get_header("User-Agent"), "Test/1.0  ");
}

TEST_F(RequestTest, HandlesEmptyRequest) {
  std::string raw_request = "";
  Request request(raw_request);
  
  EXPECT_EQ(request.method(), "");
  EXPECT_EQ(request.uri(), "");
  EXPECT_EQ(request.version(), "");
  EXPECT_EQ(request.body(), "");
  EXPECT_TRUE(request.headers().empty());
}

TEST_F(RequestTest, HandlesMalformedRequest) {
  std::string raw_request = "This is not a valid HTTP request";
  Request request(raw_request);
  
  // Should extract what it can but most fields will be empty or incorrect
  EXPECT_EQ(request.method(), "This");
  EXPECT_EQ(request.uri(), "is");
  EXPECT_EQ(request.version(), "not");
  EXPECT_TRUE(request.body().empty());
}

TEST_F(RequestTest, ParsesRequestWithMultilineBody) {
  std::string raw_request = 
      "POST /form HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Content-Type: text/plain\r\n"
      "Content-Length: 28\r\n"
      "\r\n"
      "Line 1\n"
      "Line 2\n"
      "Line 3";
  
  Request request(raw_request);
  
  EXPECT_EQ(request.method(), "POST");
  EXPECT_EQ(request.uri(), "/form");
  EXPECT_EQ(request.body(), "Line 1\nLine 2\nLine 3");
}

TEST_F(RequestTest, AllHeadersAccessor) {
  std::string raw_request = MakeValidRequest();
  Request request(raw_request);
  
  const auto& headers = request.headers();
  EXPECT_EQ(headers.size(), 4); // We have 4 headers in the test request
  
  // Check that all expected headers are present
  EXPECT_TRUE(headers.find("Host") != headers.end());
  EXPECT_TRUE(headers.find("User-Agent") != headers.end());
  EXPECT_TRUE(headers.find("Accept") != headers.end());
  EXPECT_TRUE(headers.find("Content-Length") != headers.end());
}

TEST_F(RequestTest, MultipleColonsInHeader) {
  std::string raw_request = 
      "GET /index.html HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Custom-Header: value:with:colons\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  EXPECT_EQ(request.get_header("Custom-Header"), "value:with:colons");
}

TEST_F(RequestTest, EmptyHeaderValue) {
  std::string raw_request = 
      "GET /index.html HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Empty-Header:\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  EXPECT_EQ(request.get_header("Empty-Header"), "");
} 