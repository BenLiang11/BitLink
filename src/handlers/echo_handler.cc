#include "handlers/echo_handler.h"

bool EchoHandler::HandleRequest(const Request& request, Response* response) {
    // Set status code to OK
    response->set_status(Response::OK);
    
    // Set Content-Type header
    response->set_header("Content-Type", "text/plain");
    
    // Set the body to the raw request
    response->set_body(request.raw_request());
    
    // Set additional headers
    response->set_header("Connection", "close");
    
    return true;
} 