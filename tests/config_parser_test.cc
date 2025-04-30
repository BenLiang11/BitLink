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
  bool success = parser.Parse("test_configs/null_file", &out_config);
  EXPECT_FALSE(success);
}

// Test 2: Illegal token ($)
TEST_F(NginxConfigParserTestFixture, IllegalToken) {
  bool success = parser.Parse("test_configs/illegal_token_config", &out_config);
  EXPECT_FALSE(success);
}
// Test 3: Empty file
TEST_F(NginxConfigParserTestFixture, emptyConfig) {
  bool success = parser.Parse("test_configs/empty_config", &out_config);
  EXPECT_TRUE(success);
}

// Test 4: 
TEST_F(NginxConfigParserTestFixture, nestedLoopConfig) {
  bool success = parser.Parse("test_configs/nested_loop_config", &out_config);
  EXPECT_TRUE(success);
}

// Test 5: separator char after quotes (1st TODO from Assignment 1)
TEST_F(NginxConfigParserTestFixture, spaceBeforeColonConfig) {
  bool success = parser.Parse("test_configs/space_before_colon_config", &out_config);
  EXPECT_TRUE(success);
}
// Test 6: invalid char after quotes (1st TODO from Assignment 1)
TEST_F(NginxConfigParserTestFixture, numberBeforeColonConfig) {
  bool success = parser.Parse("test_configs/number_before_colon_config", &out_config);
  EXPECT_FALSE(success);
}
// Test 7: empty statements {}
TEST_F(NginxConfigParserTestFixture, emptyStatementsConfig) {
  bool success = parser.Parse("test_configs/empty_statements_config", &out_config);
  std::cout << "HELLOO" << std::endl; // adds a space after "/" in the example txt file
  EXPECT_TRUE(success);
}

// Test 8: backslash handling (3rd TODO from Assignment 1)
TEST_F(NginxConfigParserTestFixture, backslashEscapingConfig) {
  bool success = parser.Parse("test_configs/backslash_escaping_config", &out_config);
  EXPECT_TRUE(success);
  //  expect: "hel'lo"
  EXPECT_EQ(out_config.statements_[0]->tokens_[1],"\"hel'lo\""); // string of "hel'lo"
}

// Test 9: left parenthesis check
TEST_F(NginxConfigParserTestFixture, invalidLeftParenthesisConfig) {
  bool success = parser.Parse("test_configs/invalid_left_parenthesis_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 10: right parenthesis check
TEST_F(NginxConfigParserTestFixture, invalidRightParenthesisConfig) {
  bool success = parser.Parse("test_configs/invalid_right_parenthesis_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 11: single quotes
TEST_F(NginxConfigParserTestFixture, singleQuoteConfig) {
  bool success = parser.Parse("test_configs/single_quote_config", &out_config);
  EXPECT_TRUE(success);
}

// Test 12: double quotes
TEST_F(NginxConfigParserTestFixture, doubleQuoteConfig) {
  bool success = parser.Parse("test_configs/double_quote_config", &out_config);
  EXPECT_TRUE(success);
}

// Test 13: single quote error
TEST_F(NginxConfigParserTestFixture, singleQuoteErrorConfig) {
  bool success = parser.Parse("test_configs/single_quote_error_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 14: double quote error
TEST_F(NginxConfigParserTestFixture, doubleQuoteErrorConfig) {
  bool success = parser.Parse("test_configs/double_quote_error_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 15
TEST_F(NginxConfigParserTestFixture, eofSingleQuoteConfig) {
  bool success = parser.Parse("test_configs/eof_single_quote_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 16
TEST_F(NginxConfigParserTestFixture, eofDoubleQuoteConfig) {
  bool success = parser.Parse("test_configs/eof_double_quote_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 17
TEST_F(NginxConfigParserTestFixture, statementEndErrorConfig) {
  bool success = parser.Parse("test_configs/statement_end_error_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 18
TEST_F(NginxConfigParserTestFixture, startBlockErrorConfig) {
  bool success = parser.Parse("test_configs/start_block_error_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 19
TEST_F(NginxConfigParserTestFixture, endBlockErrorConfig) {
  bool success = parser.Parse("test_configs/end_block_error_config", &out_config);
  EXPECT_FALSE(success);
}

// Test 20
TEST_F(NginxConfigParserTestFixture, notInlineComment) {
  bool success = parser.Parse("test_configs/not_inline_comment_config", &out_config);
  EXPECT_TRUE(success);
}

// Test 21 no increase
TEST_F(NginxConfigParserTestFixture, NginxConfigToString) {
  bool success = parser.Parse("test_configs/to_string_config", &out_config);
  std::string res =  "foo bar;\nserver {\n  port 8080;\n  server_name foo.com;\n  root /home/ubuntu/sites/foo/;\n}\n";
  std::string config_string = out_config.ToString();
  bool isSame = config_string.compare(res)==0; //not same => not 1 => FALSE
  EXPECT_TRUE(isSame);
}
//Test 22 no increase
TEST_F(NginxConfigParserTestFixture, emptyToString) {
  bool success = parser.Parse("test_configs/empty_config", &out_config);
  std::string res =  "bar;\nserver {\n  port 8080;\n  server_name foo.com;\n  root /home/ubuntu/sites/foo/;\n}\n";
  std::string config_string = out_config.ToString();
  bool isSame = config_string.compare(res)==1; //not same => not 1 => FALSE
  EXPECT_FALSE(isSame);
}
