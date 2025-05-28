#ifndef BASE_HANDLER_H
#define BASE_HANDLER_H

#include <memory> // Required for std::unique_ptr
#include <boost/beast/http.hpp> // Required for boost::beast types
#include "request.h"
#include "response.h"

/**
 * @brief Abstract base class for all request handlers.
 * 
 * Each handler must implement handle_request, which takes a request and returns a response.
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
     * @brief Handle an HTTP request and return a response.
     * 
     * @param req The HTTP request object.
     * @return A unique_ptr to the response object.
     */
    virtual std::unique_ptr<Response> handle_request(const Request& req) = 0;

    
    virtual std::string name() const {
        return "UnnamedHandler";
    }
    
};

#endif // BASE_HANDLER_H
