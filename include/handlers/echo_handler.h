#ifndef ECHO_HANDLER_H
#define ECHO_HANDLER_H

#include "handlers/base_handler.h"
#include <memory>
#include <vector>
#include <string>

/**
 * @brief Handler that echoes the HTTP request body in the response body.
 *
 * This handler implements the echo functionality by taking the request body
 * and setting it as the response body. It does not take any arguments
 * from the config file.
 */
class EchoHandler : public RequestHandler {
public:
    /**
     * @brief Default constructor for EchoHandler.
     * This handler does not require any arguments from the config.
     */
    EchoHandler() = default;

    /**
     * @brief Handle an HTTP request by echoing its body back in the response.
     *
     * @param req The HTTP request object.
     * @return A unique_ptr to the response object containing the echoed body.
     */
    std::unique_ptr<Response> handle_request(const Request& req) override;

    /**
     * @brief Static factory function for creating EchoHandler instances.
     *        Registered with HandlerRegistry.
     *
     * @param args A vector of string arguments (expected to be empty for EchoHandler).
     * @return A unique_ptr to a new EchoHandler instance.
     * @throws std::invalid_argument if any arguments are provided.
     */
    static std::unique_ptr<RequestHandler> Create(const std::vector<std::string>& args);
};

#endif // ECHO_HANDLER_H 