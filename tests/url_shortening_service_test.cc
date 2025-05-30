#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "services/url_shortening_service.h"
#include "database/database_interface.h"
#include "models/link_data.h"
#include <memory>
#include <string>

using namespace std;
using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;
using ::testing::NiceMock;

/**
 * @brief Mock implementation of DatabaseInterface for testing
 */
class MockDatabaseInterface : public DatabaseInterface {
public:
    MOCK_METHOD(bool, initialize, (const string& db_path), (override));
    MOCK_METHOD(bool, create_tables, (), (override));
    MOCK_METHOD(bool, insert_link, (const string& code, const string& destination, const string& type), (override));
    MOCK_METHOD(optional<LinkData>, get_link, (const string& code), (override));
    MOCK_METHOD(bool, code_exists, (const string& code), (override));
    MOCK_METHOD(bool, delete_link, (const string& code), (override));
    MOCK_METHOD(bool, record_click, (const ClickRecord& record), (override));
    MOCK_METHOD(ClickStats, get_click_stats, (const string& code), (override));
    MOCK_METHOD(vector<ClickRecord>, get_recent_clicks, (const string& code, int limit), (override));
};

class URLShorteningServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_db_ptr = make_unique<NiceMock<MockDatabaseInterface>>();
        mock_db = mock_db_ptr.get();
        
        service = make_unique<URLShorteningService>(
            move(mock_db_ptr), 
            "https://short.ly"
        );
    }
    
    void TearDown() override {
        service.reset();
    }
    
    unique_ptr<NiceMock<MockDatabaseInterface>> mock_db_ptr;
    MockDatabaseInterface* mock_db;
    unique_ptr<URLShorteningService> service;
};

// Test successful URL shortening
TEST_F(URLShorteningServiceTest, ShortenURL_ValidURL_Success) {
    string test_url = "https://www.example.com";
    string normalized_url = "https://www.example.com/";  // URLValidator adds trailing slash
    
    // Mock expectations - use ANY for generated code since it's random
    EXPECT_CALL(*mock_db, code_exists(_))
        .WillOnce(Return(false));  // First attempt generates unique code
    
    EXPECT_CALL(*mock_db, insert_link(_, normalized_url, "url"))
        .WillOnce(Return(true));
    
    auto result = service->shorten_url(test_url);
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.code.empty());
    EXPECT_EQ(result.short_url.substr(0, 19), "https://short.ly/r/");
    // QR code might be empty if generation fails, so don't check it
    EXPECT_TRUE(result.error_message.empty());
}

// Test URL shortening with invalid URL
TEST_F(URLShorteningServiceTest, ShortenURL_InvalidURL_Failure) {
    string invalid_url = "not-a-valid-url";
    
    auto result = service->shorten_url(invalid_url);
    
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.code.empty());
    EXPECT_TRUE(result.short_url.empty());
    EXPECT_TRUE(result.qr_code_data_url.empty());
    EXPECT_FALSE(result.error_message.empty());
}

// Test URL shortening when database insertion fails
TEST_F(URLShorteningServiceTest, ShortenURL_DatabaseInsertFails_Failure) {
    string test_url = "https://www.example.com";
    string normalized_url = "https://www.example.com/";
    
    EXPECT_CALL(*mock_db, code_exists(_))
        .WillOnce(Return(false));
    
    EXPECT_CALL(*mock_db, insert_link(_, normalized_url, "url"))
        .WillOnce(Return(false));
    
    auto result = service->shorten_url(test_url);
    
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.code.empty());
    EXPECT_EQ(result.error_message, "Failed to store shortened URL");
}

// Test URL shortening when unique code generation fails
TEST_F(URLShorteningServiceTest, ShortenURL_UniqueCodeGenerationFails_Failure) {
    string test_url = "https://www.example.com";
    
    // Mock that all codes already exist (simulating failure to generate unique code)
    EXPECT_CALL(*mock_db, code_exists(_))
        .WillRepeatedly(Return(true));
    
    auto result = service->shorten_url(test_url);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message, "Failed to generate unique code");
}

