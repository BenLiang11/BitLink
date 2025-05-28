#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <map>
#include <thread>
#include <vector>
#include <signal.h>
#include "server.h"
#include "session.h"
#include "handlers/base_handler.h"
#include "config_parser.h"
#include "logger.h"
#include <boost/log/trivial.hpp>
#include "server_config.h"
#include "handler_server.h"
#include "handler_dispatcher.h"
#include "handlers/echo_handler.h"
#include "handlers/static_file_handler.h"
#include "handlers/not_found_handler.h"
#include "handlers/api_handler.h"
#include "handlers/health_handler.h"
#include "handlers/sleep_handler.h"

// Global variables for graceful shutdown
std::unique_ptr<boost::asio::io_context> global_io_context = nullptr;
std::vector<std::thread> worker_threads;
void signal_handler(int signal) {
    BOOST_LOG_TRIVIAL(info) << "Received signal " << signal << ", shutting down gracefully...";
    if (global_io_context) {
        global_io_context->stop();
    }
}
int main(int argc, char* argv[]) {
    init_logging();
    BOOST_LOG_TRIVIAL(info) << "Enhanced multithreaded server starting up...";
    
    try {
        if (argc != 2) {
            std::cerr << "Usage: webserver <config_file>\n";
            return 1;
        }
        // Set up signal handlers for graceful shutdown
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        // Parse config file
        NginxConfigParser parser;
        NginxConfig config;
        if (!parser.Parse(argv[1], &config)) {
            std::cerr << "Failed to parse config file\n";
            return 1;
        }
        // Parse server configuration
        ServerConfig server_config;
        if (!server_config.ParseConfig(config)) {
            std::cerr << "Failed to parse server configuration\n";
            return 1;
        }
        // Register all handlers
        assert(HandlerRegistry::RegisterHandler("EchoHandler", EchoHandler::Create));
        assert(HandlerRegistry::RegisterHandler("StaticHandler", StaticFileHandler::Create));
        assert(HandlerRegistry::RegisterHandler("NotFoundHandler", NotFoundHandler::Create));
        assert(HandlerRegistry::RegisterHandler("ApiHandler", ApiHandler::Create));
        assert(HandlerRegistry::RegisterHandler("HealthHandler", HealthHandler::Create));
        assert(HandlerRegistry::RegisterHandler("SleepHandler", SleepHandler::Create));
        // Create handler registrations based on configuration
        auto handler_registrations = server_config.CreateHandlerRegistrations();
        
        // Add health handler if not already configured
        if (handler_registrations.find("/health") == handler_registrations.end()) {
            HandlerRegistration health_reg;
            health_reg.location = "/health";
            health_reg.handler_name = "HealthHandler";
            health_reg.args = {};
            handler_registrations["/health"] = health_reg;
            BOOST_LOG_TRIVIAL(info) << "Added default health check endpoint at /health";
        }
        
        // Create handler dispatcher with the registrations
        HandlerDispatcher handler_dispatcher(handler_registrations);
        // Determine number of threads (default to hardware concurrency)
        unsigned int thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0) thread_count = 4; // Fallback
        
        // Check for thread count environment variable
        const char* thread_env = std::getenv("SERVER_THREADS");
        if (thread_env) {
            try {
                unsigned int env_threads = std::stoul(thread_env);
                if (env_threads > 0 && env_threads <= 32) { // Reasonable limits
                    thread_count = env_threads;
                }
            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(warning) << "Invalid SERVER_THREADS value, using default: " << e.what();
            }
        }
        BOOST_LOG_TRIVIAL(info) << "Starting server with " << thread_count << " worker threads";
        // Create global IO context
        global_io_context = std::make_unique<boost::asio::io_context>();
        
        // Create and start server
        handler_server s(*global_io_context, server_config.port(), handler_dispatcher);
        
        BOOST_LOG_TRIVIAL(info) << "Server running on port " << server_config.port() 
                               << " with " << thread_count << " worker threads";
        BOOST_LOG_TRIVIAL(info) << "Health check available at: http://localhost:" << server_config.port() << "/health";
        
        // Log server configuration
        BOOST_LOG_TRIVIAL(info) << "Registered " << handler_registrations.size() << " handlers:";
        for (const auto& reg : handler_registrations) {
            BOOST_LOG_TRIVIAL(info) << "  " << reg.first << " -> " << reg.second.handler_name;
        }
        
        // Create worker threads
        worker_threads.reserve(thread_count - 1);
        for (unsigned int i = 0; i < thread_count - 1; ++i) {
            worker_threads.emplace_back([&]() {
                try {
                    BOOST_LOG_TRIVIAL(info) << "Worker thread " << std::this_thread::get_id() << " started";
                    global_io_context->run();
                    BOOST_LOG_TRIVIAL(info) << "Worker thread " << std::this_thread::get_id() << " stopped";
                } catch (const std::exception& e) {
                    BOOST_LOG_TRIVIAL(error) << "Worker thread exception: " << e.what();
                }
            });
        }
        
        // Run on main thread as well
        BOOST_LOG_TRIVIAL(info) << "Main thread starting event loop";
        global_io_context->run();
        
        // Wait for all worker threads to finish
        BOOST_LOG_TRIVIAL(info) << "Waiting for worker threads to finish...";
        for (auto& thread : worker_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        BOOST_LOG_TRIVIAL(error) << "Server exception: " << e.what();
        
        // Clean up threads on exception
        if (global_io_context) {
            global_io_context->stop();
        }
        for (auto& thread : worker_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        return 1;
    }
    BOOST_LOG_TRIVIAL(info) << "Server shutting down gracefully...";
    return 0;
}
