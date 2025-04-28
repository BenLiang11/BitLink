#include "gtest/gtest.h"
#include "response.h"
#include <string>
#include <map>

class ResponseTest : public ::testing::Test {
protected:
  Response response_;
};

TEST_F(ResponseTest, DefaultConstructor) {
  // Default constructor should set status to OK
  EXPECT_EQ(response_.status(), Response::OK);
  
  // Headers and body should be empty
  EXPECT_TRUE(response_.headers().empty());
  EXPECT_TRUE(response_.body().empty());
}

TEST_F(ResponseTest, SetStatusOK) {
  // Test setting OK Status
  response_.set_status(Response::OK);
  EXPECT_EQ(response_.status(), Response::OK);

  // Check final response string to ensure status is correct
  std::string response_str = response_.to_string();
  EXPECT_TRUE(response_str.find("HTTP/1.1 200 OK") != std::string::npos);
}

TEST_F(ResponseTest, SetStatusNotFound) {
  // Test setting Not Found Status
  response_.set_status(Response::NOT_FOUND);
  EXPECT_EQ(response_.status(), Response::NOT_FOUND);

  // Check final response string to ensure status is correct
  std::string response_str = response_.to_string();
  EXPECT_TRUE(response_str.find("HTTP/1.1 404 Not Found") != std::string::npos);
}

TEST_F(ResponseTest, SetStatusInternalServerError) {
  // Test setting Internal Server Error Status
  response_.set_status(Response::INTERNAL_SERVER_ERROR);
  EXPECT_EQ(response_.status(), Response::INTERNAL_SERVER_ERROR);
  
  // Check final response string to ensure status is correct
  std::string response_str = response_.to_string();
  EXPECT_TRUE(response_str.find("HTTP/1.1 500 Internal Server Error") != std::string::npos);
}

TEST_F(ResponseTest, SetStatusDefaultBranch) {
  // Test setting bad response code to enable default branch
  response_.set_status(static_cast<Response::StatusCode>(123));
  EXPECT_EQ(response_.status(), 123);

  // Check final response string to ensure status is correct
  std::string response_str = response_.to_string();
  EXPECT_TRUE(response_str.find("HTTP/1.1 200 OK") != std::string::npos);
}


TEST_F(ResponseTest, SetAndGetHeader) {
  // Test setting and getting headers
  response_.set_header("Content-Type", "text/html");
  response_.set_header("Content-Length", "123");
  response_.set_header("Connection", "close");
  
  // Check that headers are correctly stored
  const auto& headers = response_.headers();
  EXPECT_EQ(headers.size(), 3);
  
  auto it = headers.find("Content-Type");
  EXPECT_NE(it, headers.end());
  EXPECT_EQ(it->second, "text/html");
  
  it = headers.find("Content-Length");
  EXPECT_NE(it, headers.end());
  EXPECT_EQ(it->second, "123");
  
  it = headers.find("Connection");
  EXPECT_NE(it, headers.end());
  EXPECT_EQ(it->second, "close");
}

TEST_F(ResponseTest, SetAndGetBody) {
  // Test setting and getting the body
  std::string test_body = "This is a test body";
  response_.set_body(test_body);
  
  EXPECT_EQ(response_.body(), test_body);
  
  // Check that Content-Length header is automatically set
  const auto& headers = response_.headers();
  auto it = headers.find("Content-Length");
  EXPECT_NE(it, headers.end());
  EXPECT_EQ(it->second, std::to_string(test_body.size()));
}

TEST_F(ResponseTest, StatusToString) {
  // Test converting status codes to strings in response output
  response_.set_status(Response::OK);
  std::string response_str = response_.to_string();
  EXPECT_TRUE(response_str.find("HTTP/1.1 200 OK") != std::string::npos);
  
  response_.set_status(Response::NOT_FOUND);
  response_str = response_.to_string();
  EXPECT_TRUE(response_str.find("HTTP/1.1 404 Not Found") != std::string::npos);
  
  response_.set_status(Response::INTERNAL_SERVER_ERROR);
  response_str = response_.to_string();
  EXPECT_TRUE(response_str.find("HTTP/1.1 500 Internal Server Error") != std::string::npos);
}

TEST_F(ResponseTest, HeadersInToString) {
  // Test that headers are included in the string representation
  response_.set_header("Content-Type", "text/plain");
  response_.set_header("Connection", "close");
  
  std::string response_str = response_.to_string();
  
  EXPECT_TRUE(response_str.find("Content-Type: text/plain") != std::string::npos);
  EXPECT_TRUE(response_str.find("Connection: close") != std::string::npos);
}

TEST_F(ResponseTest, BodyInToString) {
  // Test that the body is included in the string representation
  std::string test_body = "Hello, World!";
  response_.set_body(test_body);
  
  std::string response_str = response_.to_string();
  
  // Check that the body is included and preceded by a blank line
  EXPECT_TRUE(response_str.find("\r\n\r\n" + test_body) != std::string::npos);
}

TEST_F(ResponseTest, CompleteResponse) {
  // Test a complete response with status, headers, and body
  response_.set_status(Response::OK);
  response_.set_header("Content-Type", "text/html");
  response_.set_header("Connection", "close");
  response_.set_body("<html><body><h1>Hello</h1></body></html>");
  
  std::string response_str = response_.to_string();
  
  // Check all parts of the response
  EXPECT_TRUE(response_str.find("HTTP/1.1 200 OK") != std::string::npos);
  EXPECT_TRUE(response_str.find("Content-Type: text/html") != std::string::npos);
  EXPECT_TRUE(response_str.find("Connection: close") != std::string::npos);
  EXPECT_TRUE(response_str.find("Content-Length: 40") != std::string::npos);
  EXPECT_TRUE(response_str.find("<html><body><h1>Hello</h1></body></html>") != std::string::npos);
  
  // Check proper formatting with carriage returns and newlines
  EXPECT_TRUE(response_str.find("\r\n\r\n") != std::string::npos);
}

TEST_F(ResponseTest, OverwriteHeader) {
  // Test that setting a header twice overwrites the previous value
  response_.set_header("Content-Type", "text/plain");
  EXPECT_EQ(response_.headers().at("Content-Type"), "text/plain");
  
  response_.set_header("Content-Type", "text/html");
  EXPECT_EQ(response_.headers().at("Content-Type"), "text/html");
}

TEST_F(ResponseTest, BodyUpdatesContentLength) {
  // Test that updating the body updates the Content-Length header
  response_.set_body("Short body");
  EXPECT_EQ(response_.headers().at("Content-Length"), "10");
  
  response_.set_body("This is a longer body text");
  EXPECT_EQ(response_.headers().at("Content-Length"), "26");
}

TEST_F(ResponseTest, EmptyBody) {
  // Test response with empty body
  response_.set_body("");
  EXPECT_EQ(response_.headers().at("Content-Length"), "0");
  
  std::string response_str = response_.to_string();
  EXPECT_TRUE(response_str.find("Content-Length: 0") != std::string::npos);
  
  // The body should be empty after the double CRLF
  size_t header_end = response_str.find("\r\n\r\n");
  EXPECT_NE(header_end, std::string::npos);
  EXPECT_EQ(response_str.substr(header_end + 4), "");
} 