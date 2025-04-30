#include "handler_dispatcher.h"
#include <algorithm>

HandlerDispatcher::HandlerDispatcher(const std::map<std::string,
 std::shared_ptr<RequestHandler>>& path_to_handler)
    : path_to_handler_(path_to_handler) {
}

std::shared_ptr<RequestHandler> HandlerDispatcher::GetHandler(const std::string& request_path) const {
    // Check for exact match first
    auto it = path_to_handler_.find(request_path);
    if (it != path_to_handler_.end()) {
        return it->second;
    }
    
    // No exact match, find the longest matching prefix
    // For example, if request_path is /static/img/file.jpg and we have a handler for /static/
    std::string best_match = "";
    std::shared_ptr<RequestHandler> best_handler = nullptr;
    
    for (const auto& entry : path_to_handler_) {
        const std::string& path = entry.first;
        
        // Check if this path is a prefix of the request path
        if (request_path.find(path) == 0) {
            // Make sure it's a path component match
            // check could be further improved
            // Either the path ends with a slash or it's followed by a slash in the request path
            if (path.back() == '/' || 
                request_path.size() == path.size() || 
                request_path[path.size()] == '/') {
                
                // If this path is longer than our current best match, update it
                if (path.length() > best_match.length()) {
                    best_match = path;
                    best_handler = entry.second;
                }
            }
        }
    }
    
    return best_handler;
}
