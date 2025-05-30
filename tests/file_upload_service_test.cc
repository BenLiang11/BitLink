#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "services/file_upload_service.h"
#include "database/database_interface.h"
#include "models/link_data.h"
#include <memory>
#include <string>
#include <filesystem>
#include <fstream>

using namespace std;
using ::testing::_;
using ::testing::Return;
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

class FileUploadServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_db_ptr = make_unique<NiceMock<MockDatabaseInterface>>();
        mock_db = mock_db_ptr.get();
        
        // Create test upload directory
        test_upload_dir = "/tmp/test_uploads_" + to_string(time(nullptr));
        filesystem::create_directories(test_upload_dir);
        
        service = make_unique<FileUploadService>(
            move(mock_db_ptr), 
            test_upload_dir,
            "https://short.ly"
        );
    }
    
    void TearDown() override {
        service.reset();
        
        // Clean up test directory
        try {
            filesystem::remove_all(test_upload_dir);
        } catch (...) {
            // Ignore cleanup errors
        }
    }
    
    unique_ptr<NiceMock<MockDatabaseInterface>> mock_db_ptr;
    MockDatabaseInterface* mock_db;
    unique_ptr<FileUploadService> service;
    string test_upload_dir;
    
    // Helper to create test file data
    vector<uint8_t> create_test_file_data(const string& content = "test file content") {
        return vector<uint8_t>(content.begin(), content.end());
    }
};

// Test successful file upload
TEST_F(FileUploadServiceTest, UploadFile_ValidFile_Success) {
    string filename = "test.txt";
    auto file_data = create_test_file_data("Hello, World!");
    
    // Mock expectations - use ANY for generated code and path since they're dynamic
    EXPECT_CALL(*mock_db, code_exists(_))
        .WillOnce(Return(false));  // First attempt generates unique code
    
    EXPECT_CALL(*mock_db, insert_link(_, _, "file"))
        .WillOnce(Return(true));
    
    auto result = service->upload_file(filename, file_data);
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.code.empty());
    EXPECT_EQ(result.short_url.substr(0, 19), "https://short.ly/f/");
    EXPECT_FALSE(result.file_path.empty());
    EXPECT_EQ(result.original_filename, filename);
    EXPECT_TRUE(result.error_message.empty());
    
    // Verify file was actually written to disk
    EXPECT_TRUE(filesystem::exists(result.file_path));
}

// Test file upload with invalid file type
TEST_F(FileUploadServiceTest, UploadFile_InvalidFileType_Failure) {
    string filename = "malware.exe";
    auto file_data = create_test_file_data("malicious content");
    
    auto result = service->upload_file(filename, file_data);
    
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.code.empty());
    EXPECT_TRUE(result.short_url.empty());
    EXPECT_TRUE(result.file_path.empty());
    EXPECT_FALSE(result.error_message.empty());
}

// Test file upload with oversized file
TEST_F(FileUploadServiceTest, UploadFile_OversizedFile_Failure) {
    string filename = "large.txt";
    // Create file data larger than 25MB
    vector<uint8_t> large_file_data(26 * 1024 * 1024, 'A');
    
    auto result = service->upload_file(filename, large_file_data);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message, "File size exceeds maximum limit (25MB)");
}

// Test file upload with invalid filename
TEST_F(FileUploadServiceTest, UploadFile_InvalidFilename_Failure) {
    string invalid_filename = "../../etc/passwd";  // Path traversal attempt
    auto file_data = create_test_file_data("malicious");
    
    auto result = service->upload_file(invalid_filename, file_data);
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// Test file upload when database insertion fails
TEST_F(FileUploadServiceTest, UploadFile_DatabaseInsertFails_Failure) {
    string filename = "test.txt";
    auto file_data = create_test_file_data("test content");
    
    EXPECT_CALL(*mock_db, code_exists(_))
        .WillOnce(Return(false));
    
    EXPECT_CALL(*mock_db, insert_link(_, _, "file"))
        .WillOnce(Return(false));
    
    auto result = service->upload_file(filename, file_data);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message, "Failed to store file record in database");
    
    // Verify no file was left on disk after database failure
    EXPECT_TRUE(result.file_path.empty() || !filesystem::exists(result.file_path));
}

