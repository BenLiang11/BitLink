#pragma once

#include "handlers/base_handler.h"
#include <chrono>
#include <thread>
#include <vector>
#include <string>

class SleepHandler : public RequestHandler {
public:
    SleepHandler() = default;

    std::unique_ptr<Response> handle_request(const Request& request) override {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        auto response = std::make_unique<Response>();
        response->set_status(Response::OK);
        response->set_header("Content-Type", "text/plain");
        response->set_body("Slept for 3 seconds\n");
        return response;
    }

    std::string name() const override {
        return "SleepHandler";
    }

    static std::unique_ptr<RequestHandler> Create(const std::vector<std::string>& args) {
        return std::make_unique<SleepHandler>();
    }
};
