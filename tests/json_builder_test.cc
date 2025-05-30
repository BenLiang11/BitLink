#include <gtest/gtest.h>
#include "utils/json_builder.h"
#include <string>
#include <vector>
#include <map>

using namespace std;

/**
 * @brief Test fixture for JSONBuilder tests
 * 
 * Provides common setup and utility methods for all JSON builder tests.
 */
class JSONBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        builder_.clear();
    }
    
    void TearDown() override {
        // Cleanup if needed
    }
    
    /**
     * @brief Helper to check if JSON contains expected key-value pair
     */
    bool contains_key_value(const string& json, const string& key, const string& expected_value) {
        string search_pattern = "\"" + key + "\":" + expected_value;
        return json.find(search_pattern) != string::npos;
    }
    
    /**
     * @brief Helper to count occurrences of a substring
     */
    int count_occurrences(const string& str, const string& substr) {
        int count = 0;
        size_t pos = 0;
        while ((pos = str.find(substr, pos)) != string::npos) {
            count++;
            pos += substr.length();
        }
        return count;
    }
    
    JSONBuilder builder_;
};

// =============================================================================
// Basic Functionality Tests
// =============================================================================

TEST_F(JSONBuilderTest, Constructor_CreatesEmptyBuilder) {
    EXPECT_TRUE(builder_.empty());
    EXPECT_EQ(builder_.size(), 0);
    EXPECT_EQ(builder_.build(), "{}");
}

TEST_F(JSONBuilderTest, AddString_SingleField_Success) {
    string result = builder_.add("name", "John Doe").build();
    
    EXPECT_FALSE(builder_.empty());
    EXPECT_EQ(builder_.size(), 1);
    EXPECT_TRUE(contains_key_value(result, "name", "\"John Doe\""));
    EXPECT_EQ(result, "{\"name\":\"John Doe\"}");
}

TEST_F(JSONBuilderTest, AddInteger_SingleField_Success) {
    string result = builder_.add("age", 25).build();
    
    EXPECT_EQ(builder_.size(), 1);
    EXPECT_TRUE(contains_key_value(result, "age", "25"));
    EXPECT_EQ(result, "{\"age\":25}");
}

TEST_F(JSONBuilderTest, AddLong_SingleField_Success) {
    string result = builder_.add("timestamp", 1234567890L).build();
    
    EXPECT_EQ(builder_.size(), 1);
    EXPECT_TRUE(contains_key_value(result, "timestamp", "1234567890"));
}

TEST_F(JSONBuilderTest, AddDouble_SingleField_Success) {
    string result = builder_.add("price", 19.99).build();
    
    EXPECT_EQ(builder_.size(), 1);
    EXPECT_TRUE(contains_key_value(result, "price", "19.99"));
}

TEST_F(JSONBuilderTest, AddDouble_RemovesTrailingZeros) {
    string result = builder_.add("value", 10.000).build();
    
    EXPECT_TRUE(contains_key_value(result, "value", "10"));
    
    builder_.clear();
    result = builder_.add("value", 10.100).build();
    EXPECT_TRUE(contains_key_value(result, "value", "10.1"));
}

TEST_F(JSONBuilderTest, AddBoolean_BothValues_Success) {
    string result = builder_.add("active", true)
                            .add("deleted", false)
                            .build();
    
    EXPECT_EQ(builder_.size(), 2);
    EXPECT_TRUE(contains_key_value(result, "active", "true"));
    EXPECT_TRUE(contains_key_value(result, "deleted", "false"));
}

TEST_F(JSONBuilderTest, AddNull_SingleField_Success) {
    string result = builder_.add_null("data").build();
    
    EXPECT_EQ(builder_.size(), 1);
    EXPECT_TRUE(contains_key_value(result, "data", "null"));
}

// =============================================================================
// Array Tests
// =============================================================================