// Test file upload when unique code generation fails
TEST_F(FileUploadServiceTest, UploadFile_UniqueCodeGenerationFails_Failure) {
    string filename = "test.txt";
    auto file_data = create_test_file_data("test content");
    
    // Mock that all codes already exist
    EXPECT_CALL(*mock_db, code_exists(_))
        .WillRepeatedly(Return(true));
    
    auto result = service->upload_file(filename, file_data);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message, "Failed to generate unique code");
}

// Test getting file path for existing file
TEST_F(FileUploadServiceTest, GetFilePath_ExistingFile_Success) {
    string test_code = "abc12345";
    string file_path = "/uploads/test.txt";
    
    LinkData link_data(test_code, file_path, "file");
    
    EXPECT_CALL(*mock_db, get_link(test_code))
        .WillOnce(Return(link_data));
    
    auto result = service->get_file_path(test_code);
    
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), file_path);
}

// Test getting file path for non-existent code
TEST_F(FileUploadServiceTest, GetFilePath_NonExistentCode_ReturnsEmpty) {
    string test_code = "nonexist";
    
    EXPECT_CALL(*mock_db, get_link(test_code))
        .WillOnce(Return(nullopt));
    
    auto result = service->get_file_path(test_code);
    
    EXPECT_FALSE(result.has_value());
}

// Test getting file path for URL type (should not return)
TEST_F(FileUploadServiceTest, GetFilePath_URLType_ReturnsEmpty) {
    string test_code = "abc12345";
    string url = "https://example.com";
    
    LinkData link_data(test_code, url, "url");
    
    EXPECT_CALL(*mock_db, get_link(test_code))
        .WillOnce(Return(link_data));
    
    auto result = service->get_file_path(test_code);
    
    EXPECT_FALSE(result.has_value());
}

// Test getting file metadata
TEST_F(FileUploadServiceTest, GetFileMetadata_ExistingFile_Success) {
    string test_code = "abc12345";
    string file_path = "/uploads/test.txt";
    
    LinkData link_data(test_code, file_path, "file");
    
    EXPECT_CALL(*mock_db, get_link(test_code))
        .WillOnce(Return(link_data));
    
    auto result = service->get_file_metadata(test_code);
    
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->code, test_code);
    EXPECT_EQ(result->destination, file_path);
    EXPECT_EQ(result->type, "file");
}

// Test deleting existing file
TEST_F(FileUploadServiceTest, DeleteFile_ExistingFile_Success) {
    string test_code = "abc12345";
    string file_path = test_upload_dir + "/test_file.txt";
    
    // Create actual file on disk
    ofstream test_file(file_path);
    test_file << "test content";
    test_file.close();
    
    LinkData link_data(test_code, file_path, "file");
    
    EXPECT_CALL(*mock_db, get_link(test_code))
        .WillOnce(Return(link_data));
    
    EXPECT_CALL(*mock_db, delete_link(test_code))
        .WillOnce(Return(true));
    
    bool result = service->delete_file(test_code);
    
    EXPECT_TRUE(result);
    // Verify file was removed from disk
    EXPECT_FALSE(filesystem::exists(file_path));
}

// Test deleting non-existent file
TEST_F(FileUploadServiceTest, DeleteFile_NonExistentFile_Failure) {
    string test_code = "nonexist";
    
    EXPECT_CALL(*mock_db, get_link(test_code))
        .WillOnce(Return(nullopt));
    
    bool result = service->delete_file(test_code);
    
    EXPECT_FALSE(result);
}

