#include "handlers/health_handler.h"
#include "handler_registry.h"

// Register the handler
namespace {
    const bool health_handler_registered = HandlerRegistry::RegisterHandler("HealthHandler", HealthHandler::Create);
}