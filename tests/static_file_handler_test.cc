#include "gtest/gtest.h"
#include "handlers/static_file_handler.h"
#include "request.h"
#include "response.h"
#include "mime_types.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

class StaticFileHandlerTest : public ::testing::Test {
protected:
  void SetUp() override {
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
    
    // Initialize the handler with the temp directory
    handler_ = std::make_unique<StaticFileHandler>(temp_dir_.string());
    
    // Create an alternate handler for testing root directory with trailing slash
    std::string root_with_slash = temp_dir_.string() + "/";
    handler_with_slash_ = std::make_unique<StaticFileHandler>(root_with_slash);
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
      "GET /index.html HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  Response response;
  
  // Call the handler
  bool result = handler_->HandleRequest(request, &response);
  
  // Check the result
  EXPECT_TRUE(result);
  
  // Check the response status
  EXPECT_EQ(response.status(), Response::OK);
  
  // Check the content type
  const auto& headers = response.headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "text/html");
  
  // Check the body content
  EXPECT_EQ(response.body(), "<html><body><h1>Test Page</h1></body></html>");
}

TEST_F(StaticFileHandlerTest, ServeTxtFile) {
  // Create a request for test.txt
  std::string raw_request = 
      "GET /test.txt HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  Response response;
  
  // Call the handler
  bool result = handler_->HandleRequest(request, &response);
  
  // Check the result
  EXPECT_TRUE(result);
  
  // Check the response status
  EXPECT_EQ(response.status(), Response::OK);
  
  // Check the content type
  const auto& headers = response.headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "text/plain");
  
  // Check the body content
  EXPECT_EQ(response.body(), "This is a test file.");
}

TEST_F(StaticFileHandlerTest, ServeImageFile) {
  // Create a request for test.jpg
  std::string raw_request = 
      "GET /test.jpg HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  Response response;
  
  // Call the handler
  bool result = handler_->HandleRequest(request, &response);
  
  // Check the result
  EXPECT_TRUE(result);
  
  // Check the response status
  EXPECT_EQ(response.status(), Response::OK);
  
  // Check the content type
  const auto& headers = response.headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "image/jpeg");
}

TEST_F(StaticFileHandlerTest, ServePngFile) {
  // Create a request for test.png
  std::string raw_request = 
      "GET /test.png HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  Response response;
  
  // Call the handler
  bool result = handler_->HandleRequest(request, &response);
  
  // Check the result
  EXPECT_TRUE(result);
  
  // Check the response status
  EXPECT_EQ(response.status(), Response::OK);
  
  // Check the content type
  const auto& headers = response.headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "image/png");
}

TEST_F(StaticFileHandlerTest, ServeFileInSubdirectory) {
  // Create a request for subdirectory file
  std::string raw_request = 
      "GET /subdir/file.txt HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  Response response;
  
  // Call the handler
  bool result = handler_->HandleRequest(request, &response);
  
  // Check the result
  EXPECT_TRUE(result);
  
  // Check the response status
  EXPECT_EQ(response.status(), Response::OK);
  
  // Check the content type
  const auto& headers = response.headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "text/plain");
  
  // Check the body content
  EXPECT_EQ(response.body(), "File in subdirectory");
}

TEST_F(StaticFileHandlerTest, ServeIndexForDirectory) {
  // Create a request for the root directory 
  std::string raw_request = 
      "GET / HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  Response response;
  
  // Call the handler
  bool result = handler_->HandleRequest(request, &response);
  
  // Check the result
  EXPECT_TRUE(result);
  
  // Check the response status
  EXPECT_EQ(response.status(), Response::OK);
  
  // Check the content type
  const auto& headers = response.headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "text/html");
  
  // Should serve index.html by default
  EXPECT_EQ(response.body(), "<html><body><h1>Test Page</h1></body></html>");
}

TEST_F(StaticFileHandlerTest, ServeDirectoryWithTrailingSlash) {
  // Create a request for a subdirectory with trailing slash
  std::string raw_request = 
      "GET /subdir/ HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  Response response;
  
  // This should fail since we don't have an index.html in the subdirectory
  bool result = handler_->HandleRequest(request, &response);
  
  // Check that the request failed
  EXPECT_FALSE(result);
  
  // Check the response status
  EXPECT_EQ(response.status(), Response::NOT_FOUND);
}

TEST_F(StaticFileHandlerTest, UnknownExtensionDefaultsToOctetStream) {
  // Create a file with unknown extension
  std::string unknown_path = (temp_dir_ / "unknown.xyz").string();
  std::ofstream unknown_file(unknown_path);
  unknown_file << "File with unknown extension";
  unknown_file.close();
  
  // Create a request for the unknown file
  std::string raw_request = 
      "GET /unknown.xyz HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  Response response;
  
  // Call the handler
  bool result = handler_->HandleRequest(request, &response);
  
  // Check the result
  EXPECT_TRUE(result);
  
  // Check the response status
  EXPECT_EQ(response.status(), Response::OK);
  
  // Check the content type
  const auto& headers = response.headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "application/octet-stream");
}

TEST_F(StaticFileHandlerTest, FileNotFound) {
  // Create a request for a non-existent file
  std::string raw_request = 
      "GET /nonexistent.html HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  Response response;
  
  // Call the handler
  bool result = handler_->HandleRequest(request, &response);
  
  // Check the result (should be false for failure)
  EXPECT_FALSE(result);
  
  // Check the response status (should be 404 Not Found)
  EXPECT_EQ(response.status(), Response::NOT_FOUND);
  
  // Check the content type (should be text/html for the error page)
  const auto& headers = response.headers();
  auto content_type_it = headers.find("Content-Type");
  EXPECT_NE(content_type_it, headers.end());
  EXPECT_EQ(content_type_it->second, "text/html");
  
  // Check that the body contains a 404 message
  EXPECT_NE(response.body().find("404 Not Found"), std::string::npos);
}

TEST_F(StaticFileHandlerTest, InvalidMethod) {
  // Create a POST request (which is not supported)
  std::string raw_request = 
      "POST /index.html HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  Response response;
  
  // Call the handler
  bool result = handler_->HandleRequest(request, &response);
  
  // Check the result (should be false for failure)
  EXPECT_FALSE(result);
  
  // Check the response status (should be 404 Not Found)
  EXPECT_EQ(response.status(), Response::NOT_FOUND);
}

TEST_F(StaticFileHandlerTest, RootDirectoryWithTrailingSlash) {
  // Create a request for index.html
  std::string raw_request = 
      "GET /index.html HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "\r\n";
  
  Request request(raw_request);
  Response response;
  
  // Call the handler with trailing slash
  bool result = handler_with_slash_->HandleRequest(request, &response);
  
  // Check the result
  EXPECT_TRUE(result);
  
  // Check the response status
  EXPECT_EQ(response.status(), Response::OK);
  
  // Check the body content
  EXPECT_EQ(response.body(), "<html><body><h1>Test Page</h1></body></html>");
}

// Direct tests for the StaticFileHandler methods
TEST_F(StaticFileHandlerTest, TestMimeTypeUtility) {
  // These can be tested directly by accessing GetMimeType through the public API
  // Create handler to test with
  StaticFileHandler handler(temp_dir_.string());
  
  // Test various file extensions - these tests and those above will drive 
  // more code coverage for the mime_types.cc file
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