#include "response.h"

std::string Response::to_string() const {
    std::string result = "HTTP/1.1 " + status_to_string(status_) + "\r\n";
    
    // Add Content-Length header if not already set and body is not empty
    std::map<std::string, std::string> headers = headers_;
    if (headers.find("Content-Length") == headers.end()) {
        headers["Content-Length"] = std::to_string(body_.size());
    }
    
    // Add all headers
    for (const auto& header : headers) {
        result += header.first + ": " + header.second + "\r\n";
    }
    
    // Add a blank line to separate headers from body
    result += "\r\n";
    
    // Add the body
    result += body_;
    
    return result;
}

std::string Response::status_to_string(StatusCode status) {
    switch (status) {
        case OK:                     return "200 OK";
        case CREATED:                return "201 Created";
        case NO_CONTENT:             return "204 No Content";
        case BAD_REQUEST:            return "400 Bad Request";
        case UNAUTHORIZED:           return "401 Unauthorized";
        case FORBIDDEN:              return "403 Forbidden";
        case NOT_FOUND:              return "404 Not Found";
        case INTERNAL_SERVER_ERROR:  return "500 Internal Server Error";
        case NOT_IMPLEMENTED:        return "501 Not Implemented";
        case SERVICE_UNAVAILABLE:    return "503 Service Unavailable";
        default:                     return "200 OK"; // Default to OK
    }
} 