TEST_F(JSONBuilderTest, AddStringArray_EmptyArray_Success) {
    vector<string> empty_array;
    string result = builder_.add_array("tags", empty_array).build();
    
    EXPECT_TRUE(contains_key_value(result, "tags", "[]"));
}

TEST_F(JSONBuilderTest, AddStringArray_MultipleValues_Success) {
    vector<string> tags = {"web", "api", "json"};
    string result = builder_.add_array("tags", tags).build();
    
    EXPECT_TRUE(result.find("\"tags\":[\"web\",\"api\",\"json\"]") != string::npos);
}

TEST_F(JSONBuilderTest, AddIntegerArray_MultipleValues_Success) {
    vector<int> numbers = {1, 2, 3, 4, 5};
    string result = builder_.add_array("numbers", numbers).build();
    
    EXPECT_TRUE(result.find("\"numbers\":[1,2,3,4,5]") != string::npos);
}

TEST_F(JSONBuilderTest, AddStringArray_SpecialCharacters_EscapedCorrectly) {
    vector<string> special = {"hello \"world\"", "line\nbreak", "tab\there"};
    string result = builder_.add_array("special", special).build();
    
    EXPECT_TRUE(result.find("hello \\\"world\\\"") != string::npos);
    EXPECT_TRUE(result.find("line\\nbreak") != string::npos);
    EXPECT_TRUE(result.find("tab\\there") != string::npos);
}

// =============================================================================
// Object Tests
// =============================================================================

TEST_F(JSONBuilderTest, AddObjectFromMap_EmptyMap_Success) {
    map<string, string> empty_map;
    string result = builder_.add_object("config", empty_map).build();
    
    EXPECT_TRUE(contains_key_value(result, "config", "{}"));
}

TEST_F(JSONBuilderTest, AddObjectFromMap_MultipleFields_Success) {
    map<string, string> config = {
        {"host", "localhost"},
        {"port", "8080"},
        {"secure", "true"}
    };
    string result = builder_.add_object("config", config).build();
    
    EXPECT_TRUE(result.find("\"config\":{") != string::npos);
    EXPECT_TRUE(result.find("\"host\":\"localhost\"") != string::npos);
    EXPECT_TRUE(result.find("\"port\":\"8080\"") != string::npos);
    EXPECT_TRUE(result.find("\"secure\":\"true\"") != string::npos);
}

TEST_F(JSONBuilderTest, AddObjectFromBuilder_NestedObject_Success) {
    JSONBuilder nested;
    nested.add("x", 10).add("y", 20);
    
    string result = builder_.add("position", "center")
                            .add_object("coordinates", nested)
                            .build();
    
    EXPECT_TRUE(result.find("\"coordinates\":{\"x\":10,\"y\":20}") != string::npos ||
                result.find("\"coordinates\":{\"y\":20,\"x\":10}") != string::npos);
}

// =============================================================================
// Raw JSON Tests
// =============================================================================

TEST_F(JSONBuilderTest, AddRaw_ValidJSON_Success) {
    string result = builder_.add_raw("custom", "{\"nested\":true}").build();
    
    EXPECT_TRUE(result.find("\"custom\":{\"nested\":true}") != string::npos);
}

TEST_F(JSONBuilderTest, AddRaw_ValidArray_Success) {
    string result = builder_.add_raw("items", "[1,2,3]").build();
    
    EXPECT_TRUE(result.find("\"items\":[1,2,3]") != string::npos);
}

TEST_F(JSONBuilderTest, AddRaw_InvalidJSON_Ignored) {
    size_t initial_size = builder_.size();
    builder_.add_raw("invalid", "not json");
    
    EXPECT_EQ(builder_.size(), initial_size); // Should not add invalid JSON
}

// =============================================================================
// Chaining and Builder Methods Tests
// =============================================================================

