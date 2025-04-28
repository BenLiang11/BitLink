#include "gtest/gtest.h"
#include "handlers/echo_handler.h"
#include "request.h"
#include "response.h"
#include <string>
#include <map>

class EchoHandlerTest : public ::testing::Test {
protected:
  EchoHandler handler_;
};

TEST_F(EchoHandlerTest, HandlesBasicRequest) {
  // Create a test request
  std::string raw_request = 
      "GET /echo HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "User-Agent: Mozilla/5.0\r\n"
      "Accept: text/html\r\n"
      "\r\n";
  
  Request request(raw_request);
  Response response;
  
  // Call the handler
  bool result = handler_.HandleRequest(request, &response);
  
  // Check the result
  EXPECT_TRUE(result);
  
  // Check the response status
  EXPECT_EQ(response.status(), Response::OK);
  
  // Check the content type
  const auto& headers = response.headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "text/plain");
  
  // Check the Connection header
  auto connection_it = headers.find("Connection");
  EXPECT_NE(connection_it, headers.end());
  EXPECT_EQ(connection_it->second, "close");
  
  // Check that the body contains the raw request
  EXPECT_EQ(response.body(), raw_request);
}

TEST_F(EchoHandlerTest, HandlesRequestWithBody) {
  // Create a request with body
  std::string raw_request = 
      "POST /echo HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "Content-Type: application/x-www-form-urlencoded\r\n"
      "Content-Length: 13\r\n"
      "\r\n"
      "name=test&x=10";
  
  Request request(raw_request);
  Response response;
  
  // Call the handler
  bool result = handler_.HandleRequest(request, &response);
  
  // Check the result
  EXPECT_TRUE(result);
  
  // Check that the response includes Content-Length header
  const auto& headers = response.headers();
  auto content_length_it = headers.find("Content-Length");
  EXPECT_NE(content_length_it, headers.end());
  EXPECT_EQ(content_length_it->second, std::to_string(raw_request.size()));
  
  // Check that the body contains the raw request
  EXPECT_EQ(response.body(), raw_request);
}

TEST_F(EchoHandlerTest, HandlesEmptyRequest) {
  // Create an empty request
  std::string raw_request = "";
  
  Request request(raw_request);
  Response response;
  
  // Call the handler
  bool result = handler_.HandleRequest(request, &response);
  
  // Check the result
  EXPECT_TRUE(result);
  
  // Check the response status
  EXPECT_EQ(response.status(), Response::OK);
  
  // Check the content type
  const auto& headers = response.headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "text/plain");
  
  // Check that the body contains the raw request (which is empty)
  EXPECT_EQ(response.body(), raw_request);
}

TEST_F(EchoHandlerTest, AllResponseHeadersPresent) {
  std::string raw_request = "GET /echo HTTP/1.1\r\n\r\n";
  
  Request request(raw_request);
  Response response;
  
  handler_.HandleRequest(request, &response);
  
  // Ensure all expected headers are present
  const auto& headers = response.headers();
  EXPECT_NE(headers.find("Content-Type"), headers.end());
  EXPECT_NE(headers.find("Content-Length"), headers.end());
  EXPECT_NE(headers.find("Connection"), headers.end());
  
  // Check response conversion to string
  std::string response_str = response.to_string();
  EXPECT_TRUE(response_str.find("HTTP/1.1 200 OK") != std::string::npos);
  EXPECT_TRUE(response_str.find("Content-Type: text/plain") != std::string::npos);
  EXPECT_TRUE(response_str.find("Connection: close") != std::string::npos);
} 