// Test successful code resolution
TEST_F(URLShorteningServiceTest, ResolveCode_ExistingURLCode_Success) {
    string test_code = "abc12345";
    string original_url = "https://www.example.com";
    
    LinkData link_data(test_code, original_url, "url");
    
    EXPECT_CALL(*mock_db, get_link(test_code))
        .WillOnce(Return(link_data));
    
    auto result = service->resolve_code(test_code);
    
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), original_url);
}

// Test code resolution for non-existent code
TEST_F(URLShorteningServiceTest, ResolveCode_NonExistentCode_ReturnsEmpty) {
    string test_code = "nonexist";
    
    EXPECT_CALL(*mock_db, get_link(test_code))
        .WillOnce(Return(nullopt));
    
    auto result = service->resolve_code(test_code);
    
    EXPECT_FALSE(result.has_value());
}

// Test code resolution for file type (should not resolve)
TEST_F(URLShorteningServiceTest, ResolveCode_FileType_ReturnsEmpty) {
    string test_code = "abc12345";
    string file_path = "/path/to/file.txt";
    
    LinkData link_data(test_code, file_path, "file");
    
    EXPECT_CALL(*mock_db, get_link(test_code))
        .WillOnce(Return(link_data));
    
    auto result = service->resolve_code(test_code);
    
    EXPECT_FALSE(result.has_value());
}

// Test successful access recording
TEST_F(URLShorteningServiceTest, RecordAccess_ValidCode_Success) {
    string test_code = "abc12345";
    string ip_address = "192.168.1.100";
    string user_agent = "Mozilla/5.0";
    string referrer = "https://google.com";
    
    EXPECT_CALL(*mock_db, code_exists(test_code))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_db, record_click(_))
        .WillOnce(Return(true));
    
    bool result = service->record_access(test_code, ip_address, user_agent, referrer);
    
    EXPECT_TRUE(result);
}

// Test access recording for non-existent code
TEST_F(URLShorteningServiceTest, RecordAccess_InvalidCode_Failure) {
    string test_code = "nonexist";
    string ip_address = "192.168.1.100";
    string user_agent = "Mozilla/5.0";
    
    EXPECT_CALL(*mock_db, code_exists(test_code))
        .WillOnce(Return(false));
    
    bool result = service->record_access(test_code, ip_address, user_agent);
    
    EXPECT_FALSE(result);
}

// Test statistics retrieval
TEST_F(URLShorteningServiceTest, GetStatistics_ValidCode_ReturnsStats) {
    string test_code = "abc12345";
    ClickStats expected_stats;
    expected_stats.total_clicks = 42;
    expected_stats.daily_clicks["2024-01-01"] = 10;
    expected_stats.daily_clicks["2024-01-02"] = 32;
    
    EXPECT_CALL(*mock_db, get_click_stats(test_code))
        .WillOnce(Return(expected_stats));
    
    auto result = service->get_statistics(test_code);
    
    EXPECT_EQ(result.total_clicks, 42);
    EXPECT_EQ(result.daily_clicks.size(), 2);
    EXPECT_EQ(result.daily_clicks["2024-01-01"], 10);
    EXPECT_EQ(result.daily_clicks["2024-01-02"], 32);
}

// Test link deletion
TEST_F(URLShorteningServiceTest, DeleteLink_ValidCode_Success) {
    string test_code = "abc12345";
    
    EXPECT_CALL(*mock_db, delete_link(test_code))
        .WillOnce(Return(true));
    
    bool result = service->delete_link(test_code);
    
    EXPECT_TRUE(result);
}

TEST_F(URLShorteningServiceTest, DeleteLink_InvalidCode_Failure) {
    string test_code = "nonexist";
    
    EXPECT_CALL(*mock_db, delete_link(test_code))
        .WillOnce(Return(false));
    
    bool result = service->delete_link(test_code);
    
    EXPECT_FALSE(result);
}

// Test IP privacy truncation
class URLShorteningServicePrivacyTest : public URLShorteningServiceTest {
protected:
    // Inherits setup from base class
};

