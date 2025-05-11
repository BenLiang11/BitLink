#include "gtest/gtest.h"
#include "handlers/static_file_handler.h"
#include "request.h"
#include "response.h"
#include "mime_types.h"
#include "handler_registry.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <memory>

namespace fs = std::filesystem;

/**
 * @brief Test fixture for StaticFileHandler tests
 * 
 * Creates a temporary directory structure with test files
 * to verify the functionality of the static file handler.
 */
class StaticFileHandlerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Register the StaticFileHandler with the registry
    ASSERT_TRUE(HandlerRegistry::RegisterHandler("StaticHandler", StaticFileHandler::Create));
    
    // Create a temporary directory for test files
    temp_dir_ = fs::temp_directory_path() / "static_file_handler_test";
    fs::create_directories(temp_dir_);
    
    // Create a test HTML file
    std::string html_path = (temp_dir_ / "index.html").string();
    std::ofstream html_file(html_path);
    html_file << "<html><body><h1>Test Page</h1></body></html>";
    html_file.close();
    
    // Create a test text file
    std::string txt_path = (temp_dir_ / "test.txt").string();
    std::ofstream txt_file(txt_path);
    txt_file << "This is a test file.";
    txt_file.close();
    
    // Create a subdirectory with a file
    std::string subdir_path = (temp_dir_ / "subdir").string();
    fs::create_directories(subdir_path);
    std::string subdir_file_path = (temp_dir_ / "subdir" / "file.txt").string();
    std::ofstream subdir_file(subdir_file_path);
    subdir_file << "File in subdirectory";
    subdir_file.close();
    
    // Create a JPG file (just a small binary file for testing mime type)
    std::string jpg_path = (temp_dir_ / "test.jpg").string();
    std::ofstream jpg_file(jpg_path, std::ios::binary);
    unsigned char jpg_data[] = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 'J', 'F', 'I', 'F'};
    jpg_file.write(reinterpret_cast<const char*>(jpg_data), sizeof(jpg_data));
    jpg_file.close();
    
    // Create a PNG file (just a small binary file for testing mime type)
    std::string png_path = (temp_dir_ / "test.png").string();
    std::ofstream png_file(png_path, std::ios::binary);
    unsigned char png_data[] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    png_file.write(reinterpret_cast<const char*>(png_data), sizeof(png_data));
    png_file.close();
    
    // Initialize the handler with the temp directory and serving path
    handler_ = std::make_unique<StaticFileHandler>("/static", temp_dir_.string());
    
    // Create an alternate handler for testing root directory with trailing slash
    std::string root_with_slash = temp_dir_.string() + "/";
    handler_with_slash_ = std::make_unique<StaticFileHandler>("/static", root_with_slash);
  }
  
  void TearDown() override {
    // Clean up temporary directory and files
    fs::remove_all(temp_dir_);
  }
  
  fs::path temp_dir_;
  std::unique_ptr<StaticFileHandler> handler_;
  std::unique_ptr<StaticFileHandler> handler_with_slash_;
};

TEST_F(StaticFileHandlerTest, ServeHtmlFile) {
  // Create a request for index.html
  std::string raw_request = 
      "GET /static/index.html HTTP/1.1\r\n"
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
  EXPECT_EQ(content_type_it->second, "text/html");
  
  // Check the body content
  EXPECT_EQ(response->body(), "<html><body><h1>Test Page</h1></body></html>");
}

TEST_F(StaticFileHandlerTest, ServeTxtFile) {
  // Create a request for test.txt
  std::string raw_request = 
      "GET /static/test.txt HTTP/1.1\r\n"
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
  EXPECT_EQ(content_type_it->second, "text/plain");
  
  // Check the body content
  EXPECT_EQ(response->body(), "This is a test file.");
}

TEST_F(StaticFileHandlerTest, ServeImageFile) {
  // Create a request for test.jpg
  std::string raw_request = 
      "GET /static/test.jpg HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check the result
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::OK);
  
  // Check the content type
  const auto& headers = response->headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "image/jpeg");
}

TEST_F(StaticFileHandlerTest, ServePngFile) {
  // Create a request for test.png
  std::string raw_request = 
      "GET /static/test.png HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check the result
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::OK);
  
  // Check the content type
  const auto& headers = response->headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "image/png");
}

TEST_F(StaticFileHandlerTest, ServeFileInSubdirectory) {
  // Create a request for subdirectory file
  std::string raw_request = 
      "GET /static/subdir/file.txt HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check the result
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::OK);
  
  // Check the content type
  const auto& headers = response->headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "text/plain");
  
  // Check the body content
  EXPECT_EQ(response->body(), "File in subdirectory");
}

TEST_F(StaticFileHandlerTest, ServeIndexForDirectory) {
  // Create a request for the root directory 
  std::string raw_request = 
      "GET /static/ HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check the result
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::OK);
  
  // Check the content type
  const auto& headers = response->headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "text/html");
  
  // Should serve index.html by default
  EXPECT_EQ(response->body(), "<html><body><h1>Test Page</h1></body></html>");
}

