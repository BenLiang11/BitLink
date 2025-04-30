#ifndef HANDLER_DISPATCHER_H
#define HANDLER_DISPATCHER_H

#include <map>
#include <string>
#include <memory>
#include "handlers/base_handler.h"

class HandlerDispatcher {
public:
    // Constructor that takes a map of paths to handlers
    HandlerDispatcher(const std::map<std::string, std::shared_ptr<RequestHandler>>& path_to_handler);
    
    // Get the appropriate handler for a given request path
    std::shared_ptr<RequestHandler> GetHandler(const std::string& request_path) const;

private:
    std::map<std::string, std::shared_ptr<RequestHandler>> path_to_handler_;
};

#endif // HANDLER_DISPATCHER_H 