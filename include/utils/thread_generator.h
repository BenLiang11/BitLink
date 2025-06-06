#pragma once
#include <boost/asio/io_context.hpp>
#include <thread>
#include <vector>

// Starts (thread_count - 1) worker threads that run the io_context event loop.
void StartWorkerThreads(boost::asio::io_context& io_context, unsigned int thread_count, std::vector<std::thread>& worker_threads);