TEST_F(StaticFileHandlerTest, ServeDirectoryWithTrailingSlash) {
  // Create a request for a subdirectory with trailing slash
  std::string raw_request = 
      "GET /static/subdir/ HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // This should fail since we don't have an index.html in the subdirectory
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check that the request failed
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::NOT_FOUND);
}

TEST_F(StaticFileHandlerTest, UnknownExtensionDefaultsToOctetStream) {
  // Create a file with unknown extension
  std::string unknown_path = (temp_dir_ / "unknown.xyz").string();
  std::ofstream unknown_file(unknown_path);
  unknown_file << "File with unknown extension";
  unknown_file.close();
  
  // Create a request for the unknown file
  std::string raw_request = 
      "GET /static/unknown.xyz HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check the result
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::OK);
  
  // Check the content type
  const auto& headers = response->headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "application/octet-stream");
}

TEST_F(StaticFileHandlerTest, FileNotFound) {
  // Create a request for a non-existent file
  std::string raw_request = 
      "GET /static/nonexistent.html HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  
  // Check the response status (should be 404 Not Found)
  EXPECT_EQ(response->status(), Response::NOT_FOUND);
  
  // Check the content type (should be text/html for the error page)
  const auto& headers = response->headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "text/html");
  
  // Check that the body contains a 404 message
  EXPECT_NE(response->body().find("404 Not Found"), std::string::npos);
}

TEST_F(StaticFileHandlerTest, InvalidMethod) {
  // Create a POST request (which is not supported)
  std::string raw_request = 
      "POST /static/index.html HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler
  std::unique_ptr<Response> response = handler_->handle_request(request);
  
  // Check that we got a response
  ASSERT_NE(response, nullptr);
  
  // Check the response status (should be 404 Not Found or 405 Method Not Allowed)
  EXPECT_EQ(response->status(), Response::NOT_FOUND);
}

TEST_F(StaticFileHandlerTest, RootDirectoryWithTrailingSlash) {
  // Create a request for index.html
  std::string raw_request = 
      "GET /static/index.html HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  
  // Call the handler with trailing slash
  std::unique_ptr<Response> response = handler_with_slash_->handle_request(request);
  
  // Check the result
  ASSERT_NE(response, nullptr);
  
  // Check the response status
  EXPECT_EQ(response->status(), Response::OK);
  
  // Check the body content
  EXPECT_EQ(response->body(), "<html><body><h1>Test Page</h1></body></html>");
}

// Direct tests for the StaticFileHandler methods
TEST_F(StaticFileHandlerTest, TestMimeTypeUtility) {
  // Create handler to test with
  StaticFileHandler handler("/static", temp_dir_.string());
  
  // Test various file extensions
  EXPECT_EQ(MimeTypes::GetMimeType("html"), "text/html");
  EXPECT_EQ(MimeTypes::GetMimeType("txt"), "text/plain");
  EXPECT_EQ(MimeTypes::GetMimeType("jpg"), "image/jpeg");
  EXPECT_EQ(MimeTypes::GetMimeType("jpeg"), "image/jpeg");
  EXPECT_EQ(MimeTypes::GetMimeType("png"), "image/png");
  EXPECT_EQ(MimeTypes::GetMimeType("gif"), "image/gif");
  EXPECT_EQ(MimeTypes::GetMimeType("css"), "text/css");
  EXPECT_EQ(MimeTypes::GetMimeType("js"), "application/javascript");
  EXPECT_EQ(MimeTypes::GetMimeType("json"), "application/json");
  EXPECT_EQ(MimeTypes::GetMimeType("pdf"), "application/pdf");
  EXPECT_EQ(MimeTypes::GetMimeType("zip"), "application/zip");
  EXPECT_EQ(MimeTypes::GetMimeType("unknown"), "application/octet-stream");
  
  // Test case insensitivity
  EXPECT_EQ(MimeTypes::GetMimeType("HTML"), "text/html");
  EXPECT_EQ(MimeTypes::GetMimeType("Jpg"), "image/jpeg");
}

TEST_F(StaticFileHandlerTest, TestFactoryMethod) {
  // Test the Create factory method with valid arguments
  std::vector<std::string> valid_args = {"/static", temp_dir_.string()};
  std::unique_ptr<RequestHandler> handler = StaticFileHandler::Create(valid_args);
  ASSERT_NE(handler, nullptr);
  
  // Test with missing arguments (should throw)
  std::vector<std::string> missing_args = {"/static"};
  EXPECT_THROW(StaticFileHandler::Create(missing_args), std::invalid_argument);
  
  // Test with invalid path (no leading slash)
  std::vector<std::string> invalid_args = {"static", temp_dir_.string()};
  EXPECT_THROW(StaticFileHandler::Create(invalid_args), std::invalid_argument);
}

/**
 * @brief Test StaticFileHandler factory method with valid arguments
 */
