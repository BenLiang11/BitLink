#include "gtest/gtest.h"
#include "config_parser.h"
#include <sstream>

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
  bool success = parser.Parse("illegal_token_config", &out_config);
  EXPECT_FALSE(success);
}
// Test 3: Empty file
TEST_F(NginxConfigParserTestFixture, empty_config) {
  bool success = parser.Parse("empty_config", &out_config);
  EXPECT_TRUE(success);
}

// Test 4: 
TEST_F(NginxConfigParserTestFixture, nested_loop_config) {
  bool success = parser.Parse("nested_loop_config", &out_config);
  std::cout << out_config.ToString() << std::endl; // adds a space after "/" in the example txt file
  EXPECT_TRUE(success);
}

// Test 5: separator char after quotes (1st TODO from Assignment 1)
TEST_F(NginxConfigParserTestFixture, space_before_colon_config) {
  bool success = parser.Parse("space_before_colon_config", &out_config);
  EXPECT_TRUE(success);
}
// Test 6: invalid char after quotes (1st TODO from Assignment 1)
TEST_F(NginxConfigParserTestFixture, number_before_colon_config) {
  bool success = parser.Parse("number_before_colon_config", &out_config);
  EXPECT_FALSE(success);
}
// Test 7: empty statements {}
TEST_F(NginxConfigParserTestFixture, empty_statements_config) {
  bool success = parser.Parse("empty_statements_config", &out_config);
  EXPECT_TRUE(success);
}

// Test 8: backslash handling (3rd TODO from Assignment 1)
TEST_F(NginxConfigParserTestFixture, backslash_escaping_config) {
  bool success = parser.Parse("backslash_escaping_config", &out_config);
  EXPECT_TRUE(success);
  //  expect: "hel'lo"
  EXPECT_EQ(out_config.statements_[0]->tokens_[1],"\"hel'lo\""); // string of "hel'lo"
}

// Test 9: left parenthesis check
TEST_F(NginxConfigParserTestFixture, invalid_left_parenthesis_config) {
  bool success = parser.Parse("invalid_left_parenthesis_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 10: right parenthesis check
TEST_F(NginxConfigParserTestFixture, invalid_right_parenthesis_config) {
  bool success = parser.Parse("invalid_right_parenthesis_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 11: single quotes
TEST_F(NginxConfigParserTestFixture, single_quote_config) {
  bool success = parser.Parse("single_quote_config", &out_config);
  EXPECT_TRUE(success);
}

// Test 12: double quotes
TEST_F(NginxConfigParserTestFixture, double_quote_config) {
  bool success = parser.Parse("double_quote_config", &out_config);
  EXPECT_TRUE(success);
}

// Test 13: single quote error
TEST_F(NginxConfigParserTestFixture, single_quote_error_config) {
  bool success = parser.Parse("single_quote_error_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 14: double quote error
TEST_F(NginxConfigParserTestFixture, double_quote_error_config) {
  bool success = parser.Parse("double_quote_error_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 15
TEST_F(NginxConfigParserTestFixture, eof_single_quote_config) {
  bool success = parser.Parse("eof_single_quote_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 16
TEST_F(NginxConfigParserTestFixture, eof_double_quote_config) {
  bool success = parser.Parse("eof_double_quote_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 17
TEST_F(NginxConfigParserTestFixture, statement_end_error_config) {
  bool success = parser.Parse("statement_end_error_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 18
TEST_F(NginxConfigParserTestFixture, start_block_error_config) {
  bool success = parser.Parse("start_block_error_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 19
TEST_F(NginxConfigParserTestFixture, end_block_error_config) {
  bool success = parser.Parse("end_block_error_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 20
TEST_F(NginxConfigParserTestFixture, notInlineComment) {
  bool success = parser.Parse("not_inline_comment_config", &out_config);
  EXPECT_TRUE(success);
}

// Test 21 no increase
TEST_F(NginxConfigParserTestFixture, NginxConfigToString) {
  bool success = parser.Parse("to_string_config", &out_config);
  std::string res =  "foo bar;\nserver {\n  port 8080;\n  server_name foo.com;\n  root /home/ubuntu/sites/foo/;\n}\n";
  std::string config_string = out_config.ToString();
  bool isSame = config_string.compare(res)==0; //not same => not 1 => FALSE
  EXPECT_TRUE(isSame);
}
//Test 22 no increase
TEST_F(NginxConfigParserTestFixture, emptyToString) {
  bool success = parser.Parse("empty_config", &out_config);
  std::string res =  "bar;\nserver {\n  port 8080;\n  server_name foo.com;\n  root /home/ubuntu/sites/foo/;\n}\n";
  std::string config_string = out_config.ToString();
  bool isSame = config_string.compare(res)==1; //not same => not 1 => FALSE
  EXPECT_FALSE(isSame);
}
