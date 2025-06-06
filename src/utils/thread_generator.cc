#include "utils/thread_generator.h"
#include <boost/log/trivial.hpp>

void StartWorkerThreads(boost::asio::io_context& io_context, unsigned int thread_count, std::vector<std::thread>& worker_threads) {
    worker_threads.reserve(thread_count - 1);
    for (unsigned int i = 0; i < thread_count - 1; ++i) {
        worker_threads.emplace_back([&io_context]() {
            try {
                BOOST_LOG_TRIVIAL(info) << "Worker thread " << std::this_thread::get_id() << " started";
                io_context.run();
                BOOST_LOG_TRIVIAL(info) << "Worker thread " << std::this_thread::get_id() << " stopped";
            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << "Worker thread exception: " << e.what();
            }
        });
    }
}