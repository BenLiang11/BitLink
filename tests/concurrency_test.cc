#include "gtest/gtest.h"
#include <future>
#include <vector>
#include <cstdlib>
#include <string>

TEST(ConcurrencyTest, HandlesMultipleParallelHealthRequests) {
    const int thread_count = 5;
    std::vector<std::future<int>> futures;

    for (int i = 0; i < thread_count; ++i) {
        futures.push_back(std::async(std::launch::async, [i]() -> int {
            std::string cmd = "curl -s -o /dev/null -w \"%{http_code}\" http://localhost/health";
            FILE* pipe = popen(cmd.c_str(), "r");
            if (!pipe) return -1;

            char buffer[4]; 
            fgets(buffer, sizeof(buffer), pipe);
            pclose(pipe);

            return std::atoi(buffer);
        }));
    }

    for (auto& f : futures) {
        int status_code = f.get();
        EXPECT_EQ(status_code, 200);
    }
}
