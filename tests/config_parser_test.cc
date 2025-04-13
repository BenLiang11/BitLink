#include "gtest/gtest.h"
#include "config_parser.h"

// Fixture
class NginxConfigParserTestFixture : public ::testing::Test {
 protected:
    NginxConfigParser parser;
    NginxConfig out_config;
};

// Test 1: Non existent file
TEST_F(NginxConfigParserTestFixture, NonExistentFile) {
  bool success = parser.Parse("null_file", &out_config);
  EXPECT_FALSE(success);
}

// Test 2: Illegal token ($)
TEST_F(NginxConfigParserTestFixture, IllegalToken) {
  bool success = parser.Parse("test2", &out_config);
  EXPECT_FALSE(success);
}
