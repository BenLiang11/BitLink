#include "gtest/gtest.h"
#include "models/link_data.h"
#include <chrono>
#include <string>

using namespace std;

/**
 * @brief Test fixture for LinkData model tests
 * 
 * Tests the core data structures used in the URL shortening service:
 * LinkData, ClickStats, and ClickRecord.
 */
class LinkDataTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test data
        test_code_ = "abc12345";
        test_destination_ = "https://example.com";
        test_type_url_ = "url";
        test_type_file_ = "file";
        test_ip_ = "192.168.1.1";
        test_user_agent_ = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36";
        test_referrer_ = "https://google.com";
        
        // Get consistent timestamps for testing
        test_time_ = chrono::system_clock::now();
    }
    
    string test_code_;
    string test_destination_;
    string test_type_url_;
    string test_type_file_;
    string test_ip_;
    string test_user_agent_;
    string test_referrer_;
    chrono::system_clock::time_point test_time_;
};

// ===== LinkData Tests =====

TEST_F(LinkDataTest, DefaultConstructor) {
    LinkData link;
    
    EXPECT_TRUE(link.code.empty());
    EXPECT_TRUE(link.destination.empty());
    EXPECT_TRUE(link.type.empty());
    // created timestamp should be default-initialized
}

TEST_F(LinkDataTest, ParameterizedConstructor) {
    LinkData link(test_code_, test_destination_, test_type_url_);
    
    EXPECT_EQ(link.code, test_code_);
    EXPECT_EQ(link.destination, test_destination_);
    EXPECT_EQ(link.type, test_type_url_);
    
    // Check that created timestamp is recent (within last 5 seconds)
    auto now = chrono::system_clock::now();
    auto diff = chrono::duration_cast<chrono::seconds>(now - link.created);
    EXPECT_LT(diff.count(), 5);
    EXPECT_GE(diff.count(), 0);
}

TEST_F(LinkDataTest, ParameterizedConstructorWithFile) {
    LinkData link(test_code_, "/path/to/file.pdf", test_type_file_);
    
    EXPECT_EQ(link.code, test_code_);
    EXPECT_EQ(link.destination, "/path/to/file.pdf");
    EXPECT_EQ(link.type, test_type_file_);
    
    // Check timestamp validity
    auto now = chrono::system_clock::now();
    auto diff = chrono::duration_cast<chrono::seconds>(now - link.created);
    EXPECT_LT(diff.count(), 5);
}

TEST_F(LinkDataTest, ParameterizedConstructorEmptyStrings) {
    LinkData link("", "", "");
    
    EXPECT_TRUE(link.code.empty());
    EXPECT_TRUE(link.destination.empty());
    EXPECT_TRUE(link.type.empty());
    
    // Timestamp should still be set
    auto now = chrono::system_clock::now();
    auto diff = chrono::duration_cast<chrono::seconds>(now - link.created);
    EXPECT_LT(diff.count(), 5);
}

TEST_F(LinkDataTest, CopyConstructor) {
    LinkData original(test_code_, test_destination_, test_type_url_);
    LinkData copy = original;
    
    EXPECT_EQ(copy.code, original.code);
    EXPECT_EQ(copy.destination, original.destination);
    EXPECT_EQ(copy.type, original.type);
    EXPECT_EQ(copy.created, original.created);
}

TEST_F(LinkDataTest, AssignmentOperator) {
    LinkData original(test_code_, test_destination_, test_type_url_);
    LinkData assigned;
    
    assigned = original;
    
    EXPECT_EQ(assigned.code, original.code);
    EXPECT_EQ(assigned.destination, original.destination);
    EXPECT_EQ(assigned.type, original.type);
    EXPECT_EQ(assigned.created, original.created);
}

// ===== ClickStats Tests =====

TEST_F(LinkDataTest, ClickStatsDefaultConstructor) {
    ClickStats stats;
    
    EXPECT_EQ(stats.total_clicks, 0);
    EXPECT_TRUE(stats.daily_clicks.empty());
    // last_accessed should be default-initialized
}

TEST_F(LinkDataTest, ClickStatsAddDailyClicks) {
    ClickStats stats;
    
    stats.daily_clicks["2024-01-15"] = 5;
    stats.daily_clicks["2024-01-16"] = 10;
    stats.daily_clicks["2024-01-17"] = 3;
    stats.total_clicks = 18;
    
    EXPECT_EQ(stats.total_clicks, 18);
    EXPECT_EQ(stats.daily_clicks.size(), 3);
    EXPECT_EQ(stats.daily_clicks["2024-01-15"], 5);
    EXPECT_EQ(stats.daily_clicks["2024-01-16"], 10);
    EXPECT_EQ(stats.daily_clicks["2024-01-17"], 3);
}

TEST_F(LinkDataTest, ClickStatsSetLastAccessed) {
    ClickStats stats;
    stats.last_accessed = test_time_;
    
    EXPECT_EQ(stats.last_accessed, test_time_);
}

TEST_F(LinkDataTest, ClickStatsCopyConstructor) {
    ClickStats original;
    original.total_clicks = 42;
    original.daily_clicks["2024-01-15"] = 15;
    original.daily_clicks["2024-01-16"] = 27;
    original.last_accessed = test_time_;
    
    ClickStats copy = original;
    
    EXPECT_EQ(copy.total_clicks, original.total_clicks);
    EXPECT_EQ(copy.daily_clicks, original.daily_clicks);
    EXPECT_EQ(copy.last_accessed, original.last_accessed);
}

