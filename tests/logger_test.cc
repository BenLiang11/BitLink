#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <string>
#include <iostream>

#include "logger.h"
#include <boost/log/trivial.hpp>

namespace fs = std::filesystem;

TEST(LoggerTest, LogFileCreation) {
    init_logging();  // ensure logger is initialized for this test
    BOOST_LOG_TRIVIAL(info) << "LoggerTest: Checking log file creation.";

    EXPECT_TRUE(fs::exists("logs")); // check logs directory exists

    auto it = fs::directory_iterator("logs");
    EXPECT_TRUE(it != fs::end(it));  // at least one log file should exist
}

TEST(LoggerTest, LogFileContent) {
    bool found = false;

    for (const auto& entry : fs::directory_iterator("logs")) {
        std::ifstream file(entry.path());
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("starting up") != std::string::npos) {
                found = true;
                break;
            }
        }
    }

    EXPECT_TRUE(found);  // check if the log file contains "starting up"
}
