#include "server_config.h"
#include "handlers/base_handler.h"
#include "handlers/echo_handler.h"
#include "handlers/static_file_handler.h"
#include <iostream>

//Parse provided NginxConfig object and update server configuration data
bool ServerConfig::ParseConfig(const NginxConfig& config) {
    std::cout << "Parsing server configuration..." << std::endl;
    
    // Iterate through all configuration statements
    for (const auto& statement : config.statements_) {
        // Skip statements with insufficient tokens
        if (statement->tokens_.size() < 2) {
            continue;
        }

        const std::string& directive = statement->tokens_[0];
        
        // Handle port configuration
        if (directive == "listen" && statement->tokens_.size() >= 2) {
            if (!ParsePortDirective(statement->tokens_[1])) {
                return false;
            }
        }
        // Handle location blocks
        else if (directive == "location" && statement->tokens_.size() >= 3) {
            if (!ParseLocationDirective(statement)) {
                return false;
            }
        }
    }
    
    // Add default echo handler if no locations are specified
    AddDefaultLocationIfNeeded();

    // Validate the completed configuration
    return ValidateConfiguration();
}

// Returns a map of URL paths to RequestHandlers 
std::map<std::string, std::shared_ptr<RequestHandler>> ServerConfig::CreateHandlers() const {
    std::map<std::string, std::shared_ptr<RequestHandler>> path_to_handler;
    std::cout << "Creating request handlers for " << locations_.size() << " location(s)..." << std::endl;

    // Create handlers based on location configurations
    for (const auto& location : locations_) {
        try {
            if (location.handler_type == "echo") {
                path_to_handler[location.path] = std::make_shared<EchoHandler>();
                std::cout << "Added echo handler for path: " << location.path << std::endl;
            } 
            else if (location.handler_type == "static") {
                path_to_handler[location.path] = std::make_shared<StaticFileHandler>(
                    location.root_directory, location.path);
                std::cout << "Added static file handler for path: " << location.path 
                          << " with root: " << location.root_directory << std::endl;
            }
            else {
                std::cerr << "Warning: Unknown handler type '" << location.handler_type 
                          << "' for path: " << location.path << ". Skipping." << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error creating handler for path " << location.path 
                      << ": " << e.what() << std::endl;
        }
    }
    
    std::cout << "Successfully created " << path_to_handler.size() << " handler(s)" << std::endl;
    return path_to_handler;
}

// Parses and validates the port string, returning true if successful
bool ServerConfig::ParsePortDirective(const std::string& port_str) {
    try {
        int port = std::stoi(port_str);
        
        if (port <= 0 || port > 65535) {
            std::cerr << "Port number out of range (1-65535): " << port << std::endl;
            return false;
        }
        
        port_ = port;
        std::cout << "Server port set to: " << port_ << std::endl;
        return true;
    } 
    catch (const std::exception& e) {
        std::cerr << "Invalid port number: " << port_str << " - " << e.what() << std::endl;
        return false;
    }
}

// Parses a location directive with its child blocks, returning true if successful
bool ServerConfig::ParseLocationDirective(const std::shared_ptr<NginxConfigStatement>& statement) {
    LocationConfig location;
    location.path = statement->tokens_[1];
    location.handler_type = statement->tokens_[2];
    
    std::cout << "Parsing location directive: " << location.path 
              << " with handler: " << location.handler_type << std::endl;
              
    // Static handlers must specify a root directory
    if (location.handler_type == "static") {
        // Ensure child block exists
        if (!statement->child_block_ || statement->child_block_->statements_.empty()) {
            std::cerr << "Static handler for path " << location.path 
                      << " missing configuration block" << std::endl;
            return false;
        }
        
        auto& root_statement = statement->child_block_->statements_[0];
        if (root_statement->tokens_.size() == 2 && root_statement->tokens_[0] == "root") {
            location.root_directory = root_statement->tokens_[1];
            std::cout << "Static handler for path " << location.path 
                      << " has root directory " << location.root_directory << std::endl;
        }
        else {
            std::cerr << "Static handler for path " << location.path 
                      << " must specify a 'root' directive" << std::endl;
            return false;
        }
    }
    
    locations_.push_back(location);
    return true;
}


//Adds a default echo handler for root path if no locations are specified
void ServerConfig::AddDefaultLocationIfNeeded() {
    if (locations_.empty()) {
        std::cout << "No locations specified, adding default echo handler for path '/'" << std::endl;
        LocationConfig location;
        location.path = "/";
        location.handler_type = "echo";
        locations_.push_back(location);
    }
}

// Validate configuration
bool ServerConfig::ValidateConfiguration() {
    // Validate port configuration
    if (port_ == -1) {
        std::cerr << "Error: No valid 'listen' directive found in config" << std::endl;
        return false;
    }
    
    // Check for duplicate paths
    std::set<std::string> unique_paths;
    for (const auto& location : locations_) {
        if (unique_paths.find(location.path) != unique_paths.end()) {
            std::cerr << "Warning: Duplicate handler for path '" << location.path 
                      << "'. Only the last one will be used." << std::endl;
        }
        unique_paths.insert(location.path);
    }
    
    std::cout << "Configuration validated successfully" << std::endl;
    return true;
}