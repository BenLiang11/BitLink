#include "handler_dispatcher.h"
#include "handler_registry.h"
#include "common_exceptions.h"
#include <algorithm>
#include <iostream>
#include <set>

HandlerDispatcher::HandlerDispatcher(const std::map<std::string, HandlerRegistration>& handler_registrations) {
    // Check for duplicate locations by storing and checking each location path
    std::set<std::string> location_paths;
    for (const auto& entry : handler_registrations) {
        const auto& reg = entry.second;
        const std::string& location_path = reg.location;
        
        // Check for duplicate location
        if (location_paths.find(location_path) != location_paths.end()) {
            // Found a duplicate location path, fail at startup as required
            throw common::DuplicateLocationException(location_path);
        }
        location_paths.insert(location_path);
        
        // Also verify that location doesn't have a trailing slash (except for root "/")
        if (location_path.length() > 1 && location_path.back() == '/') {
            throw common::TrailingSlashException(location_path);
        }
    }
    
    // Store the handler registrations
    handler_registrations_ = handler_registrations;
    std::cout << "Registered " << handler_registrations_.size() << " handlers" << std::endl;
}

std::unique_ptr<RequestHandler> HandlerDispatcher::CreateHandlerForRequest(const Request& request) const {
    // Get the URI from the request
    std::string uri = request.uri();
    
    // Find the best matching location
    std::string best_match = FindLongestPrefixMatch(uri);
    if (best_match.empty()) {
        best_match = "/";
    }
    
    // Get the handler registration
    const auto it = handler_registrations_.find(best_match);
    if (it == handler_registrations_.end()) {
        return nullptr;
    }
    const HandlerRegistration& reg = it->second;
    
    std::cout << "Creating handler " << reg.handler_name << " for URI: " << uri 
              << " (matched location: " << best_match << ")" << std::endl;
    
    // Create the handler using the HandlerRegistry
    try {
        std::unique_ptr<RequestHandler> handler = 
            HandlerRegistry::CreateHandler(reg.handler_name, reg.args);
        
        if (!handler) {
            std::cerr << "Failed to create handler for " << reg.handler_name << std::endl;
            return nullptr;
        }
        
        return handler;
    } catch (const std::exception& e) {
        std::cerr << "Error creating handler: " << e.what() << std::endl;
        return nullptr;
    }
}

std::string HandlerDispatcher::FindLongestPrefixMatch(const std::string& uri) const {
    // Find the location with the longest matching prefix
    std::string best_match = "";
    
    for (const auto& entry : handler_registrations_) {
        const std::string& location = entry.first;
        
        // Check if location is a prefix of the URI
        if (uri.find(location) == 0) {
            // Make sure it's a path component match
            // Either the location is "/", or the URI is exactly the location,
            // or the next character after the location in the URI is a "/"
            if (location == "/" || 
                uri.length() == location.length() || 
                uri[location.length()] == '/') {
                
                // If this location is longer than our current best match, update it
                if (location.length() > best_match.length()) {
                    best_match = location;
                }
            }
        }
    }
    
    return best_match;
}