TEST_F(StaticFileHandlerTest, TestFactoryMethodValidArgs) {
  // Create a handler using the factory method
  std::vector<std::string> args = {"/static", temp_dir_.string()};
  auto handler = StaticFileHandler::Create(args);
  
  // Verify it was created successfully
  ASSERT_NE(handler, nullptr);
  
  // Test it with a request
  std::string raw_request = "GET /static/test.txt HTTP/1.1\r\nHost: localhost\r\n\r\n";
  Request request(raw_request);
  auto response = handler->handle_request(request);
  
  // Verify correct response
  ASSERT_NE(response, nullptr);
  EXPECT_EQ(response->status(), Response::OK);
  EXPECT_EQ(response->body(), "This is a test file.");
}

/**
 * @brief Test StaticFileHandler factory method with invalid arguments
 */
TEST_F(StaticFileHandlerTest, TestFactoryMethodInvalidArgs) {
  // Test with too few arguments
  std::vector<std::string> too_few_args = {"/static"};
  EXPECT_THROW(StaticFileHandler::Create(too_few_args), std::invalid_argument);
  
  // Test with too many arguments
  std::vector<std::string> too_many_args = {"/static", temp_dir_.string(), "extra_arg"};
  EXPECT_THROW(StaticFileHandler::Create(too_many_args), std::invalid_argument);
  
  // Test with empty location
  std::vector<std::string> empty_location = {"", temp_dir_.string()};
  EXPECT_THROW(StaticFileHandler::Create(empty_location), std::invalid_argument);
  
  // Test with empty root
  std::vector<std::string> empty_root = {"/static", ""};
  EXPECT_THROW(StaticFileHandler::Create(empty_root), std::invalid_argument);
}

/**
 * @brief Test StaticFileHandler creation through the registry
 */
TEST_F(StaticFileHandlerTest, TestHandlerRegistryCreation) {
  // Create a handler using the registry
  std::vector<std::string> args = {"/static", temp_dir_.string()};
  auto handler = HandlerRegistry::CreateHandler("StaticHandler", args);
  
  // Verify handler was created successfully
  ASSERT_NE(handler, nullptr);
  
  // Verify it's a StaticFileHandler by checking its behavior
  std::string raw_request = "GET /static/test.txt HTTP/1.1\r\nHost: localhost\r\n\r\n";
  Request request(raw_request);
  auto response = handler->handle_request(request);
  
  ASSERT_NE(response, nullptr);
  EXPECT_EQ(response->status(), Response::OK);
  EXPECT_EQ(response->body(), "This is a test file.");
}

/**
 * @brief Test StaticFileHandler URL mapping with non-existent files
 */
TEST_F(StaticFileHandlerTest, TestNonExistentFile) {
  // Request a file that doesn't exist
  std::string raw_request = "GET /static/nonexistent.txt HTTP/1.1\r\nHost: localhost\r\n\r\n";
  Request request(raw_request);
  
  // Should return 404
  std::unique_ptr<Response> response = handler_->handle_request(request);
  ASSERT_NE(response, nullptr);
  EXPECT_EQ(response->status(), Response::NOT_FOUND);
}

/**
 * @brief Test path traversal attack protection
 */
TEST_F(StaticFileHandlerTest, TestDirectoryTraversal) {
  // Request a file trying to traverse outside the root directory
  std::string raw_request = "GET /static/../../../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n";
  Request request(raw_request);
  
  // Should return 403 Forbidden
  std::unique_ptr<Response> response = handler_->handle_request(request);
  ASSERT_NE(response, nullptr);
  EXPECT_EQ(response->status(), Response::FORBIDDEN);
}

/**
 * @brief Test per-request handler instantiation with unique configurations
 */
TEST_F(StaticFileHandlerTest, TestMultipleHandlerInstances) {
  // Create two handlers with different base paths and root directories
  std::vector<std::string> args1 = {"/static1", temp_dir_.string() + "/subdir"};
  auto handler1 = HandlerRegistry::CreateHandler("StaticHandler", args1);
  
  std::vector<std::string> args2 = {"/static2", temp_dir_.string()};
  auto handler2 = HandlerRegistry::CreateHandler("StaticHandler", args2);
  
  // Both handlers should be created successfully
  ASSERT_NE(handler1, nullptr);
  ASSERT_NE(handler2, nullptr);
  
  // Request from first handler
  std::string raw_request1 = "GET /static1/file.txt HTTP/1.1\r\nHost: localhost\r\n\r\n";
  Request request1(raw_request1);
  auto response1 = handler1->handle_request(request1);
  
  // Request from second handler
  std::string raw_request2 = "GET /static2/test.txt HTTP/1.1\r\nHost: localhost\r\n\r\n";
  Request request2(raw_request2);
  auto response2 = handler2->handle_request(request2);
  
  // Verify both responses
  ASSERT_NE(response1, nullptr);
  EXPECT_EQ(response1->status(), Response::OK);
  EXPECT_EQ(response1->body(), "File in subdirectory");
  
  ASSERT_NE(response2, nullptr);
  EXPECT_EQ(response2->status(), Response::OK);
  EXPECT_EQ(response2->body(), "This is a test file.");
} 