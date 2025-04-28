#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>
#include <map>

/**
 * @brief Represents an HTTP response.
 * 
 * This class encapsulates an HTTP response with its status code, headers,
 * and body.
 */
class Response {
public:
    /**
     * @brief HTTP status codes.
     */
    enum StatusCode {
        OK = 200,
        NOT_FOUND = 404,
        INTERNAL_SERVER_ERROR = 500
    };

    /**
     * @brief Construct a new Response object.
     */
    Response() : status_(OK) {}

    /**
     * @brief Set the HTTP status code.
     * 
     * @param status The status code.
     */
    void set_status(StatusCode status) { status_ = status; }

    /**
     * @brief Get the HTTP status code.
     * 
     * @return StatusCode The status code.
     */
    StatusCode status() const { return status_; }

    /**
     * @brief Set a specific header value.
     * 
     * @param name The header name.
     * @param value The header value.
     */
    void set_header(const std::string& name, const std::string& value) {
        headers_[name] = value;
    }

    /**
     * @brief Get all headers.
     * 
     * @return const std::map<std::string, std::string>& All headers.
     */
    const std::map<std::string, std::string>& headers() const { return headers_; }

    /**
     * @brief Set the response body.
     * 
     * @param body The response body.
     */
    void set_body(const std::string& body) { 
        body_ = body;
        set_header("Content-Length", std::to_string(body_.length()));
    }

    /**
     * @brief Get the response body.
     * 
     * @return const std::string& The response body.
     */
    const std::string& body() const { return body_; }

    /**
     * @brief Convert the response to a string.
     * 
     * @return std::string The full HTTP response as a string.
     */
    std::string to_string() const;

private:
    StatusCode status_;
    std::map<std::string, std::string> headers_;
    std::string body_;

    /**
     * @brief Get the string representation of a status code.
     * 
     * @param status The status code.
     * @return std::string The string representation.
     */
    static std::string status_to_string(StatusCode status);
};

#endif // RESPONSE_H 