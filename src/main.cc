#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <map>
#include "server.h"
#include "session.h"
#include "handlers/base_handler.h"
#include "handlers/echo_handler.h"
#include "handlers/static_file_handler.h"
#include "config_parser.h"

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
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

    // Extract port number from config
    int port = -1;
    std::map<std::string, std::shared_ptr<RequestHandler>> path_to_handler;
    std::string static_file_root;

    for (const auto& statement : config.statements_) {
      // Handle port configuration
      if (statement->tokens_.size() >= 2 && statement->tokens_[0] == "listen") {
        port = std::stoi(statement->tokens_[1]);
      }
      
      // Handle static file root directory
      if (statement->tokens_.size() >= 2 && statement->tokens_[0] == "root") {
        static_file_root = statement->tokens_[1];
      }
      
      // Handle path mappings
      if (statement->tokens_.size() >= 3 && statement->tokens_[0] == "location") {
        std::string path = statement->tokens_[1];
        std::string handler_type = statement->tokens_[2];
        
        if (handler_type == "echo") {
          path_to_handler[path] = std::make_shared<EchoHandler>();
          std::cout << "Added echo handler for path: " << path << std::endl;
        } else if (handler_type == "static" && !static_file_root.empty()) {
          path_to_handler[path] = std::make_shared<StaticFileHandler>(static_file_root);
          std::cout << "Added static file handler for path: " << path 
                    << " with root: " << static_file_root << std::endl;
        }
      }
    }

    // Terminate if there is no valid listen port
    if (port == -1) {
      std::cerr << "No valid 'listen' directive found in config\n";
      return 1;
    }

    // Ensure we have at least one handler
    if (path_to_handler.empty()) {
      // Add a default echo handler for "/"
      path_to_handler["/"] = std::make_shared<EchoHandler>();
      std::cout << "No handlers configured. Added default echo handler for path: /" << std::endl;
    }

    // Create I/O service
    boost::asio::io_service io_service;

    // Create a custom server that will set handlers based on request path
    // This is a placeholder for the actual server implementation
    // TODO: Implement an actual server dispatcher to the correct handler and unit test it
    class handler_server : public server {
    public:
      handler_server(boost::asio::io_service& io_service, 
                    short port,
                    const std::map<std::string, std::shared_ptr<RequestHandler>>& handlers)
        : server(io_service, port), handlers_(handlers) {}
      
      void handle_accept(session* new_session, const boost::system::error_code& error) override {
        if (!error) {
          // For now, just use the first handler (we'll improve this later)
          if (!handlers_.empty()) {
            new_session->set_handler(handlers_.begin()->second);
          }
          new_session->start();
          
          // Start accepting another connection
          new_session = new session(io_service_);
          acceptor_.async_accept(new_session->socket(),
            boost::bind(&handler_server::handle_accept, this, new_session,
              boost::asio::placeholders::error));
        } else {
          delete new_session;
        }
      }
      
    private:
      std::map<std::string, std::shared_ptr<RequestHandler>> handlers_;
    };

    // Start service on read port with configured handlers
    handler_server s(io_service, port, path_to_handler);

    std::cout << "Server running on port " << port << std::endl;
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}