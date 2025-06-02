#include "gtest/gtest.h"
#include "database/sqlite_database.h"
#include "models/link_data.h"
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <fstream>

using namespace std;
namespace fs = std::filesystem;

/**
 * @brief Test fixture for SQLiteDatabase tests
 * 
 * Tests the SQLite database implementation with comprehensive coverage
 * including CRUD operations, analytics, concurrency, and error handling.
 */
class SQLiteDatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a unique temporary database file for each test
        temp_db_path_ = "/tmp/test_shortener_" + to_string(chrono::system_clock::now().time_since_epoch().count()) + ".db";
        
        // Initialize test data
        test_code_ = "abc12345";
        test_code2_ = "xyz67890";
        test_url_ = "https://example.com/test";
        test_file_path_ = "/tmp/test_file.pdf";
        test_type_url_ = "url";
        test_type_file_ = "file";
        
        test_ip_ = "192.168.1.100";
        test_ip_truncated_ = "192.168.1";  // Privacy: last octet removed
        test_user_agent_ = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36";
        test_referrer_ = "https://google.com/search";
        
        // Create database instance
        db_ = make_unique<SQLiteDatabase>();
    }
    
    void TearDown() override {
        // Clean up database file
        db_.reset();  // Close database first
        if (fs::exists(temp_db_path_)) {
            fs::remove(temp_db_path_);
        }
    }
    
    string temp_db_path_;
    unique_ptr<SQLiteDatabase> db_;
    
    // Test data
    string test_code_;
    string test_code2_;
    string test_url_;
    string test_file_path_;
    string test_type_url_;
    string test_type_file_;
    string test_ip_;
    string test_ip_truncated_;
    string test_user_agent_;
    string test_referrer_;
};

// ===== Database Initialization Tests =====

TEST_F(SQLiteDatabaseTest, InitializeSuccessfully) {
    EXPECT_TRUE(db_->initialize(temp_db_path_));
    EXPECT_TRUE(fs::exists(temp_db_path_));
}

TEST_F(SQLiteDatabaseTest, InitializeInvalidPath) {
    // Try to initialize with an invalid path
    string invalid_path = "/invalid/directory/that/does/not/exist/db.sqlite";
    EXPECT_FALSE(db_->initialize(invalid_path));
}

TEST_F(SQLiteDatabaseTest, InitializeEmptyPath) {
    EXPECT_FALSE(db_->initialize(""));
}

TEST_F(SQLiteDatabaseTest, ReinitializeDatabase) {
    // First initialization
    EXPECT_TRUE(db_->initialize(temp_db_path_));
    
    // Second initialization should also work (reopening existing database)
    EXPECT_TRUE(db_->initialize(temp_db_path_));
}

TEST_F(SQLiteDatabaseTest, CreateTablesSuccessfully) {
    EXPECT_TRUE(db_->initialize(temp_db_path_));
    // Tables should be created during initialization
    
    // Insert a test record to verify tables exist
    EXPECT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
}

// ===== Link CRUD Operations Tests =====

TEST_F(SQLiteDatabaseTest, InsertLinkSuccess) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    EXPECT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
}

TEST_F(SQLiteDatabaseTest, InsertMultipleLinks) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    EXPECT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    EXPECT_TRUE(db_->insert_link(test_code2_, test_file_path_, test_type_file_));
}

TEST_F(SQLiteDatabaseTest, InsertLinkDuplicateCode) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    // Insert first link
    EXPECT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    // Try to insert with same code (should fail due to PRIMARY KEY constraint)
    EXPECT_FALSE(db_->insert_link(test_code_, "https://different.com", test_type_url_));
}

TEST_F(SQLiteDatabaseTest, InsertLinkInvalidType) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    // Try to insert with invalid type (should fail due to CHECK constraint)
    EXPECT_FALSE(db_->insert_link(test_code_, test_url_, "invalid_type"));
}

