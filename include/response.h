#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>
#include <map>

// HTTP response with status code, headers, and body
class Response {
public:
    // HTTP Status codes
    enum StatusCode {
        OK = 200,
        CREATED = 201,
        NO_CONTENT = 204,
        FOUND = 302,
        BAD_REQUEST = 400,
        UNAUTHORIZED = 401,
        FORBIDDEN = 403,
        NOT_FOUND = 404,
        METHOD_NOT_ALLOWED = 405,
        INTERNAL_SERVER_ERROR = 500,
        NOT_IMPLEMENTED = 501,
        SERVICE_UNAVAILABLE = 503
    };

    // Default‑construct with 200 OK
    Response() : status_(OK) {}

    // Set/Get status
    void set_status(StatusCode status) { status_ = status; }
    void set_status_code(StatusCode status) { status_ = status; }
    StatusCode status() const { return status_; }
    StatusCode status_code() const { return status_; }

    // Set/Get header
    void set_header(const std::string& name, const std::string& value) {
        headers_[name] = value;
    }
    const std::map<std::string, std::string>& headers() const { return headers_; }

    // Set/Get body
    void set_body(const std::string& body) { 
        body_ = body;
        set_header("Content-Length", std::to_string(body_.length()));
    }
    const std::string& body() const { return body_; }

    // Render full HTTP response
    std::string to_string() const;

private:
    StatusCode status_;
    std::map<std::string, std::string> headers_;
    std::string body_;

    // Convert status code to text
    static std::string status_to_string(StatusCode status);
};

#endif // RESPONSE_H 