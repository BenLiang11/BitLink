#include "gtest/gtest.h"
#include "handlers/api_handler.h"
#include "request.h"
#include "response.h"
#include "mime_types.h"
#include "handler_registry.h"
#include "fake_file_system.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

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
    HandlerRegistry::RegisterHandler("ApiHandler", ApiHandler::Create);
    
    // Create a temporary directory for test files
    temp_dir_ = "/tmp/api_handler_test/";

    // fs::create_directories(temp_dir_);
    
    // The handler constructor takes (serving_path, root_directory)
    handler_ = std::make_unique<ApiHandler>("/api", temp_dir_, fs);
    
    // Create test directories directly under temp_dir_
    std::string products_dir = (temp_dir_ + "products");
    // fs::create_directories(products_dir);
    
    // Add test files to the products directory
    std::stringstream product1_stream;
    std::stringstream product2_stream;
    product1_stream << "{\"id\": 1, \"name\": \"Product 1\", \"price\": 19.99}";
    product2_stream << "{\"id\": 2, \"name\": \"Product 2\", \"price\": 29.99}";
    fs.overwrite_file(products_dir + "/1", product1_stream);
    fs.overwrite_file(products_dir + "/2", product2_stream);
  }
  
  void TearDown() override {

  }
  
  FakeFileSystem fs;
  std::string temp_dir_;
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
  std::cout << "handl request" << std::endl;
  std::unique_ptr<Response> response = handler_->handle_request(request);
  std::cout << "handl request done" << std::endl;
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  std::cout << "response not null" << std::endl;
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::NOT_FOUND);
  std::cout << "response not found" << std::endl;
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

// Test POST request to create a new resource
TEST_F(ApiHandlerTest, PostNewResource) {
  // Create a POST request to add a new product
  std::string raw_request = 
      "POST /api/products HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 53\r\n"
      "\r\n"
      "{\"name\": \"New Product\", \"price\": 39.99, \"inStock\": true}";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::CREATED);
  
  // The response should contain the ID of the newly created resource
  const std::string& body = response->body();
  EXPECT_NE(body.find("\"id\""), std::string::npos);
  EXPECT_NE(body.find("3"), std::string::npos);  // Since we already have products 1 and 2
  
  // Verify the file was actually created
  std::string file_path = (temp_dir_ / "products" / "3").string();
  EXPECT_TRUE(fs::exists(file_path));
  
  // Check the content of the created file
  std::ifstream file(file_path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  EXPECT_EQ(buffer.str(), "{\"name\": \"New Product\", \"price\": 39.99, \"inStock\": true}");
}

// Test PUT request to update an existing resource
TEST_F(ApiHandlerTest, PutExistingResource) {
  // Create a PUT request to update product 1
  std::string raw_request = 
      "PUT /api/products/1 HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 59\r\n"
      "\r\n"
      "{\"id\": 1, \"name\": \"Updated Product\", \"price\": 24.99}";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::OK);
  
  // Verify the file was actually updated
  std::string file_path = (temp_dir_ / "products" / "1").string();
  std::ifstream file(file_path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  EXPECT_EQ(buffer.str(), "{\"id\": 1, \"name\": \"Updated Product\", \"price\": 24.99}");
}

// Test PUT request for non-existent resource
TEST_F(ApiHandlerTest, PutNonExistentResource) {
  // Create a PUT request for a non-existent product
  std::string raw_request = 
      "PUT /api/products/999 HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 60\r\n"
      "\r\n"
      "{\"id\": 999, \"name\": \"Nonexistent Product\", \"price\": 0.99}";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::NOT_FOUND);
}

// Test DELETE request for an existing resource
TEST_F(ApiHandlerTest, DeleteExistingResource) {
  // Create a DELETE request for product 2
  std::string raw_request = 
      "DELETE /api/products/2 HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::OK);
  
  // Verify the file was actually deleted
  std::string file_path = (temp_dir_ / "products" / "2").string();
  EXPECT_FALSE(fs::exists(file_path));
}

// Test DELETE followed by GET request for an existing resource
TEST_F(ApiHandlerTest, DeleteAndGet) {
  // First verify the resource exists
  std::string check_request = 
      "GET /api/products/2 HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request check_req(check_request);
  std::unique_ptr<Response> check_response = handler_->handle_request(check_req);
  ASSERT_NE(check_response, nullptr);
  EXPECT_EQ(check_response->status(), Response::OK);
  
  // Create a DELETE request for product 2
  std::string raw_request = 
      "DELETE /api/products/2 HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::OK);
  
  // Verify the file was actually deleted
  std::string file_path = (temp_dir_ / "products" / "2").string();
  EXPECT_FALSE(fs::exists(file_path));
  
  // Verify the resource is no longer accessible via GET
  std::string get_request = 
      "GET /api/products/2 HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request get_req(get_request);
  std::unique_ptr<Response> get_response = handler_->handle_request(get_req);
  
  // Check GET response - should be NOT_FOUND
  ASSERT_NE(get_response, nullptr);
  EXPECT_EQ(get_response->status(), Response::NOT_FOUND);
  
  // Also verify the product is removed from the collection listing
  std::string list_request = 
      "GET /api/products HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request list_req(list_request);
  std::unique_ptr<Response> list_response = handler_->handle_request(list_req);
  
  ASSERT_NE(list_response, nullptr);
  EXPECT_EQ(list_response->status(), Response::OK);
  const std::string& list_body = list_response->body();
  
  // Find position of "file_ids"
  size_t file_ids_pos = list_body.find("\"file_ids\"");
  // Find position of ID 2 after "file_ids"
  size_t id_pos = list_body.find("2", file_ids_pos);
  
  // If ID 2 is found in the listing response body after "file_ids", the test will fail
  EXPECT_EQ(id_pos, std::string::npos);
}

// Test DELETE request for non-existent resource
TEST_F(ApiHandlerTest, DeleteNonExistentResource) {
  // Create a DELETE request for a non-existent product
  std::string raw_request = 
      "DELETE /api/products/999 HTTP/1.1\r\n"
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