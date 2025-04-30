#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <string>
#include <iostream>
#include <thread>

#include "logger.h"
#include <boost/log/trivial.hpp>

namespace fs = std::filesystem;

TEST(LoggerTest, LogFileCreation) {
    init_logging();
    BOOST_LOG_TRIVIAL(info) << "LoggerTest: Checking log file creation.";

    std::this_thread::sleep_for(std::chrono::milliseconds(300));


    EXPECT_TRUE(fs::exists("logs")); // check logs directory exists

    auto it = fs::directory_iterator("logs");
    EXPECT_TRUE(it != fs::end(it));  // at least one log file should exist
}

TEST(LoggerTest, LogFileContent) {
    init_logging();
    BOOST_LOG_TRIVIAL(info) << "starting up"; 
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

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
