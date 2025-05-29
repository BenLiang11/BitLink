#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <set>
#include <variant>
#include "handlers/base_handler.h"
#include "handler_dispatcher.h"
#include "config_parser.h"
#include "common_exceptions.h"

// Forward declarations
class NginxConfig;
class NginxConfigStatement;

/**
 * @brief Represents a typed configuration value.
 * 
 * Allows storing configuration values as different types: integer, string, or boolean.
 */
using TypedValue = std::variant<int, std::string, bool>;

/**
 * @brief Represents a location block in the server configuration.
 */
struct LocationConfig {
    std::string path;                           // URL path (e.g., "/static")
    std::string handler_type;                   // Handler type name (e.g., "static")
    std::map<std::string, TypedValue> args;     // Typed arguments for the handler
};

/**
 * @brief Server configuration class that parses and stores server settings.
 */
class ServerConfig {
public:
    /**
     * @brief Constructor with default port value.
     */
    ServerConfig() : port_(-1) {}

    /**
     * @brief Parse the configuration from a config file.
     * 
     * @param config The parsed nginx configuration.
     * @return true if parsing was successful, false otherwise.
     * @throws common::DuplicateLocationException if duplicate locations are found
     * @throws common::TrailingSlashException if a location has a trailing slash
     */
    bool ParseConfig(const NginxConfig& config);

    /**
     * @brief Create handler registrations based on the configuration.
     * 
     * @return A map of location paths to handler registrations.
     */
    std::map<std::string, HandlerRegistration> CreateHandlerRegistrations() const;

    /**
     * @brief Get the server port.
     * 
     * @return The configured port number.
     */
    int port() const { return port_; }

    /**
     * @brief Get the location configurations.
     * 
     * @return A vector of location configurations.
     */
    const std::vector<LocationConfig>& locations() const { return locations_; }

private:
    /**
     * @brief Helper method to parse port directive.
     * 
     * @param port_str The port string to parse.
     * @return true if parsing was successful, false otherwise.
     */
    bool ParsePortDirective(const std::string& port_str);
    
    /**
     * @brief Helper method to parse location directive.
     * 
     * @param statement The nginx config statement containing the location directive.
     * @return true if parsing was successful, false otherwise.
     * @throws common::TrailingSlashException if the location has a trailing slash
     */
    bool ParseLocationDirective(const std::shared_ptr<NginxConfigStatement>& statement);
    
    /**
     * @brief Parse typed arguments from a child block.
     * 
     * @param child_block The child block containing handler arguments.
     * @param args Map to store parsed arguments.
     * @return true if parsing was successful, false otherwise.
     */
    bool ParseTypedArguments(const NginxConfig* child_block, std::map<std::string, TypedValue>& args);
    
    /**
     * @brief Add default location if none specified.
     */
    void AddDefaultNotFoundHandler();
    
    /**
     * @brief Validate configuration completeness and consistency.
     * 
     * @return true if the configuration is valid, false otherwise.
     * @throws common::DuplicateLocationException if duplicate locations are found
     */
    bool ValidateConfiguration();
    
    /**
     * @brief Convert typed arguments to string vector for HandlerRegistry.
     * 
     * @param location_path The location path.
     * @param typed_args The typed arguments.
     * @return A vector of string arguments.
     */
    std::vector<std::string> TypedArgsToStringVectorForStaticFileHandler(
        const std::string& location_path,
        const std::map<std::string, TypedValue>& typed_args) const;
    std::vector<std::string> TypedArgsToStringVectorForApiHandler(
        const std::string& location_path,
        const std::map<std::string, TypedValue>& typed_args) const;
    
    int port_;
    std::vector<LocationConfig> locations_;
    const std::string ECHO_HANDLER_NAME = "EchoHandler";
    const std::string STATIC_FILE_HANDLER_NAME = "StaticHandler";
    const std::string NOT_FOUND_HANDLER_NAME = "NotFoundHandler";
    const std::string API_HANDLER_NAME = "ApiHandler";
    const std::string HEALTH_HANDLER_NAME = "HealthHandler";
};

#endif // SERVER_CONFIG_H 