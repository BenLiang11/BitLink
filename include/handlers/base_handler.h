#ifndef BASE_HANDLER_H
#define BASE_HANDLER_H

#include "request.h"
#include "response.h"

/**
 * @brief Abstract base class for HTTP request handlers.
 * 
 * To add a new handler, subclass RequestHandler and implement HandleRequest.
 * This allows for modular, extensible request handling where different
 * handlers can be used for different request paths.
 */
class RequestHandler {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~RequestHandler() = default;
    
    /**
     * @brief Handle an HTTP request and populate the response.
     * 
     * @param request The HTTP request to handle.
     * @param response The HTTP response to populate.
     * @return bool true if handled successfully, false otherwise.
     */
    virtual bool HandleRequest(const Request& request, Response* response) = 0;
};

#endif // BASE_HANDLER_H