// Test recording file access
TEST_F(FileUploadServiceTest, RecordFileAccess_ValidCode_Success) {
    string test_code = "abc12345";
    string file_path = "/uploads/test.txt";
    string ip_address = "192.168.1.100";
    string user_agent = "Mozilla/5.0";
    string referrer = "https://google.com";
    
    LinkData link_data(test_code, file_path, "file");
    
    EXPECT_CALL(*mock_db, get_link(test_code))
        .WillOnce(Return(link_data));
    
    EXPECT_CALL(*mock_db, record_click(_))
        .WillOnce(Return(true));
    
    bool result = service->record_file_access(test_code, ip_address, user_agent, referrer);
    
    EXPECT_TRUE(result);
}

// Test recording access for non-existent file
TEST_F(FileUploadServiceTest, RecordFileAccess_InvalidCode_Failure) {
    string test_code = "nonexist";
    string ip_address = "192.168.1.100";
    string user_agent = "Mozilla/5.0";
    
    EXPECT_CALL(*mock_db, get_link(test_code))
        .WillOnce(Return(nullopt));
    
    bool result = service->record_file_access(test_code, ip_address, user_agent);
    
    EXPECT_FALSE(result);
}

// Test recording access for URL type (should fail)
TEST_F(FileUploadServiceTest, RecordFileAccess_URLType_Failure) {
    string test_code = "abc12345";
    string url = "https://example.com";
    string ip_address = "192.168.1.100";
    string user_agent = "Mozilla/5.0";
    
    LinkData link_data(test_code, url, "url");
    
    EXPECT_CALL(*mock_db, get_link(test_code))
        .WillOnce(Return(link_data));
    
    bool result = service->record_file_access(test_code, ip_address, user_agent);
    
    EXPECT_FALSE(result);
}

// Test IP privacy truncation
class FileUploadServicePrivacyTest : public FileUploadServiceTest {
};

TEST_F(FileUploadServicePrivacyTest, RecordFileAccess_IPv4_TruncatesCorrectly) {
    string test_code = "abc12345";
    string file_path = "/uploads/test.txt";
    string ipv4_address = "192.168.1.100";
    
    LinkData link_data(test_code, file_path, "file");
    
    EXPECT_CALL(*mock_db, get_link(test_code))
        .WillOnce(Return(link_data));
    
    // Verify that record_click is called with truncated IP
    EXPECT_CALL(*mock_db, record_click(_))
        .WillOnce([](const ClickRecord& record) {
            // The IP should be truncated to 192.168.1.0
            EXPECT_EQ(record.ip_truncated, "192.168.1.0");
            return true;
        });
    
    service->record_file_access(test_code, ipv4_address, "Mozilla/5.0");
}

// Test URL construction
class FileUploadServiceURLTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_db_ptr = make_unique<NiceMock<MockDatabaseInterface>>();
        mock_db = mock_db_ptr.get();
        
        test_upload_dir = "/tmp/test_uploads_url_" + to_string(time(nullptr));
        filesystem::create_directories(test_upload_dir);
        
        service = make_unique<FileUploadService>(
            move(mock_db_ptr), 
            test_upload_dir,
            "https://short.ly"
        );
    }
    
    void TearDown() override {
        service.reset();
        try {
            filesystem::remove_all(test_upload_dir);
        } catch (...) {}
    }
    
    unique_ptr<NiceMock<MockDatabaseInterface>> mock_db_ptr;
    MockDatabaseInterface* mock_db;
    unique_ptr<FileUploadService> service;
    string test_upload_dir;
};

TEST_F(FileUploadServiceURLTest, BuildShortURL_FileService_UsesCorrectPrefix) {
    string filename = "test.txt";
    vector<uint8_t> file_data = {'t', 'e', 's', 't'};
    
    EXPECT_CALL(*mock_db, code_exists(_))
        .WillOnce(Return(false));
    
    EXPECT_CALL(*mock_db, insert_link(_, _, "file"))
        .WillOnce(Return(true));
    
    auto result = service->upload_file(filename, file_data);
    
    EXPECT_TRUE(result.success);
    // File service should use /f/ prefix instead of /r/
    EXPECT_TRUE(result.short_url.find("https://short.ly/f/") == 0);
    EXPECT_TRUE(result.short_url.find("/r/") == string::npos);
} 