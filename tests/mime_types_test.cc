#include "gtest/gtest.h"
#include "mime_types.h"
#include <string>

class MimeTypesTest : public ::testing::Test {};

TEST_F(MimeTypesTest, CommonMimeTypes) {
  // Test common MIME types
  EXPECT_EQ(MimeTypes::GetMimeType("html"), "text/html");
  EXPECT_EQ(MimeTypes::GetMimeType("htm"), "text/html");
  EXPECT_EQ(MimeTypes::GetMimeType("css"), "text/css");
  EXPECT_EQ(MimeTypes::GetMimeType("txt"), "text/plain");
  EXPECT_EQ(MimeTypes::GetMimeType("md"), "text/markdown");
  EXPECT_EQ(MimeTypes::GetMimeType("js"), "application/javascript");
  EXPECT_EQ(MimeTypes::GetMimeType("json"), "application/json");
}

TEST_F(MimeTypesTest, ImageMimeTypes) {
  // Test image MIME types
  EXPECT_EQ(MimeTypes::GetMimeType("png"), "image/png");
  EXPECT_EQ(MimeTypes::GetMimeType("jpg"), "image/jpeg");
  EXPECT_EQ(MimeTypes::GetMimeType("jpeg"), "image/jpeg");
  EXPECT_EQ(MimeTypes::GetMimeType("gif"), "image/gif");
  EXPECT_EQ(MimeTypes::GetMimeType("svg"), "image/svg+xml");
  EXPECT_EQ(MimeTypes::GetMimeType("ico"), "image/x-icon");
}

TEST_F(MimeTypesTest, ApplicationMimeTypes) {
  // Test application MIME types
  EXPECT_EQ(MimeTypes::GetMimeType("pdf"), "application/pdf");
  EXPECT_EQ(MimeTypes::GetMimeType("zip"), "application/zip");
  EXPECT_EQ(MimeTypes::GetMimeType("tar"), "application/x-tar");
  EXPECT_EQ(MimeTypes::GetMimeType("gz"), "application/gzip");
}

TEST_F(MimeTypesTest, MediaMimeTypes) {
  // Test media MIME types
  EXPECT_EQ(MimeTypes::GetMimeType("mp3"), "audio/mpeg");
  EXPECT_EQ(MimeTypes::GetMimeType("mp4"), "video/mp4");
}

TEST_F(MimeTypesTest, CaseInsensitivity) {
  // Test case insensitivity
  EXPECT_EQ(MimeTypes::GetMimeType("HTML"), "text/html");
  EXPECT_EQ(MimeTypes::GetMimeType("Html"), "text/html");
  EXPECT_EQ(MimeTypes::GetMimeType("PNG"), "image/png");
  EXPECT_EQ(MimeTypes::GetMimeType("JPG"), "image/jpeg");
  EXPECT_EQ(MimeTypes::GetMimeType("PDF"), "application/pdf");
}

TEST_F(MimeTypesTest, UnknownExtension) {
  // Test unknown extensions
  EXPECT_EQ(MimeTypes::GetMimeType("unknown"), "application/octet-stream");
  EXPECT_EQ(MimeTypes::GetMimeType("xyz"), "application/octet-stream");
  EXPECT_EQ(MimeTypes::GetMimeType("abc123"), "application/octet-stream");
  EXPECT_EQ(MimeTypes::GetMimeType(""), "application/octet-stream");
}

TEST_F(MimeTypesTest, DotInExtension) {
  // Test extensions that might include a dot
  EXPECT_EQ(MimeTypes::GetMimeType(".html"), "application/octet-stream"); // Should only match the extension part
  EXPECT_EQ(MimeTypes::GetMimeType("file.html"), "application/octet-stream"); // Should only match the extension part
} 