TEST_F(JSONBuilderTest, MethodChaining_MultipleFields_Success) {
    string result = builder_.add("name", "Alice")
                            .add("age", 30)
                            .add("active", true)
                            .add_null("description")
                            .build();
    
    EXPECT_EQ(builder_.size(), 4);
    EXPECT_TRUE(result.find("\"name\":\"Alice\"") != string::npos);
    EXPECT_TRUE(result.find("\"age\":30") != string::npos);
    EXPECT_TRUE(result.find("\"active\":true") != string::npos);
    EXPECT_TRUE(result.find("\"description\":null") != string::npos);
}

TEST_F(JSONBuilderTest, Clear_RemovesAllFields_Success) {
    builder_.add("field1", "value1").add("field2", "value2");
    EXPECT_EQ(builder_.size(), 2);
    
    builder_.clear();
    EXPECT_TRUE(builder_.empty());
    EXPECT_EQ(builder_.size(), 0);
    EXPECT_EQ(builder_.build(), "{}");
}

TEST_F(JSONBuilderTest, MultipleBuilds_SameResult_Success) {
    builder_.add("constant", "value");
    
    string result1 = builder_.build();
    string result2 = builder_.build();
    
    EXPECT_EQ(result1, result2);
}

// =============================================================================
// Pretty Printing Tests
// =============================================================================

TEST_F(JSONBuilderTest, PrettyPrint_SimpleObject_FormattedCorrectly) {
    string result = builder_.add("name", "test")
                            .add("value", 42)
                            .build(true);
    
    EXPECT_TRUE(result.find("{\n") != string::npos);
    EXPECT_TRUE(result.find("\n}") != string::npos);
    EXPECT_GT(count_occurrences(result, "\n"), 0);
}

TEST_F(JSONBuilderTest, PrettyPrint_EmptyObject_Simple) {
    string result = builder_.build(true);
    EXPECT_EQ(result, "{}");
}

// =============================================================================
// Static Utility Methods Tests
// =============================================================================

TEST_F(JSONBuilderTest, Success_DefaultMessage_CorrectFormat) {
    string result = JSONBuilder::success();
    
    EXPECT_TRUE(result.find("\"success\":true") != string::npos);
    EXPECT_TRUE(result.find("\"message\":\"Success\"") != string::npos);
}

TEST_F(JSONBuilderTest, Success_CustomMessage_CorrectFormat) {
    string result = JSONBuilder::success("Operation completed");
    
    EXPECT_TRUE(result.find("\"success\":true") != string::npos);
    EXPECT_TRUE(result.find("\"message\":\"Operation completed\"") != string::npos);
}

TEST_F(JSONBuilderTest, Error_MessageOnly_CorrectFormat) {
    string result = JSONBuilder::error("Something went wrong");
    
    EXPECT_TRUE(result.find("\"success\":false") != string::npos);
    EXPECT_TRUE(result.find("\"error\":\"Something went wrong\"") != string::npos);
}

TEST_F(JSONBuilderTest, Error_MessageAndCode_CorrectFormat) {
    string result = JSONBuilder::error("Not found", 404);
    
    EXPECT_TRUE(result.find("\"success\":false") != string::npos);
    EXPECT_TRUE(result.find("\"error\":\"Not found\"") != string::npos);
    EXPECT_TRUE(result.find("\"error_code\":404") != string::npos);
}

TEST_F(JSONBuilderTest, DataResponse_WithData_CorrectFormat) {
    JSONBuilder data;
    data.add("id", 123).add("name", "Test Item");
    
    string result = JSONBuilder::data_response(data);
    
    EXPECT_TRUE(result.find("\"success\":true") != string::npos);
    EXPECT_TRUE(result.find("\"data\":{") != string::npos);
    EXPECT_TRUE(result.find("\"id\":123") != string::npos);
    EXPECT_TRUE(result.find("\"name\":\"Test Item\"") != string::npos);
}

TEST_F(JSONBuilderTest, DataResponse_WithFailure_CorrectFormat) {
    JSONBuilder data;
    data.add("error_details", "Validation failed");
    
    string result = JSONBuilder::data_response(data, false);
    
    EXPECT_TRUE(result.find("\"success\":false") != string::npos);
    EXPECT_TRUE(result.find("\"data\":{") != string::npos);
}

