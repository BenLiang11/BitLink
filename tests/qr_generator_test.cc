#include <gtest/gtest.h>
#include "utils/qr_generator.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;

class QRGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = "test_qr_output";
        fs::create_directories(test_dir_);
    }
    
    void TearDown() override {
        // Clean up test files
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }
    
    string test_dir_;
    
    // Helper function to check if data looks like PNG
    bool is_valid_png_format(const vector<uint8_t>& data) {
        // Check PNG signature
        const vector<uint8_t> png_signature = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
        return data.size() >= png_signature.size() && 
               equal(png_signature.begin(), png_signature.end(), data.begin());
    }
    
    // Helper function to check if string is valid base64
    bool is_valid_base64(const string& str) {
        if (str.empty() || str.size() % 4 != 0) return false;
        
        for (char c : str) {
            if (!isalnum(c) && c != '+' && c != '/' && c != '=') {
                return false;
            }
        }
        return true;
    }
    
    // Helper function to decode base64 for testing
    vector<uint8_t> base64_decode(const string& encoded) {
        const string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        vector<uint8_t> result;
        int val = 0, valb = -8;
        
        for (char c : encoded) {
            if (c == '=') break;
            
            size_t pos = chars.find(c);
            if (pos == string::npos) continue;
            
            val = (val << 6) + pos;
            valb += 6;
            if (valb >= 0) {
                result.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return result;
    }
};

// Test generate_png with valid data
TEST_F(QRGeneratorTest, GeneratePngValidData) {
    string test_data = "https://example.com";
    vector<uint8_t> png_data = QRCodeGenerator::generate_png(test_data);
    
    EXPECT_FALSE(png_data.empty());
    EXPECT_TRUE(is_valid_png_format(png_data));
}

// Test generate_png with different sizes
TEST_F(QRGeneratorTest, GeneratePngDifferentSizes) {
    string test_data = "test";
    
    vector<int> sizes = {50, 100, 200, 400};
    for (int size : sizes) {
        vector<uint8_t> png_data = QRCodeGenerator::generate_png(test_data, size);
        
        EXPECT_FALSE(png_data.empty()) << "Failed for size: " << size;
        EXPECT_TRUE(is_valid_png_format(png_data)) << "Invalid PNG for size: " << size;
    }
}

// Test generate_png with different error correction levels
TEST_F(QRGeneratorTest, GeneratePngDifferentErrorCorrection) {
    string test_data = "test data for error correction";
    
    vector<QRCodeGenerator::ErrorCorrection> levels = {
        QRCodeGenerator::ErrorCorrection::LOW,
        QRCodeGenerator::ErrorCorrection::MEDIUM,
        QRCodeGenerator::ErrorCorrection::QUARTILE,
        QRCodeGenerator::ErrorCorrection::HIGH
    };
    
    for (auto level : levels) {
        vector<uint8_t> png_data = QRCodeGenerator::generate_png(test_data, 200, level);
        
        EXPECT_FALSE(png_data.empty()) << "Failed for error correction level";
        EXPECT_TRUE(is_valid_png_format(png_data)) << "Invalid PNG for error correction level";
    }
}

// Test generate_png with empty data
TEST_F(QRGeneratorTest, GeneratePngEmptyData) {
    vector<uint8_t> png_data = QRCodeGenerator::generate_png("");
    
    EXPECT_TRUE(png_data.empty());
}

// Test generate_png with invalid size
TEST_F(QRGeneratorTest, GeneratePngInvalidSize) {
    string test_data = "test";
    
    // Test zero size
    vector<uint8_t> png_data = QRCodeGenerator::generate_png(test_data, 0);
    EXPECT_TRUE(png_data.empty());
    
    // Test negative size
    png_data = QRCodeGenerator::generate_png(test_data, -100);
    EXPECT_TRUE(png_data.empty());
}

// Test generate_png with very large data
TEST_F(QRGeneratorTest, GeneratePngLargeData) {
    // Create a large string (QR codes have data limits)
    string large_data(1000, 'A');
    vector<uint8_t> png_data = QRCodeGenerator::generate_png(large_data);
    
    // Should either succeed or fail gracefully
    if (!png_data.empty()) {
        EXPECT_TRUE(is_valid_png_format(png_data));
    }
}

// Test generate_png with special characters
TEST_F(QRGeneratorTest, GeneratePngSpecialCharacters) {
    string special_data = "Hello! @#$%^&*()_+-=[]{}|;':\",./<>?`~";
    vector<uint8_t> png_data = QRCodeGenerator::generate_png(special_data);
    
    EXPECT_FALSE(png_data.empty());
    EXPECT_TRUE(is_valid_png_format(png_data));
}

// Test generate_png with Unicode characters
TEST_F(QRGeneratorTest, GeneratePngUnicodeCharacters) {
    string unicode_data = "Hello 世界! 🌍 café naïve résumé";
    vector<uint8_t> png_data = QRCodeGenerator::generate_png(unicode_data);
    
    EXPECT_FALSE(png_data.empty());
    EXPECT_TRUE(is_valid_png_format(png_data));
}

// Test save_qr_code with valid data
TEST_F(QRGeneratorTest, SaveQrCodeValidData) {
    string test_data = "https://example.com";
    string file_path = test_dir_ + "/test_qr.png";
    
    bool result = QRCodeGenerator::save_qr_code(test_data, file_path);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(fs::exists(file_path));
    
    // Verify file content
    ifstream file(file_path, ios::binary);
    EXPECT_TRUE(file.good());
    
    vector<uint8_t> file_data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    EXPECT_TRUE(is_valid_png_format(file_data));
}

// Test save_qr_code with different parameters
TEST_F(QRGeneratorTest, SaveQrCodeDifferentParameters) {
    string test_data = "test save";
    string file_path = test_dir_ + "/test_qr_params.png";
    
    bool result = QRCodeGenerator::save_qr_code(
        test_data, file_path, 150, QRCodeGenerator::ErrorCorrection::HIGH);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(fs::exists(file_path));
}

// Test save_qr_code with empty file path
TEST_F(QRGeneratorTest, SaveQrCodeEmptyFilePath) {
    string test_data = "test";
    
    bool result = QRCodeGenerator::save_qr_code(test_data, "");
    
    EXPECT_FALSE(result);
}

// Test save_qr_code with invalid directory
TEST_F(QRGeneratorTest, SaveQrCodeInvalidDirectory) {
    string test_data = "test";
    string file_path = "/nonexistent/path/test.png";
    
    bool result = QRCodeGenerator::save_qr_code(test_data, file_path);
    
    EXPECT_FALSE(result);
}

// Test save_qr_code with empty data
TEST_F(QRGeneratorTest, SaveQrCodeEmptyData) {
    string file_path = test_dir_ + "/empty_data.png";
    
    bool result = QRCodeGenerator::save_qr_code("", file_path);
    
    EXPECT_FALSE(result);
    EXPECT_FALSE(fs::exists(file_path));
}

// Test save_qr_code file overwrite
TEST_F(QRGeneratorTest, SaveQrCodeFileOverwrite) {
    string test_data = "test overwrite";
    string file_path = test_dir_ + "/overwrite.png";
    
    // Create initial file
    bool result1 = QRCodeGenerator::save_qr_code(test_data, file_path);
    EXPECT_TRUE(result1);
    EXPECT_TRUE(fs::exists(file_path));
    
    // Get file size
    auto initial_size = fs::file_size(file_path);
    
    // Overwrite with different data
    string new_data = "different test data for overwrite test case";
    bool result2 = QRCodeGenerator::save_qr_code(new_data, file_path);
    EXPECT_TRUE(result2);
    
    // File should still exist and may have different size
    EXPECT_TRUE(fs::exists(file_path));
}

// Test generate_data_url with valid data
TEST_F(QRGeneratorTest, GenerateDataUrlValidData) {
    string test_data = "https://example.com";
    string data_url = QRCodeGenerator::generate_data_url(test_data);
    
    EXPECT_FALSE(data_url.empty());
    EXPECT_TRUE(data_url.substr(0, 22) == "data:image/png;base64,");
    
    // Extract and validate base64 part
    size_t comma_pos = data_url.find(',');
    ASSERT_NE(comma_pos, string::npos);
    
    string base64_part = data_url.substr(comma_pos + 1);
    EXPECT_TRUE(is_valid_base64(base64_part));
    
    // Decode and verify PNG format
    vector<uint8_t> decoded_data = base64_decode(base64_part);
    EXPECT_TRUE(is_valid_png_format(decoded_data));
}

// Test generate_data_url with different parameters
TEST_F(QRGeneratorTest, GenerateDataUrlDifferentParameters) {
    string test_data = "test data url";
    string data_url = QRCodeGenerator::generate_data_url(
        test_data, 100, QRCodeGenerator::ErrorCorrection::LOW);
    
    EXPECT_FALSE(data_url.empty());
    EXPECT_TRUE(data_url.substr(0, 22) == "data:image/png;base64,");
}

// Test generate_data_url with empty data
TEST_F(QRGeneratorTest, GenerateDataUrlEmptyData) {
    string data_url = QRCodeGenerator::generate_data_url("");
    
    EXPECT_TRUE(data_url.empty());
}

// Test generate_data_url with invalid size
TEST_F(QRGeneratorTest, GenerateDataUrlInvalidSize) {
    string test_data = "test";
    
    string data_url = QRCodeGenerator::generate_data_url(test_data, 0);
    EXPECT_TRUE(data_url.empty());
    
    data_url = QRCodeGenerator::generate_data_url(test_data, -50);
    EXPECT_TRUE(data_url.empty());
}

// Test consistency between methods
TEST_F(QRGeneratorTest, MethodConsistency) {
    string test_data = "consistency test";
    int size = 150;
    auto error_correction = QRCodeGenerator::ErrorCorrection::MEDIUM;
    
    // Generate using different methods
    vector<uint8_t> png_data = QRCodeGenerator::generate_png(test_data, size, error_correction);
    
    string file_path = test_dir_ + "/consistency.png";
    bool save_result = QRCodeGenerator::save_qr_code(test_data, file_path, size, error_correction);
    
    string data_url = QRCodeGenerator::generate_data_url(test_data, size, error_correction);
    
    // All methods should succeed
    EXPECT_FALSE(png_data.empty());
    EXPECT_TRUE(save_result);
    EXPECT_FALSE(data_url.empty());
    
    // Read saved file
    ifstream file(file_path, ios::binary);
    vector<uint8_t> file_data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    
    // PNG data should match file data
    EXPECT_EQ(png_data, file_data);
    
    // Extract data from data URL
    size_t comma_pos = data_url.find(',');
    string base64_part = data_url.substr(comma_pos + 1);
    vector<uint8_t> url_data = base64_decode(base64_part);
    
    // Data URL content should match PNG data
    EXPECT_EQ(png_data, url_data);
}

// Performance test with multiple QR codes
TEST_F(QRGeneratorTest, PerformanceTest) {
    vector<string> test_data = {
        "Performance test 1",
        "Performance test 2", 
        "Performance test 3",
        "Performance test 4",
        "Performance test 5"
    };
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        vector<uint8_t> png_data = QRCodeGenerator::generate_png(test_data[i]);
        EXPECT_FALSE(png_data.empty()) << "Failed for data: " << test_data[i];
        EXPECT_TRUE(is_valid_png_format(png_data)) << "Invalid PNG for data: " << test_data[i];
    }
}