TEST_F(SQLiteDatabaseTest, InsertLinkEmptyStrings) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    // Empty code should fail (PRIMARY KEY can't be empty)
    EXPECT_FALSE(db_->insert_link("", test_url_, test_type_url_));
    
    // Empty destination might be allowed depending on business logic
    EXPECT_TRUE(db_->insert_link(test_code_, "", test_type_url_));
}

TEST_F(SQLiteDatabaseTest, GetLinkSuccess) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    auto result = db_->get_link(test_code_);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->code, test_code_);
    EXPECT_EQ(result->destination, test_url_);
    EXPECT_EQ(result->type, test_type_url_);
    
    // Check timestamp is recent (within last minute)
    auto now = chrono::system_clock::now();
    auto diff = chrono::duration_cast<chrono::seconds>(now - result->created);
    EXPECT_LT(diff.count(), 60);
}

TEST_F(SQLiteDatabaseTest, GetLinkNotFound) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    auto result = db_->get_link("nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST_F(SQLiteDatabaseTest, GetLinkEmptyCode) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    auto result = db_->get_link("");
    EXPECT_FALSE(result.has_value());
}

TEST_F(SQLiteDatabaseTest, CodeExistsTrue) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    EXPECT_TRUE(db_->code_exists(test_code_));
}

TEST_F(SQLiteDatabaseTest, CodeExistsFalse) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    EXPECT_FALSE(db_->code_exists("nonexistent"));
}

TEST_F(SQLiteDatabaseTest, CodeExistsEmptyCode) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    EXPECT_FALSE(db_->code_exists(""));
}

TEST_F(SQLiteDatabaseTest, DeleteLinkSuccess) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    EXPECT_TRUE(db_->delete_link(test_code_));
    EXPECT_FALSE(db_->code_exists(test_code_));
}

TEST_F(SQLiteDatabaseTest, DeleteLinkNotFound) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    EXPECT_FALSE(db_->delete_link("nonexistent"));
}

TEST_F(SQLiteDatabaseTest, DeleteLinkCascadesClicks) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    // Add some click records
    ClickRecord click1(test_code_, test_ip_truncated_, test_user_agent_, test_referrer_);
    ClickRecord click2(test_code_, test_ip_truncated_, test_user_agent_, "");
    ASSERT_TRUE(db_->record_click(click1));
    ASSERT_TRUE(db_->record_click(click2));
    
    // Delete the link
    EXPECT_TRUE(db_->delete_link(test_code_));
    
    // Verify clicks are also deleted (CASCADE)
    auto stats = db_->get_click_stats(test_code_);
    EXPECT_EQ(stats.total_clicks, 0);
}

// ===== Analytics Operations Tests =====

TEST_F(SQLiteDatabaseTest, RecordClickSuccess) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    ClickRecord click(test_code_, test_ip_truncated_, test_user_agent_, test_referrer_);
    EXPECT_TRUE(db_->record_click(click));
}

TEST_F(SQLiteDatabaseTest, RecordClickInvalidCode) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    ClickRecord click("nonexistent", test_ip_truncated_, test_user_agent_, test_referrer_);
    EXPECT_FALSE(db_->record_click(click));  // Should fail due to foreign key constraint
}

TEST_F(SQLiteDatabaseTest, RecordClickEmptyFields) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    ClickRecord click(test_code_, "", "", "");  // Empty optional fields
    EXPECT_TRUE(db_->record_click(click));
}

TEST_F(SQLiteDatabaseTest, RecordMultipleClicks) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    // Record multiple clicks
    for (int i = 0; i < 5; ++i) {
        ClickRecord click(test_code_, test_ip_truncated_, test_user_agent_, test_referrer_);
        EXPECT_TRUE(db_->record_click(click));
        this_thread::sleep_for(chrono::milliseconds(10));  // Small delay for different timestamps
    }
}

TEST_F(SQLiteDatabaseTest, GetClickStatsNoClicks) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    auto stats = db_->get_click_stats(test_code_);
    
    EXPECT_EQ(stats.total_clicks, 0);
    EXPECT_TRUE(stats.daily_clicks.empty());
}