// =============================================================================
// String Escaping Tests
// =============================================================================

TEST_F(JSONBuilderTest, EscapeString_BasicCharacters_NoChange) {
    string result = JSONBuilder::escape_string("hello world");
    EXPECT_EQ(result, "hello world");
}

TEST_F(JSONBuilderTest, EscapeString_QuotesAndBackslashes_EscapedCorrectly) {
    string result = JSONBuilder::escape_string("He said \"Hello\\world\"");
    EXPECT_EQ(result, "He said \\\"Hello\\\\world\\\"");
}

TEST_F(JSONBuilderTest, EscapeString_ControlCharacters_EscapedCorrectly) {
    string input = "line1\nline2\ttab\rcarriage\bbackspace\fformfeed";
    string result = JSONBuilder::escape_string(input);
    
    EXPECT_TRUE(result.find("\\n") != string::npos);
    EXPECT_TRUE(result.find("\\t") != string::npos);
    EXPECT_TRUE(result.find("\\r") != string::npos);
    EXPECT_TRUE(result.find("\\b") != string::npos);
    EXPECT_TRUE(result.find("\\f") != string::npos);
}

TEST_F(JSONBuilderTest, EscapeString_LowControlCharacters_UnicodeEscaped) {
    string input;
    input.push_back(static_cast<char>(1));  // Control character
    input.push_back(static_cast<char>(31)); // Another control character
    
    string result = JSONBuilder::escape_string(input);
    
    EXPECT_TRUE(result.find("\\u0001") != string::npos);
    EXPECT_TRUE(result.find("\\u001f") != string::npos);
}

// =============================================================================
// JSON Validation Tests
// =============================================================================

TEST_F(JSONBuilderTest, IsValidJSON_ValidStructures_ReturnsTrue) {
    EXPECT_TRUE(JSONBuilder::is_valid_json("{}"));
    EXPECT_TRUE(JSONBuilder::is_valid_json("[]"));
    EXPECT_TRUE(JSONBuilder::is_valid_json("\"string\""));
    EXPECT_TRUE(JSONBuilder::is_valid_json("true"));
    EXPECT_TRUE(JSONBuilder::is_valid_json("false"));
    EXPECT_TRUE(JSONBuilder::is_valid_json("null"));
    EXPECT_TRUE(JSONBuilder::is_valid_json("123"));
    EXPECT_TRUE(JSONBuilder::is_valid_json("-45.67"));
}

TEST_F(JSONBuilderTest, IsValidJSON_InvalidStructures_ReturnsFalse) {
    EXPECT_FALSE(JSONBuilder::is_valid_json(""));
    EXPECT_FALSE(JSONBuilder::is_valid_json("not json"));
    EXPECT_FALSE(JSONBuilder::is_valid_json("{unclosed"));
    EXPECT_FALSE(JSONBuilder::is_valid_json("undefined"));
}

TEST_F(JSONBuilderTest, IsValidJSON_WithWhitespace_HandledCorrectly) {
    EXPECT_TRUE(JSONBuilder::is_valid_json("  {}  "));
    EXPECT_TRUE(JSONBuilder::is_valid_json("\n\t[]\n"));
    EXPECT_TRUE(JSONBuilder::is_valid_json("  \"test\"  "));
}

// =============================================================================
// Edge Cases and Error Handling Tests
// =============================================================================

TEST_F(JSONBuilderTest, InvalidFieldNames_Ignored) {
    size_t initial_size = builder_.size();
    
    builder_.add("", "empty key");  // Empty key should be ignored
    EXPECT_EQ(builder_.size(), initial_size);
    
    // Very long key (over 1000 chars) should be ignored
    string long_key(1001, 'a');
    builder_.add(long_key, "value");
    EXPECT_EQ(builder_.size(), initial_size);
}

