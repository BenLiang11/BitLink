#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <string>
#include <map>
#include <memory>
#include <vector>
#include "handlers/base_handler.h"
#include "config_parser.h"

// Forward declarations
class NginxConfig;

/**
 * @brief Represents a location block in the server configuration
 */
struct LocationConfig {
    std::string path;
    std::string handler_type;
    std::string root_directory;  // For static file handlers
};

/**
 * @brief Server configuration class that parses and stores server settings
 */
class ServerConfig {
public:
    /**
     * @brief Construct a new Server Config object
     */
    ServerConfig() : port_(-1) {}

    /**
     * @brief Parse the configuration from a config file
     * 
     * @param config The parsed NginxConfig
     * @return true if parsing was successful
     */
    bool ParseConfig(const NginxConfig& config);

    /**
     * @brief Create handlers based on the configuration
     * 
     * @return std::map<std::string, std::shared_ptr<RequestHandler>> Map of paths to handlers
     */
    std::map<std::string, std::shared_ptr<RequestHandler>> CreateHandlers() const;

    /**
     * @brief Get the server port
     * 
     * @return int The port number
     */
    int port() const { return port_; }

    /**
     * @brief Get the location configurations
     * 
     * @return const std::vector<LocationConfig>& The location configurations
     */
    const std::vector<LocationConfig>& locations() const { return locations_; }

private:
    int port_;
    std::vector<LocationConfig> locations_;
};

#endif // SERVER_CONFIG_H 