TEST_F(SQLiteDatabaseTest, GetClickStatsWithClicks) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    // Record some clicks
    ClickRecord click1(test_code_, test_ip_truncated_, test_user_agent_, test_referrer_);
    ClickRecord click2(test_code_, "192.168.2", "Chrome/90.0", "https://twitter.com");
    
    ASSERT_TRUE(db_->record_click(click1));
    this_thread::sleep_for(chrono::milliseconds(10));
    ASSERT_TRUE(db_->record_click(click2));
    
    auto stats = db_->get_click_stats(test_code_);
    
    EXPECT_EQ(stats.total_clicks, 2);
    EXPECT_GT(stats.daily_clicks.size(), 0);  // Should have at least today's date
    
    // Check that last_accessed is set and recent
    auto now = chrono::system_clock::now();
    auto diff = chrono::duration_cast<chrono::seconds>(now - stats.last_accessed);
    EXPECT_LT(diff.count(), 60);
}

TEST_F(SQLiteDatabaseTest, GetClickStatsNonexistentCode) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    auto stats = db_->get_click_stats("nonexistent");
    
    EXPECT_EQ(stats.total_clicks, 0);
    EXPECT_TRUE(stats.daily_clicks.empty());
}

TEST_F(SQLiteDatabaseTest, GetRecentClicksEmpty) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    auto clicks = db_->get_recent_clicks(test_code_, 10);
    EXPECT_TRUE(clicks.empty());
}

TEST_F(SQLiteDatabaseTest, GetRecentClicksWithData) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    // Record multiple clicks with small delays
    vector<ClickRecord> original_clicks;
    for (int i = 0; i < 3; ++i) {
        ClickRecord click(test_code_, test_ip_truncated_ + to_string(i), 
                         test_user_agent_, test_referrer_);
        original_clicks.push_back(click);
        ASSERT_TRUE(db_->record_click(click));
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    
    auto retrieved_clicks = db_->get_recent_clicks(test_code_, 10);
    
    EXPECT_EQ(retrieved_clicks.size(), 3);
    
    // Verify data integrity (should be in reverse chronological order)
    for (const auto& click : retrieved_clicks) {
        EXPECT_EQ(click.code, test_code_);
        EXPECT_EQ(click.user_agent, test_user_agent_);
        EXPECT_EQ(click.referrer, test_referrer_);
    }
}

TEST_F(SQLiteDatabaseTest, GetRecentClicksWithLimit) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    // Record 5 clicks
    for (int i = 0; i < 5; ++i) {
        ClickRecord click(test_code_, test_ip_truncated_, test_user_agent_, test_referrer_);
        ASSERT_TRUE(db_->record_click(click));
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    
    // Get only 3 most recent
    auto clicks = db_->get_recent_clicks(test_code_, 3);
    EXPECT_EQ(clicks.size(), 3);
}

TEST_F(SQLiteDatabaseTest, GetRecentClicksNonexistentCode) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    auto clicks = db_->get_recent_clicks("nonexistent", 10);
    EXPECT_TRUE(clicks.empty());
}

// ===== Edge Cases and Error Handling =====

TEST_F(SQLiteDatabaseTest, LargeDataHandling) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    // Test with very long strings
    string large_code = string(100, 'x');  // 100 character code
    string large_url = "https://example.com/" + string(5000, 'y');  // 5KB+ URL
    string large_user_agent = string(2000, 'z');  // 2KB user agent
    
    EXPECT_TRUE(db_->insert_link(large_code, large_url, test_type_url_));
    
    auto result = db_->get_link(large_code);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->destination, large_url);
    
    // Record click with large user agent
    ClickRecord large_click(large_code, test_ip_truncated_, large_user_agent, test_referrer_);
    EXPECT_TRUE(db_->record_click(large_click));
}

