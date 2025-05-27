#include "gtest/gtest.h"
#include <future>
#include <vector>
#include <cstdlib>

TEST(ConcurrencyTest, HandlesMultipleParallelHealthRequests) {
    const int thread_count = 20;
    std::vector<std::future<void>> futures;

    for (int i = 0; i < thread_count; ++i) {
        futures.push_back(std::async(std::launch::async, [i]() {
            std::string cmd = "curl -s http://localhost/health > /dev/null";
            system(cmd.c_str());
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    SUCCEED();
}
