#ifndef HEALTH_HANDLER_H
#define HEALTH_HANDLER_H

#include "handlers/base_handler.h"
#include <chrono>

class HealthHandler : public RequestHandler {
public:
    HealthHandler() : start_time_(std::chrono::steady_clock::now()) {}
    
    std::unique_ptr<Response> handle_request(const Request& req) override {
        auto response = std::make_unique<Response>();
        
        if (req.method() != "GET") {
            response->set_status(Response::BAD_REQUEST);
            response->set_header("Content-Type", "application/json");
            response->set_body("{\"error\": \"Method not allowed\", \"allowed_methods\": [\"GET\"]}");
            return response;
        }
        
        // Simple health check JSON
        std::string health_json = "{\n"
            "  \"status\": \"healthy\",\n"
            "  \"server\": \"i-am-steve\",\n"
            "  \"version\": \"1.0.0\"\n"
            "}";
        
        response->set_status(Response::OK);
        response->set_header("Content-Type", "application/json");
        response->set_body(health_json);
        return response;
    }
    
    static std::unique_ptr<RequestHandler> Create(const std::vector<std::string>& args) {
        if (!args.empty()) {
            throw std::invalid_argument("HealthHandler takes no arguments");
        }
        return std::make_unique<HealthHandler>();
    }

private:
    std::chrono::steady_clock::time_point start_time_;
};

#endif