#include "handler_registry.h"
#include "handlers/health_handler.h"

HealthHandler::HealthHandler() : start_time_(std::chrono::steady_clock::now()) {}

std::unique_ptr<Response> HealthHandler::handle_request(const Request& request) {
    auto response = std::make_unique<Response>();

    // Only GET allowed
    if (request.method() != "GET") {
        response->set_status(Response::BAD_REQUEST);
        response->set_header("Content-Type", "application/json");
        response->set_body("{\"error\": \"Method not allowed\", \"allowed_methods\": [\"GET\"]}");
        return response;
    }

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

std::unique_ptr<RequestHandler> HealthHandler::Create(const std::vector<std::string>& args) {
    if (!args.empty()) {
        throw std::invalid_argument("HealthHandler takes no arguments");
    }
    return std::make_unique<HealthHandler>();
}