// ===== ClickRecord Tests =====

TEST_F(LinkDataTest, ClickRecordDefaultConstructor) {
    ClickRecord record;
    
    EXPECT_TRUE(record.code.empty());
    EXPECT_TRUE(record.ip_truncated.empty());
    EXPECT_TRUE(record.user_agent.empty());
    EXPECT_TRUE(record.referrer.empty());
    // timestamp should be default-initialized
}

TEST_F(LinkDataTest, ClickRecordParameterizedConstructor) {
    ClickRecord record(test_code_, test_ip_, test_user_agent_, test_referrer_);
    
    EXPECT_EQ(record.code, test_code_);
    EXPECT_EQ(record.ip_truncated, test_ip_);
    EXPECT_EQ(record.user_agent, test_user_agent_);
    EXPECT_EQ(record.referrer, test_referrer_);
    
    // Check that timestamp is recent (within last 5 seconds)
    auto now = chrono::system_clock::now();
    auto diff = chrono::duration_cast<chrono::seconds>(now - record.timestamp);
    EXPECT_LT(diff.count(), 5);
    EXPECT_GE(diff.count(), 0);
}

TEST_F(LinkDataTest, ClickRecordParameterizedConstructorEmptyValues) {
    ClickRecord record("", "", "", "");
    
    EXPECT_TRUE(record.code.empty());
    EXPECT_TRUE(record.ip_truncated.empty());
    EXPECT_TRUE(record.user_agent.empty());
    EXPECT_TRUE(record.referrer.empty());
    
    // Timestamp should still be set
    auto now = chrono::system_clock::now();
    auto diff = chrono::duration_cast<chrono::seconds>(now - record.timestamp);
    EXPECT_LT(diff.count(), 5);
}

TEST_F(LinkDataTest, ClickRecordWithPartialData) {
    ClickRecord record(test_code_, test_ip_, test_user_agent_, "");
    
    EXPECT_EQ(record.code, test_code_);
    EXPECT_EQ(record.ip_truncated, test_ip_);
    EXPECT_EQ(record.user_agent, test_user_agent_);
    EXPECT_TRUE(record.referrer.empty());
}

TEST_F(LinkDataTest, ClickRecordCopyConstructor) {
    ClickRecord original(test_code_, test_ip_, test_user_agent_, test_referrer_);
    ClickRecord copy = original;
    
    EXPECT_EQ(copy.code, original.code);
    EXPECT_EQ(copy.ip_truncated, original.ip_truncated);
    EXPECT_EQ(copy.user_agent, original.user_agent);
    EXPECT_EQ(copy.referrer, original.referrer);
    EXPECT_EQ(copy.timestamp, original.timestamp);
}

TEST_F(LinkDataTest, ClickRecordAssignmentOperator) {
    ClickRecord original(test_code_, test_ip_, test_user_agent_, test_referrer_);
    ClickRecord assigned;
    
    assigned = original;
    
    EXPECT_EQ(assigned.code, original.code);
    EXPECT_EQ(assigned.ip_truncated, original.ip_truncated);
    EXPECT_EQ(assigned.user_agent, original.user_agent);
    EXPECT_EQ(assigned.referrer, original.referrer);
    EXPECT_EQ(assigned.timestamp, original.timestamp);
}

// ===== Integration Tests =====

TEST_F(LinkDataTest, DataStructuresInteraction) {
    // Create a LinkData instance
    LinkData link(test_code_, test_destination_, test_type_url_);
    
    // Create some click records for this link
    ClickRecord click1(test_code_, test_ip_, test_user_agent_, test_referrer_);
    ClickRecord click2(test_code_, "192.168.1.2", "Chrome/90.0", "https://twitter.com");
    
    // Create click stats
    ClickStats stats;
    stats.total_clicks = 2;
    stats.daily_clicks["2024-01-15"] = 1;
    stats.daily_clicks["2024-01-16"] = 1;
    stats.last_accessed = click2.timestamp;
    
    // Verify relationships
    EXPECT_EQ(link.code, click1.code);
    EXPECT_EQ(link.code, click2.code);
    EXPECT_EQ(stats.total_clicks, 2);
    EXPECT_EQ(stats.daily_clicks.size(), 2);
}

TEST_F(LinkDataTest, LargeDataHandling) {
    // Test with large strings
    string large_url(10000, 'x');  // 10KB URL
    string large_user_agent(2000, 'y');  // 2KB user agent
    
    LinkData link("testcode", large_url, "url");
    ClickRecord record("testcode", test_ip_, large_user_agent, test_referrer_);
    
    EXPECT_EQ(link.destination, large_url);
    EXPECT_EQ(record.user_agent, large_user_agent);
    EXPECT_EQ(link.destination.length(), 10000);
    EXPECT_EQ(record.user_agent.length(), 2000);
}

TEST_F(LinkDataTest, SpecialCharacterHandling) {
    // Test with special characters and unicode
    string special_destination = "https://example.com/path?query=test&param=αβγδε";
    string special_user_agent = "Mozilla/5.0 (compat; 中文测试; 한국어)";
    string special_referrer = "https://测试.com/路径";
    
    LinkData link("test123", special_destination, "url");
    ClickRecord record("test123", test_ip_, special_user_agent, special_referrer);
    
    EXPECT_EQ(link.destination, special_destination);
    EXPECT_EQ(record.user_agent, special_user_agent);
    EXPECT_EQ(record.referrer, special_referrer);
} 