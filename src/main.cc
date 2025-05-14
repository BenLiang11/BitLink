#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <map>
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

int main(int argc, char* argv[]) {
  init_logging();
  BOOST_LOG_TRIVIAL(info) << "Server starting up...";
  try {
    if (argc != 2) {
      std::cerr << "Usage: webserver <config_file>\n";
      return 1;
    }

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

    assert(HandlerRegistry::RegisterHandler("EchoHandler", EchoHandler::Create));
    assert(HandlerRegistry::RegisterHandler("StaticHandler", StaticFileHandler::Create));
    assert(HandlerRegistry::RegisterHandler("NotFoundHandler", NotFoundHandler::Create));
    
    // Create handler registrations based on configuration
    auto handler_registrations = server_config.CreateHandlerRegistrations();
    
    // Create handler dispatcher with the registrations
    HandlerDispatcher handler_dispatcher(handler_registrations);  

    boost::asio::io_context io_context;

    // Start service on read port with configured handlers
    handler_server s(io_context, server_config.port(), handler_dispatcher);
    BOOST_LOG_TRIVIAL(info) << "Server running on port " << server_config.port();
    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  BOOST_LOG_TRIVIAL(info) << "Server shutting down...";
  
  return 0;
}
