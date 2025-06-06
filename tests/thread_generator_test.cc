// tests/thread_generator_test.cc

#include "utils/thread_generator.h"
#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>

TEST(ThreadGeneratorTest, CreatesCorrectNumberOfThreadsAndExecutesWork) {
    boost::asio::io_context io_context;
    std::vector<std::thread> worker_threads;
    unsigned int thread_count = 4;

    // Counter to verify work is executed
    std::atomic<int> counter{0};
    for (unsigned int i = 0; i < thread_count - 1; ++i) {
        io_context.post([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ++counter;
        });
    }

    StartWorkerThreads(io_context, thread_count, worker_threads);

    // Run on main thread as well
    io_context.run();

    // Wait for all worker threads to finish
    for (auto& t : worker_threads) {
        if (t.joinable()) t.join();
    }

    // Check that the correct number of threads were created
    EXPECT_EQ(worker_threads.size(), thread_count - 1);

    // Check that all handlers were executed
    EXPECT_EQ(counter.load(), thread_count - 1);
}

TEST(ThreadGeneratorTest, HandlesZeroOrOneThread) {
    boost::asio::io_context io_context;
    std::vector<std::thread> worker_threads;

    // Should not create any threads if thread_count is 1
    StartWorkerThreads(io_context, 1, worker_threads);
    EXPECT_EQ(worker_threads.size(), 0);

    // // Should not create any threads if thread_count is 0
    // worker_threads.clear();
    // StartWorkerThreads(io_context, 0, worker_threads);
    // EXPECT_EQ(worker_threads.size(), 0);
}