// Edge case: very small size
TEST_F(QRGeneratorTest, VerySmallSize) {
    string test_data = "small";
    vector<uint8_t> png_data = QRCodeGenerator::generate_png(test_data, 1);
    
    if (!png_data.empty()) {
        EXPECT_TRUE(is_valid_png_format(png_data));
    }
}

// Edge case: very large size
TEST_F(QRGeneratorTest, VeryLargeSize) {
    string test_data = "large";
    vector<uint8_t> png_data = QRCodeGenerator::generate_png(test_data, 2000);
    
    // Should succeed (though image will be large)
    EXPECT_FALSE(png_data.empty());
    EXPECT_TRUE(is_valid_png_format(png_data));
}

// Test with URL data
TEST_F(QRGeneratorTest, UrlData) {
    vector<string> urls = {
        "https://www.example.com",
        "http://test.org/path?param=value",
        "ftp://files.example.com/file.txt",
        "mailto:test@example.com",
        "tel:+1234567890"
    };
    
    for (const string& url : urls) {
        vector<uint8_t> png_data = QRCodeGenerator::generate_png(url);
        EXPECT_FALSE(png_data.empty()) << "Failed for URL: " << url;
        EXPECT_TRUE(is_valid_png_format(png_data)) << "Invalid PNG for URL: " << url;
    }
}