// Integration test for IP truncation behavior
TEST_F(URLShorteningServicePrivacyTest, RecordAccess_IPv4_TruncatesCorrectly) {
    string test_code = "abc12345";
    string ipv4_address = "192.168.1.100";
    
    EXPECT_CALL(*mock_db, code_exists(test_code))
        .WillOnce(Return(true));
    
    // Verify that record_click is called with truncated IP
    EXPECT_CALL(*mock_db, record_click(_))
        .WillOnce([](const ClickRecord& record) {
            // The IP should be truncated to 192.168.1.0
            EXPECT_EQ(record.ip_truncated, "192.168.1.0");
            return true;
        });
    
    service->record_access(test_code, ipv4_address, "Mozilla/5.0");
}

TEST_F(URLShorteningServicePrivacyTest, RecordAccess_IPv6_TruncatesCorrectly) {
    string test_code = "abc12345";
    string ipv6_address = "2001:db8::1234";
    
    EXPECT_CALL(*mock_db, code_exists(test_code))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_db, record_click(_))
        .WillOnce([](const ClickRecord& record) {
            // The IP should be truncated
            EXPECT_EQ(record.ip_truncated, "2001:db8:::0");
            return true;
        });
    
    service->record_access(test_code, ipv6_address, "Mozilla/5.0");
}

TEST_F(URLShorteningServicePrivacyTest, RecordAccess_EmptyIP_DefaultsCorrectly) {
    string test_code = "abc12345";
    string empty_ip = "";
    
    EXPECT_CALL(*mock_db, code_exists(test_code))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_db, record_click(_))
        .WillOnce([](const ClickRecord& record) {
            EXPECT_EQ(record.ip_truncated, "0.0.0.0");
            return true;
        });
    
    service->record_access(test_code, empty_ip, "Mozilla/5.0");
}

// Test URL construction
class URLShorteningServiceURLTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create fresh mock for each test
        mock_db_ptr = make_unique<NiceMock<MockDatabaseInterface>>();
        mock_db = mock_db_ptr.get();
        
        service = make_unique<URLShorteningService>(
            move(mock_db_ptr), 
            "https://short.ly"
        );
    }
    
    void TearDown() override {
        service.reset();
    }
    
    unique_ptr<NiceMock<MockDatabaseInterface>> mock_db_ptr;
    MockDatabaseInterface* mock_db;
    unique_ptr<URLShorteningService> service;
};

TEST_F(URLShorteningServiceURLTest, BuildShortURL_BaseURLWithTrailingSlash) {
    // Create service with trailing slash
    auto mock_db_local = make_unique<NiceMock<MockDatabaseInterface>>();
    auto* mock_db_ref = mock_db_local.get();
    
    auto service_local = make_unique<URLShorteningService>(
        move(mock_db_local), 
        "https://short.ly/"  // Note trailing slash
    );
    
    string test_url = "https://www.example.com";
    string normalized_url = "https://www.example.com/";
    
    EXPECT_CALL(*mock_db_ref, code_exists(_))
        .WillOnce(Return(false));
    
    EXPECT_CALL(*mock_db_ref, insert_link(_, normalized_url, "url"))
        .WillOnce(Return(true));
    
    auto result = service_local->shorten_url(test_url);
    
    EXPECT_TRUE(result.success);
    // Should still produce correct URL without double slash
    EXPECT_TRUE(result.short_url.find("https://short.ly/r/") == 0);
    EXPECT_TRUE(result.short_url.find("//r/") == string::npos);
}

TEST_F(URLShorteningServiceURLTest, BuildShortURL_BaseURLWithoutTrailingSlash) {
    string test_url = "https://www.example.com";
    string normalized_url = "https://www.example.com/";
    
    EXPECT_CALL(*mock_db, code_exists(_))
        .WillOnce(Return(false));
    
    EXPECT_CALL(*mock_db, insert_link(_, normalized_url, "url"))
        .WillOnce(Return(true));
    
    auto result = service->shorten_url(test_url);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.short_url.find("https://short.ly/r/") == 0);
} 