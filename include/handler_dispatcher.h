#ifndef HANDLER_DISPATCHER_H
#define HANDLER_DISPATCHER_H

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include "handlers/base_handler.h"
#include "handler_registry.h"
#include "request.h"
#include "common_exceptions.h"

/**
 * @brief Structure to store information about handler registration.
 */
struct HandlerRegistration {
    std::string location;         // The URL path prefix (e.g., "/static")
    std::string handler_name;     // The handler name (e.g., "StaticHandler")
    std::vector<std::string> args; // The handler constructor arguments
};

/**
 * @brief Dispatches HTTP requests to the appropriate handler based on the request URL path.
 * 
 * This class maintains a map of URL path prefixes to handler registrations and uses
 * longest prefix matching to determine which handler should process a given request.
 * It instantiates a new handler for each request, as specified by the Common API.
 */
class HandlerDispatcher {
public:
    /**
     * @brief Constructs a new HandlerDispatcher with the given handler registrations.
     * 
     * @param handler_registrations Map of URL paths to handler registrations
     * @throws common::DuplicateLocationException if duplicate locations are found
     */
    explicit HandlerDispatcher(const std::map<std::string, HandlerRegistration>& handler_registrations);

    /**
     * @brief Create an appropriate handler for the given request.
     * 
     * Finds the registration with the longest matching prefix for the request URL,
     * then instantiates a new handler instance for that request.
     * 
     * @param request The HTTP request to handle
     * @return A unique_ptr to a RequestHandler instance, or nullptr if no matching handler found
     */
    std::unique_ptr<RequestHandler> CreateHandlerForRequest(const Request& request) const;

private:
    /**
     * @brief Find the location with the longest prefix match for a given URI path.
     * 
     * @param uri The URI path to match against registered locations
     * @return The best matching location, or empty string if no match found
     */
    std::string FindLongestPrefixMatch(const std::string& uri) const;

    // Map of URL path prefixes to handler registrations
    std::map<std::string, HandlerRegistration> handler_registrations_;
};

#endif // HANDLER_DISPATCHER_H 