TEST_F(SQLiteDatabaseTest, SpecialCharactersHandling) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    // Test with special characters and unicode
    string special_code = "αβγ123";
    string special_url = "https://example.com/path?query=test&param=αβγδε";
    string special_user_agent = "Mozilla/5.0 (中文测试; 한국어)";
    string special_referrer = "https://测试.com/路径";
    
    EXPECT_TRUE(db_->insert_link(special_code, special_url, test_type_url_));
    
    auto result = db_->get_link(special_code);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->destination, special_url);
    
    ClickRecord special_click(special_code, test_ip_truncated_, special_user_agent, special_referrer);
    EXPECT_TRUE(db_->record_click(special_click));
    
    auto clicks = db_->get_recent_clicks(special_code, 1);
    ASSERT_EQ(clicks.size(), 1);
    EXPECT_EQ(clicks[0].user_agent, special_user_agent);
    EXPECT_EQ(clicks[0].referrer, special_referrer);
}

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

TEST_F(SQLiteDatabaseTest, DatabaseFilePermissions) {
    // SKIP TEST: in CI environments or when running as root
    if (getenv("CI") != nullptr
    #if defined(__unix__) || defined(__APPLE__)
        || (geteuid && geteuid() == 0)
    #endif
    ) {
        GTEST_SKIP() << "Skipping permission test in CI or when running as root.";
    }

    // Create a read-only directory
    string readonly_dir = "/tmp/readonly_test_dir";
    fs::create_directories(readonly_dir);
    fs::permissions(readonly_dir, fs::perms::owner_read | fs::perms::owner_exec);
    
    string readonly_db_path = readonly_dir + "/test.db";
    
    // Should fail to create database in read-only directory
    EXPECT_FALSE(db_->initialize(readonly_db_path));
    
    // Clean up
    fs::permissions(readonly_dir, fs::perms::all);
    fs::remove_all(readonly_dir);
}

// ===== Concurrency Tests =====

TEST_F(SQLiteDatabaseTest, ConcurrentReads) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    vector<thread> threads;
    vector<bool> results(10, false);
    
    // Launch multiple threads doing reads
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &results, i]() {
            auto result = db_->get_link(test_code_);
            results[i] = result.has_value() && (result->code == test_code_);
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // All reads should succeed
    for (bool result : results) {
        EXPECT_TRUE(result);
    }
}

TEST_F(SQLiteDatabaseTest, ConcurrentWrites) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    vector<thread> threads;
    vector<bool> results(10, false);
    
    // Launch multiple threads recording clicks
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &results, i]() {
            ClickRecord click(test_code_, test_ip_truncated_ + to_string(i), 
                            test_user_agent_, test_referrer_);
            results[i] = db_->record_click(click);
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // All writes should succeed
    for (bool result : results) {
        EXPECT_TRUE(result);
    }
    
    // Verify all clicks were recorded
    auto stats = db_->get_click_stats(test_code_);
    EXPECT_EQ(stats.total_clicks, 10);
}

// ===== Performance Tests =====

TEST_F(SQLiteDatabaseTest, BulkInsertPerformance) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    
    auto start = chrono::high_resolution_clock::now();
    
    // Insert 100 links
    for (int i = 0; i < 100; ++i) {
        string code = "code" + to_string(i);
        string url = "https://example.com/" + to_string(i);
        EXPECT_TRUE(db_->insert_link(code, url, test_type_url_));
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    // Should complete within reasonable time (less than 5 seconds)
    EXPECT_LT(duration.count(), 5000);
}

TEST_F(SQLiteDatabaseTest, BulkClickRecordingPerformance) {
    ASSERT_TRUE(db_->initialize(temp_db_path_));
    ASSERT_TRUE(db_->insert_link(test_code_, test_url_, test_type_url_));
    
    auto start = chrono::high_resolution_clock::now();
    
    // Record 500 clicks
    for (int i = 0; i < 500; ++i) {
        ClickRecord click(test_code_, test_ip_truncated_, test_user_agent_, test_referrer_);
        EXPECT_TRUE(db_->record_click(click));
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    // Should complete within reasonable time (less than 10 seconds)
    EXPECT_LT(duration.count(), 10000);
    
    // Verify count
    auto stats = db_->get_click_stats(test_code_);
    EXPECT_EQ(stats.total_clicks, 500);
} 