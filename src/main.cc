#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "server.h"
#include "session.h"

#include "../include/config_parser.h"

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }

    // Parse config file
    NginxConfigParser parser;
    NginxConfig config;
    if (!parser.Parse(argv[1], &config)) {
      std::cerr << "Failed to parse config file\n";
      return 1;
    }

    // Extract port number from config
    int port = -1;
    for (const auto& statement : config.statements_) {
      if (statement->tokens_.size() >= 2 && statement->tokens_[0] == "listen") {
        port = std::stoi(statement->tokens_[1]);
        break;
      }
    }

    // Terminate if there is no valid listen port
    if (port == -1) {
      std::cerr << "No valid 'listen' directive found\n";
      return 1;
    }

    // Create I/O service
    boost::asio::io_service io_service;

    using namespace std;

    //Start service on read port
    server s(io_service, atoi(argv[1]));

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}