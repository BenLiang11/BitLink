#include "handlers/echo_handler.h"
#include "handler_registry.h" // For HandlerRegistry
#include <stdexcept> // For std::invalid_argument in Create factory

// Implementation of the handle_request method
std::unique_ptr<Response> EchoHandler::handle_request(const Request& req) {
    auto res = std::make_unique<Response>();
    res->set_status(Response::OK); 
    res->set_header("Content-Type", "text/plain");
    res->set_body(req.body()); // Assuming req.get_body() returns the body content as std::string or compatible


    // The Response object should prepare itself if necessary (e.g., Content-Length)
    // If not, you might need to set it manually here.
    // For example: res->set_header("Content-Length", std::to_string(res->get_body().length()));

    return res;
}

// Implementation of the static factory function
std::unique_ptr<RequestHandler> EchoHandler::Create(const std::vector<std::string>& args) {
    if (!args.empty()) {
        // EchoHandler expects no arguments.
        // Consider logging this event.
        throw std::invalid_argument("EchoHandler expects no arguments.");
    }
    return std::make_unique<EchoHandler>();
}

// Static registration block
namespace {
    // The variable name `echo_handler_registered` is to avoid name collisions with other handlers.
    const bool echo_handler_registered = HandlerRegistry::RegisterHandler("EchoHandler", EchoHandler::Create);
} 