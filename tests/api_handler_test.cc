#include "gtest/gtest.h"
#include "handlers/api_handler.h"
#include "request.h"
#include "response.h"
#include "mime_types.h"
#include "handler_registry.h"
#include "real_file_system.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

namespace fs = std::filesystem;

/**
 * @brief Test fixture for ApiHandler tests
 * 
 * Creates a temporary directory structure with test files
 * to verify the functionality of the API handler.
 */
class ApiHandlerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Register the ApiHandler with the registry
    //HandlerRegistry::RegisterHandler("ApiHandler", ApiHandler::Create);
    
    // Create a temporary directory for test files
    temp_dir_ = fs::temp_directory_path() / "api_handler_test";
    fs::create_directories(temp_dir_);
    
    // The handler constructor takes (serving_path, root_directory)
    RealFileSystem rfs;
    handler_ = std::make_unique<ApiHandler>("/api", temp_dir_.string(), rfs);
    
    // Create test directories directly under temp_dir_
    std::string products_dir = (temp_dir_ / "products").string();
    fs::create_directories(products_dir);
    
    // Add test files to the products directory
    std::ofstream product1_file((products_dir + "/1").c_str());
    product1_file << "{\"id\": 1, \"name\": \"Product 1\", \"price\": 19.99}";
    product1_file.close();
    
    std::ofstream product2_file((products_dir + "/2").c_str());
    product2_file << "{\"id\": 2, \"name\": \"Product 2\", \"price\": 29.99}";
    product2_file.close();
  }
  
  void TearDown() override {
    // Clean up temporary directory and files
    fs::remove_all(temp_dir_);
  }
  
  fs::path temp_dir_;
  std::unique_ptr<ApiHandler> handler_;
};

// Test GET request for a single resource
TEST_F(ApiHandlerTest, GetSingleResource) {
  // Create a request for a specific product
  std::string raw_request = 
      "GET /api/products/1 HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::OK);
  
  // Check the content type
  const auto& headers = response->headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "application/json");
  
  // Check the body content
  EXPECT_EQ(response->body(), "{\"id\": 1, \"name\": \"Product 1\", \"price\": 19.99}");
}

// Test GET request for a collection (directory listing)
TEST_F(ApiHandlerTest, GetCollection) {
  // Create a request for all products
  std::string raw_request = 
      "GET /api/products HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::OK);
  
  // Check the content type
  const auto& headers = response->headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "application/json");
  
  // Check the body content contains file IDs (order might vary, so check for substrings)
  const std::string& body = response->body();
  EXPECT_NE(body.find("\"file_ids\""), std::string::npos);
  EXPECT_NE(body.find("1"), std::string::npos);
  EXPECT_NE(body.find("2"), std::string::npos);
}

// Test GET request for non-existent resource
TEST_F(ApiHandlerTest, GetNonExistentResource) {
  // Create a request for a non-existent product
  std::string raw_request = 
      "GET /api/products/999 HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::NOT_FOUND);
}

// Test invalid API path
TEST_F(ApiHandlerTest, InvalidApiPath) {
  // Create a request with a path that doesn't start with /api/
  std::string raw_request = 
      "GET /products/1 HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::FORBIDDEN);
}