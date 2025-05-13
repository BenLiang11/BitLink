#include "handlers/not_found_handler.h"
#include "handler_registry.h"
#include <stdexcept>

std::unique_ptr<RequestHandler> NotFoundHandler::Create(const std::vector<std::string>& args) {
    if (!args.empty()) {
        // EchoHandler expects no arguments.
        // Consider logging this event.
        throw std::invalid_argument("NotFoundHandler expects no arguments.");
    }
    return std::make_unique<NotFoundHandler>();
}

// Static registration block
namespace {
    const bool not_found_handler_registered = HandlerRegistry::RegisterHandler("NotFoundHandler", NotFoundHandler::Create);
} 