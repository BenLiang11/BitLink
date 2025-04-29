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
#include "logger.h"
#include <boost/log/trivial.hpp>

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

    // Extract port number from config
    int port = -1;
    std::map<std::string, std::shared_ptr<RequestHandler>> path_to_handler;
    std::string static_file_root;

    for (const auto& statement : config.statements_) {
      if (statement->tokens_.size() >= 2 && statement->tokens_[0] == "listen") {
        port = std::stoi(statement->tokens_[1]);
      }

      if (statement->tokens_.size() >= 2 && statement->tokens_[0] == "root") {
        static_file_root = statement->tokens_[1];
      }

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

    if (port == -1) {
      std::cerr << "No valid 'listen' directive found in config\n";
      return 1;
    }

    if (path_to_handler.empty()) {
      path_to_handler["/"] = std::make_shared<EchoHandler>();
      std::cout << "No handlers configured. Added default echo handler for path: /" << std::endl;
    }

    boost::asio::io_context io_context;

    class handler_server : public server {
    public:
      handler_server(boost::asio::io_context& io_context,
                     short port,
                     const std::map<std::string, std::shared_ptr<RequestHandler>>& handlers)
          : server(io_context, port), io_context_(io_context), handlers_(handlers) {}

      void handle_accept(session* new_session, const boost::system::error_code& error) override {
        if (!error) {
          if (!handlers_.empty()) {
            new_session->set_handler(handlers_.begin()->second);
          }
          new_session->start();
          new_session = new session(io_context_);
          acceptor_.async_accept(new_session->socket(),
            boost::bind(&handler_server::handle_accept, this, new_session,
              boost::asio::placeholders::error));
        } else {
          BOOST_LOG_TRIVIAL(error) << "Accept failed: " << error.message();
          delete new_session;
        }
      }

    private:
      boost::asio::io_context& io_context_;
      std::map<std::string, std::shared_ptr<RequestHandler>> handlers_;
    };

    handler_server s(io_context, port, path_to_handler);
    BOOST_LOG_TRIVIAL(info) << "Server running on port " << port;
    io_context.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  BOOST_LOG_TRIVIAL(info) << "Server shutting down...";
  
  return 0;
}
