#ifndef ECHO_HANDLER_H
#define ECHO_HANDLER_H

#include "handlers/base_handler.h"

/**
 * @brief Handler that echoes the HTTP request in the response body.
 * 
 * This handler implements the echo functionality by setting the
 * response body to the raw request data.
 */
class EchoHandler : public RequestHandler {
public:
    /**
     * @brief Handle an HTTP request by echoing it back in the response.
     * 
     * @param request The HTTP request to handle.
     * @param response The HTTP response to populate.
     * @return bool true if handled successfully, false otherwise.
     */
    bool HandleRequest(const Request& request, Response* response) override;
};

#endif // ECHO_HANDLER_H 