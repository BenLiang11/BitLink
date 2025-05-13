#pragma once

#ifndef NOT_FOUND_HANDLER_H
#define NOT_FOUND_HANDLER_H

#include "handlers/base_handler.h"
#include "request.h"
#include "response.h"
#include <memory>
#include <string>
#include <vector>
class NotFoundHandler : public RequestHandler {
public:
    
    static std::unique_ptr<RequestHandler> Create(const std::vector<std::string>& args);
    NotFoundHandler() = default;
    std::unique_ptr<Response> handle_request(const Request& request) override {
        std::unique_ptr<Response>response = std::make_unique<Response>();
        response->set_status(Response::NOT_FOUND);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>404 Not Found</h1><p>The requested file could not be found.</p></body></html>");
        response->set_header("Connection", "close");
        return response;
    }
private:
    std::string location_;
};
#endif // NOT_FOUND_HANDLER_H