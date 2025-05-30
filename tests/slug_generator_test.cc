#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "utils/slug_generator.h"
#include "database/database_interface.h"
#include <set>
#include <regex>
#include <chrono>

using namespace std;
using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;

/**
 * @brief Mock database interface for testing
 */
class MockDatabaseInterface : public DatabaseInterface {
public:
    // Database setup methods
    MOCK_METHOD(bool, initialize, (const string& db_path), (override));
    MOCK_METHOD(bool, create_tables, (), (override));
    
    // Link operations
    MOCK_METHOD(bool, insert_link, (const string& code, const string& destination, const string& type), (override));
    MOCK_METHOD(optional<LinkData>, get_link, (const string& code), (override));
    MOCK_METHOD(bool, code_exists, (const string& code), (override));
    MOCK_METHOD(bool, delete_link, (const string& code), (override));
    
    // Analytics operations
    MOCK_METHOD(bool, record_click, (const ClickRecord& record), (override));
    MOCK_METHOD(ClickStats, get_click_stats, (const string& code), (override));
    MOCK_METHOD(vector<ClickRecord>, get_recent_clicks, (const string& code, int limit), (override));
};

/**
 * @brief Test fixture for SlugGenerator tests
 */
class SlugGeneratorTest : public ::testing::Test {
protected:
    MockDatabaseInterface mock_db;
};

/**
 * @brief Test basic code generation functionality
 */
TEST_F(SlugGeneratorTest, GenerateCodeBasicFunctionality) {
    // Test default length (8)
    string code = SlugGenerator::generate_code();
    EXPECT_EQ(code.length(), 8);
    
    // Test custom lengths
    EXPECT_EQ(SlugGenerator::generate_code(1).length(), 1);
    EXPECT_EQ(SlugGenerator::generate_code(16).length(), 16);
    EXPECT_EQ(SlugGenerator::generate_code(32).length(), 32);
}

/**
 * @brief Test that generated codes contain only valid characters
 */
TEST_F(SlugGeneratorTest, GenerateCodeValidCharacters) {
    string code = SlugGenerator::generate_code(100);
    
    // Check that all characters are alphanumeric
    regex valid_chars("^[a-zA-Z0-9]+$");
    EXPECT_TRUE(regex_match(code, valid_chars));
}

/**
 * @brief Test randomness - codes should be different
 */
TEST_F(SlugGeneratorTest, GenerateCodeRandomness) {
    set<string> generated_codes;
    const int num_codes = 1000;
    
    for (int i = 0; i < num_codes; ++i) {
        string code = SlugGenerator::generate_code(8);
        generated_codes.insert(code);
    }
    
    // With 8-character codes from 62-character set, collisions should be extremely rare
    // We expect at least 99% unique codes
    EXPECT_GT(generated_codes.size(), num_codes * 0.99);
}

/**
 * @brief Test zero length throws exception
 */
TEST_F(SlugGeneratorTest, GenerateCodeZeroLengthThrows) {
    EXPECT_THROW(SlugGenerator::generate_code(0), std::invalid_argument);
}

/**
 * @brief Test unique code generation when no collisions occur
 */
TEST_F(SlugGeneratorTest, GenerateUniqueCodeNoCollisions) {
    // Mock database to return false (code doesn't exist) for any code
    EXPECT_CALL(mock_db, code_exists(_))
        .WillRepeatedly(Return(false));
    
    string unique_code = SlugGenerator::generate_unique_code(&mock_db);
    
    EXPECT_EQ(unique_code.length(), 8);
    EXPECT_FALSE(unique_code.empty());
}

/**
 * @brief Test unique code generation with some collisions
 */
TEST_F(SlugGeneratorTest, GenerateUniqueCodeWithCollisions) {
    // First 3 codes exist, 4th doesn't
    InSequence seq;
    EXPECT_CALL(mock_db, code_exists(_))
        .WillOnce(Return(true))   // First attempt: collision
        .WillOnce(Return(true))   // Second attempt: collision  
        .WillOnce(Return(true))   // Third attempt: collision
        .WillOnce(Return(false)); // Fourth attempt: success
    
    string unique_code = SlugGenerator::generate_unique_code(&mock_db, 8, 10);
    
    EXPECT_EQ(unique_code.length(), 8);
    EXPECT_FALSE(unique_code.empty());
}

/**
 * @brief Test max attempts exceeded
 */
TEST_F(SlugGeneratorTest, GenerateUniqueCodeMaxAttemptsExceeded) {
    // All codes exist (always return true)
    EXPECT_CALL(mock_db, code_exists(_))
        .WillRepeatedly(Return(true));
    
    string unique_code = SlugGenerator::generate_unique_code(&mock_db, 8, 5);
    
    EXPECT_TRUE(unique_code.empty());
}

/**
 * @brief Test null database parameter
 */
TEST_F(SlugGeneratorTest, GenerateUniqueCodeNullDatabase) {
    EXPECT_THROW(SlugGenerator::generate_unique_code(nullptr), std::invalid_argument);
}

/**
 * @brief Test invalid parameters for unique code generation
 */
TEST_F(SlugGeneratorTest, GenerateUniqueCodeInvalidParameters) {
    EXPECT_THROW(SlugGenerator::generate_unique_code(&mock_db, 0), std::invalid_argument);
    EXPECT_THROW(SlugGenerator::generate_unique_code(&mock_db, 8, 0), std::invalid_argument);
    EXPECT_THROW(SlugGenerator::generate_unique_code(&mock_db, 8, -1), std::invalid_argument);
}

/**
 * @brief Test custom length for unique code generation
 */
TEST_F(SlugGeneratorTest, GenerateUniqueCodeCustomLength) {
    EXPECT_CALL(mock_db, code_exists(_))
        .WillOnce(Return(false));
    
    string unique_code = SlugGenerator::generate_unique_code(&mock_db, 12);
    
    EXPECT_EQ(unique_code.length(), 12);
    EXPECT_FALSE(unique_code.empty());
}

/**
 * @brief Performance test - should generate codes quickly
 */
TEST_F(SlugGeneratorTest, GenerateCodePerformance) {
    auto start = chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; ++i) {
        SlugGenerator::generate_code(8);
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    // Should generate 10,000 codes in less than 1 second
    EXPECT_LT(duration.count(), 1000);
} 