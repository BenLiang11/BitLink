#include "gtest/gtest.h"
#include "handlers/echo_handler.h"
#include "request.h"
#include "response.h"
#include "handler_registry.h"
#include <string>
#include <memory>

/**
 * @brief Test fixture for EchoHandler tests
 * 
 * Tests the functionality of the echo handler including
 * request handling, factory methods, and registration.
 */
class EchoHandlerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Register the EchoHandler with the registry
    ASSERT_TRUE(HandlerRegistry::RegisterHandler("EchoHandler", EchoHandler::Create));
  }
  
  EchoHandler handler_;
};

/**
 * @brief Test that echo handler correctly reflects basic requests
 */
TEST_F(EchoHandlerTest, HandlesBasicRequest) {
  // Create a test request
  std::string raw_request = 
      "GET /echo HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "User-Agent: Mozilla/5.0\r\n"
      "Accept: text/html\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_.handle_request(request);
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::OK);
  
  // Check the content type
  const auto& headers = response->headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "text/plain");
  
  // Check that the body contains the raw request
  EXPECT_EQ(response->body(), request.body());
}

/**
 * @brief Test that echo handler correctly handles requests with bodies
 */
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
  
  // Call the handler
  std::unique_ptr<Response> response = handler_.handle_request(request);
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::OK);
  
  // Check that the body contains the request body
  EXPECT_EQ(response->body(), request.body());
}

/**
 * @brief Test that echo handler correctly handles empty requests
 */
TEST_F(EchoHandlerTest, HandlesEmptyRequest) {
  // Create an empty request
  std::string raw_request = "";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_.handle_request(request);
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::OK);
  
  // Check the content type
  const auto& headers = response->headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "text/plain");
  
  // Check that the body contains the empty request body
  EXPECT_EQ(response->body(), request.body());
}

/**
 * @brief Test the factory method and handler registry integration
 */
TEST_F(EchoHandlerTest, TestFactoryMethod) {
  // Test the Create factory method with no arguments (valid case)
  std::vector<std::string> no_args;
  std::unique_ptr<RequestHandler> handler = EchoHandler::Create(no_args);
  ASSERT_NE(handler, nullptr);
  
  // Test with arguments (should throw)
  std::vector<std::string> some_args = {"arg1"};
  EXPECT_THROW(EchoHandler::Create(some_args), std::invalid_argument);
}

/**
 * @brief Test handler creation through the registry
 */
TEST_F(EchoHandlerTest, TestHandlerRegistryCreation) {
  // Create a handler using the registry
  std::vector<std::string> args;
  auto handler = HandlerRegistry::CreateHandler("EchoHandler", args);
  
  // Verify handler was created successfully
  ASSERT_NE(handler, nullptr);
  
  // Verify it's an EchoHandler by checking its behavior
  std::string raw_request = "GET /echo HTTP/1.1\r\nHost: localhost\r\n\r\n";
  Request request(raw_request);
  auto response = handler->handle_request(request);
  
  ASSERT_NE(response, nullptr);
  EXPECT_EQ(response->status(), Response::OK);
  EXPECT_EQ(response->headers().at("Content-Type"), "text/plain");
}

/**
 * @brief Test handler creation with invalid arguments
 */
TEST_F(EchoHandlerTest, TestHandlerCreationWithInvalidArgs) {
  // Attempt to create with invalid args
  std::vector<std::string> invalid_args = {"unexpected", "args"};
  EXPECT_THROW(HandlerRegistry::CreateHandler("EchoHandler", invalid_args), std::invalid_argument);
  
  // Attempt to create with too many args
  std::vector<std::string> too_many_args = {"too", "many", "args", "here"};
  EXPECT_THROW(HandlerRegistry::CreateHandler("EchoHandler", too_many_args), std::invalid_argument);
} 