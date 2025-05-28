#ifndef HEALTH_HANDLER_H
#define HEALTH_HANDLER_H

#include "handlers/base_handler.h" 
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include "request.h"
#include "response.h"

class HealthHandler : public RequestHandler {
public:
    HealthHandler();

    std::unique_ptr<Response> handle_request(const Request& req) override;

    static std::unique_ptr<RequestHandler> Create(const std::vector<std::string>& args);

private:
    std::chrono::steady_clock::time_point start_time_;
};

#endif // HEALTH_HANDLER_H
