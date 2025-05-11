#include "server_config.h"
#include "handlers/base_handler.h"
#include "handlers/echo_handler.h"
#include "handlers/static_file_handler.h"
#include <iostream>
#include <sstream>
#include "common_exceptions.h"

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

// Create a map of locations to handler registrations
std::map<std::string, HandlerRegistration> ServerConfig::CreateHandlerRegistrations() const {
    std::map<std::string, HandlerRegistration> location_to_registration;
    std::cout << "Creating handler registrations for " << locations_.size() << " location(s)..." << std::endl;

    // Create handler registrations based on location configurations
    for (const auto& location : locations_) {
        try {
            std::string location_path = location.path;
            
            HandlerRegistration reg;
            reg.location = location_path;
            
            if (location.handler_type == "echo") {
                reg.handler_name = "EchoHandler";
                reg.args = {}; // EchoHandler takes no args
                std::cout << "Added EchoHandler registration for path: " << location_path << std::endl;
            } 
            else if (location.handler_type == "static") {
                reg.handler_name = "StaticHandler";
                
                // Convert typed arguments to string vector
                reg.args = TypedArgsToStringVector(location_path, location.args);
                
                std::cout << "Added StaticHandler registration for path: " << location_path << std::endl;
            }
            else {
                std::cerr << "Warning: Unknown handler type '" << location.handler_type 
                          << "' for path: " << location_path << ". Skipping." << std::endl;
                continue;
            }
            
            location_to_registration[location_path] = reg;
        }
        catch (const std::exception& e) {
            std::cerr << "Error creating handler registration for path " << location.path 
                      << ": " << e.what() << std::endl;
        }
    }
    
    std::cout << "Successfully created " << location_to_registration.size() << " handler registration(s)" << std::endl;
    return location_to_registration;
}

// Convert typed arguments to string vector for HandlerRegistry
std::vector<std::string> ServerConfig::TypedArgsToStringVector(
    const std::string& location_path, 
    const std::map<std::string, TypedValue>& typed_args) const {
    
    std::vector<std::string> args;
    
    // First argument is always the location path
    args.push_back(location_path);
    
    // For StaticHandler, we need the root directory
    if (typed_args.find("root") != typed_args.end()) {
        const auto& root_value = typed_args.at("root");
        if (std::holds_alternative<std::string>(root_value)) {
            args.push_back(std::get<std::string>(root_value));
        } else {
            std::cerr << "Warning: root value for path " << location_path << " is not a string" << std::endl;
            args.push_back("./"); // Default to current directory
        }
    } else {
        std::cerr << "Warning: no root directory specified for static handler at " 
                 << location_path << std::endl;
        args.push_back("./"); // Default to current directory
    }
    
    return args;
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
    
    // Check for trailing slash (except for root "/")
    if (location.path.length() > 1 && location.path.back() == '/') {
        std::cerr << "Error: Location paths cannot have trailing slashes: " << location.path << std::endl;
        throw common::TrailingSlashException(location.path);
    }
    
    std::cout << "Parsing location directive: " << location.path 
              << " with handler: " << location.handler_type << std::endl;
    
    // Parse handler-specific arguments
    if (statement->child_block_) {
        if (!ParseTypedArguments(statement->child_block_.get(), location.args)) {
            return false;
        }
    }
    
    // Validate handler-specific requirements
    if (location.handler_type == "static") {
        // Static handlers must specify a root directory
        if (location.args.find("root") == location.args.end()) {
            std::cerr << "Static handler for path " << location.path 
                     << " must specify a 'root' directive" << std::endl;
            return false;
        }
    }
    
    locations_.push_back(location);
    return true;
}

// Parse typed arguments from a child block
bool ServerConfig::ParseTypedArguments(const NginxConfig* child_block, std::map<std::string, TypedValue>& args) {
    if (!child_block) {
        return true; // No arguments to parse
    }
    
    for (const auto& statement : child_block->statements_) {
        if (statement->tokens_.size() < 2) {
            std::cerr << "Warning: Ignoring argument with insufficient tokens" << std::endl;
            continue;
        }
        
        const std::string& arg_name = statement->tokens_[0];
        const std::string& arg_value_str = statement->tokens_[1];
        
        // Try to parse as different types
        if (arg_name == "root" || arg_name == "path" || arg_name == "file") {
            // These are always treated as strings
            args[arg_name] = arg_value_str;
        }
        else {
            // Try to parse as integer
            try {
                int int_value = std::stoi(arg_value_str);
                args[arg_name] = int_value;
                continue;
            } catch (const std::exception&) {
                // Not an integer
            }
            
            // Try to parse as boolean
            if (arg_value_str == "true" || arg_value_str == "yes" || arg_value_str == "1") {
                args[arg_name] = true;
            }
            else if (arg_value_str == "false" || arg_value_str == "no" || arg_value_str == "0") {
                args[arg_name] = false;
            }
            else {
                // Default to string
                args[arg_name] = arg_value_str;
            }
        }
    }
    
    return true;
}

// Adds a default echo handler for root path if no locations are specified
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
        std::string path = location.path;
        
        if (unique_paths.find(path) != unique_paths.end()) {
            std::cerr << "Error: Duplicate handler for path '" << path << "'. Failing at startup." << std::endl;
            throw common::DuplicateLocationException(path);
        }
        unique_paths.insert(path);
    }
    
    std::cout << "Configuration validated successfully" << std::endl;
    return true;
}