#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <set>
#include "handlers/base_handler.h"
#include "config_parser.h"

// Forward declarations
class NginxConfig;
class NginxConfigStatement;

// Represents a location block in the server configuration
struct LocationConfig {
    std::string path;
    std::string handler_type;
    std::string root_directory;  // For static file handlers
};

// Server configuration class that parses and stores server settings
class ServerConfig {
public:
    // Constructor with default port value
    ServerConfig() : port_(-1) {}

    // Parse the configuration from a config file
    bool ParseConfig(const NginxConfig& config);

    // Create handlers based on the configuration
    std::map<std::string, std::shared_ptr<RequestHandler>> CreateHandlers() const;

    // Get the server port
    int port() const { return port_; }

    // Get the location configurations
    const std::vector<LocationConfig>& locations() const { return locations_; }

private:
    // Helper method to parse port directive
    bool ParsePortDirective(const std::string& port_str);
    
    // Helper method to parse location directive
    bool ParseLocationDirective(const std::shared_ptr<NginxConfigStatement>& statement);
    
    // Add default location if none specified
    void AddDefaultLocationIfNeeded();
    
    // Validate configuration completeness and consistency
    bool ValidateConfiguration();

    int port_;
    std::vector<LocationConfig> locations_;
};

#endif // SERVER_CONFIG_H 