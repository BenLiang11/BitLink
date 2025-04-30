#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>
#include <thread>
#include "../include/logger.h"
#include <boost/log/trivial.hpp>

namespace fs = std::filesystem;

// Helper to get number of log files
size_t count_log_files() {
    size_t count = 0;
    for (const auto& entry : fs::directory_iterator("logs")) {
        if (entry.path().extension() == ".log") count++;
    }
    return count;
}

// TEST(LoggerTest, LogRotation) {
//     init_logging();  // Initialize logger with rotation settings

//     size_t initial_count = count_log_files();

//     // Generate a large number of logs to trigger 10MB rotation
//     for (int i = 0; i < 100000; ++i) {
//         BOOST_LOG_TRIVIAL(info) << "RotationTest message " << i
//                                 << " - This is a long log line to fill space. Lorem ipsum dolor sit amet, consectetur adipiscing elit.";
//     }

//     // Give Boost.Log some time to flush to disk
//     std::this_thread::sleep_for(std::chrono::seconds(2));

//     size_t after_count = count_log_files();
//     EXPECT_GT(after_count, initial_count);  // Expect more log files created after rotation
// }