// Test with numeric data
TEST_F(QRGeneratorTest, NumericData) {
    vector<string> numbers = {
        "123456789",
        "0",
        "999999999999999",
        "42"
    };
    
    for (const string& number : numbers) {
        vector<uint8_t> png_data = QRCodeGenerator::generate_png(number);
        EXPECT_FALSE(png_data.empty()) << "Failed for number: " << number;
        EXPECT_TRUE(is_valid_png_format(png_data)) << "Invalid PNG for number: " << number;
    }
}

// Test file permissions (read-only directory)
#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif
TEST_F(QRGeneratorTest, ReadOnlyDirectory) {
    // Skip in CI environments or when running as root (permissions can't be enforced)
    if (getenv("CI") != nullptr
    #if defined(__unix__) || defined(__APPLE__)
            || (geteuid && geteuid() == 0)
    #endif
    ) {
        GTEST_SKIP() << "Skipping permission test in CI or when running as root.";
    }

    string readonly_dir = test_dir_ + "/readonly";
    fs::create_directories(readonly_dir);
    
    // Make directory read-only (this may not work on all systems)
    fs::permissions(readonly_dir, fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read);
    
    string file_path = readonly_dir + "/test.png";
    bool result = QRCodeGenerator::save_qr_code("test", file_path);
    
    // Should fail to write to read-only directory
    EXPECT_FALSE(result);
    
    // Restore permissions for cleanup
    fs::permissions(readonly_dir, fs::perms::all);
} 