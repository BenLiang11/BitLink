#include "server_config.h"
#include "handlers/base_handler.h"
#include "handlers/echo_handler.h"
#include "handlers/static_file_handler.h"
#include <iostream>

bool ServerConfig::ParseConfig(const NginxConfig& config) {
    for (const auto& statement : config.statements_) {
        if (statement->tokens_.size() < 2) {
            continue;
        }

        const std::string& directive = statement->tokens_[0];
        
        // Handle port configuration
        if (directive == "listen" && statement->tokens_.size() >= 2) {
            try {
                port_ = std::stoi(statement->tokens_[1]);
            } catch (const std::exception& e) {
                std::cerr << "Invalid port number: " << statement->tokens_[1] << std::endl;
                return false;
            }
        }
        
        // Handle location blocks
        else if (directive == "location" && statement->tokens_.size() >= 3) {
            LocationConfig location;
            location.path = statement->tokens_[1];
            location.handler_type = statement->tokens_[2];
            
            // Static handlers must specify a root directory
            if (location.handler_type == "static") {
                if (statement->tokens_.size() >= 5 && statement->tokens_[3] == "root") {
                    location.root_directory = statement->tokens_[4];
                } else {
                    std::cerr << "Static handler for path " << location.path 
                              << " must specify a root directory" << std::endl;
                    return false;
                }
            }
            locations_.push_back(location);
        }
    }
    // If there are no locations, add default echo location
    if (locations_.empty()) {
        LocationConfig location;
        location.path = "/";
        location.handler_type = "echo";
        locations_.push_back(location);
    }

    // Validate configuration
    if (port_ == -1) {
        std::cerr << "No valid 'listen' directive found in config" << std::endl;
        return false;
    }

    return true;
}

std::map<std::string, std::shared_ptr<RequestHandler>> ServerConfig::CreateHandlers() const {
    std::map<std::string, std::shared_ptr<RequestHandler>> path_to_handler;

    // Create handlers based on location configurations
    for (const auto& location : locations_) {
        if (location.handler_type == "echo") {
            path_to_handler[location.path] = std::make_shared<EchoHandler>();
            std::cout << "Added echo handler for path: " << location.path << std::endl;
        } else if (location.handler_type == "static") {
            path_to_handler[location.path] = std::make_shared<StaticFileHandler>(location.root_directory);
            std::cout << "Added static file handler for path: " << location.path 
                      << " with root: " << location.root_directory << std::endl;
        }
    }
    
    return path_to_handler;
} 