#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

TEST(LoggerTest, LogFileCreation) {
    EXPECT_TRUE(fs::exists("logs")); // logs 폴더 존재 확인
    auto it = fs::directory_iterator("logs");
    EXPECT_TRUE(it != fs::end(it));  // 파일 하나 이상 있어야함
}

TEST(LoggerTest, LogFileContent) {
    for (const auto& entry : fs::directory_iterator("logs")) {
        std::ifstream file(entry.path());
        std::string line;
        bool found = false;
        while (std::getline(file, line)) {
            if (line.find("Server starting up") != std::string::npos) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found); // 로그 내용 안에 "Server starting up" 있어야 함
    }
}