TEST_F(JSONBuilderTest, UnicodeCharacters_HandledCorrectly) {
    string result = builder_.add("message", "Hello 🌍 World! © 2024").build();
    
    // Should contain the Unicode characters properly encoded
    EXPECT_TRUE(result.find("Hello 🌍 World! © 2024") != string::npos);
}

TEST_F(JSONBuilderTest, LargeNumbers_HandledCorrectly) {
    string result = builder_.add("large_int", 2147483647)  // INT_MAX
                            .add("large_long", 9223372036854775807L)  // LONG_MAX
                            .build();
    
    EXPECT_TRUE(result.find("2147483647") != string::npos);
    EXPECT_TRUE(result.find("9223372036854775807") != string::npos);
}

TEST_F(JSONBuilderTest, SpecialDoubleValues_HandledCorrectly) {
    string result = builder_.add("zero", 0.0)
                            .add("small", 0.000001)
                            .add("large", 1000000.0)
                            .build();
    
    EXPECT_TRUE(result.find("\"zero\":0") != string::npos);
    EXPECT_TRUE(result.find("\"small\":0.000001") != string::npos ||
                result.find("\"small\":1e-06") != string::npos);  // Scientific notation acceptable
    EXPECT_TRUE(result.find("\"large\":1000000") != string::npos);
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST_F(JSONBuilderTest, ComplexNestedStructure_BuildsCorrectly) {
    JSONBuilder user_data;
    user_data.add("id", 12345)
             .add("name", "John Doe")
             .add("email", "john@example.com");
    
    JSONBuilder permissions;
    permissions.add("read", true)
               .add("write", false)
               .add("admin", false);
    
    vector<string> tags = {"user", "active", "premium"};
    map<string, string> metadata = {
        {"created_by", "system"},
        {"version", "1.0"}
    };
    
    string result = builder_.add_object("user", user_data)
                            .add_object("permissions", permissions)
                            .add_array("tags", tags)
                            .add_object("metadata", metadata)
                            .add("timestamp", 1640995200L)
                            .build();
    
    // Verify all components are present
    EXPECT_TRUE(result.find("\"user\":{") != string::npos);
    EXPECT_TRUE(result.find("\"permissions\":{") != string::npos);
    EXPECT_TRUE(result.find("\"tags\":[") != string::npos);
    EXPECT_TRUE(result.find("\"metadata\":{") != string::npos);
    EXPECT_TRUE(result.find("\"timestamp\":1640995200") != string::npos);
    
    // Verify nested content
    EXPECT_TRUE(result.find("\"id\":12345") != string::npos);
    EXPECT_TRUE(result.find("\"read\":true") != string::npos);
    EXPECT_TRUE(result.find("\"user\",\"active\",\"premium\"") != string::npos);
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(JSONBuilderTest, Performance_ManyFields_BuildsEfficiently) {
    auto start = chrono::high_resolution_clock::now();
    
    // Add 1000 fields
    for (int i = 0; i < 1000; ++i) {
        builder_.add("field_" + to_string(i), "value_" + to_string(i));
    }
    
    string result = builder_.build();
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    EXPECT_EQ(builder_.size(), 1000);
    EXPECT_LT(duration.count(), 100);  // Should complete within 100ms
    
    // Verify some fields are present
    EXPECT_TRUE(result.find("\"field_0\":\"value_0\"") != string::npos);
    EXPECT_TRUE(result.find("\"field_999\":\"value_999\"") != string::npos);
}

TEST_F(JSONBuilderTest, Performance_LargeStrings_HandledEfficiently) {
    string large_string(10000, 'A');  // 10KB string
    
    auto start = chrono::high_resolution_clock::now();
    string result = builder_.add("large_data", large_string).build();
    auto end = chrono::high_resolution_clock::now();
    
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 50);  // Should complete within 50ms
    EXPECT_TRUE(result.find(large_string) != string::npos);
} 