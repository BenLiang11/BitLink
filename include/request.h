#ifndef REQUEST_H
#define REQUEST_H

#include <string>
#include <map>

/**
 * @brief Represents an HTTP request.
 * 
 * This class encapsulates an HTTP request with its method, URI, HTTP version,
 * headers, and body.
 */
class Request {
public:
    /**
     * @brief Construct a new Request object from raw HTTP request data.
     * 
     * @param raw_request The raw HTTP request as a string.
     */
    explicit Request(const std::string& raw_request);

    /**
     * @brief Get the HTTP method (GET, POST, etc.).
     * 
     * @return const std::string& The HTTP method.
     */
    const std::string& method() const { return method_; }

    /**
     * @brief Get the request URI.
     * 
     * @return const std::string& The URI.
     */
    const std::string& uri() const { return uri_; }

    /**
     * @brief Get the HTTP version.
     * 
     * @return const std::string& The HTTP version (e.g., "HTTP/1.1").
     */
    const std::string& version() const { return version_; }

    /**
     * @brief Get a specific header value.
     * 
     * @param name The header name.
     * @return const std::string& The header value, or empty string if not found.
     */
    const std::string& get_header(const std::string& name) const;

    /**
     * @brief Get all headers.
     * 
     * @return const std::map<std::string, std::string>& All headers.
     */
    const std::map<std::string, std::string>& headers() const { return headers_; }

    /**
     * @brief Get the request body.
     * 
     * @return const std::string& The request body.
     */
    const std::string& body() const { return body_; }

    /**
     * @brief Get the raw request.
     * 
     * @return const std::string& The raw HTTP request.
     */
    const std::string& raw_request() const { return raw_request_; }

    /**
     * @brief Get the file path from the request URI
     * 
     * @param api_path The API path.
     * @return std::string The file path.
     */
    std::string get_file_path(const std::string& api_path) const;

private:
    std::string raw_request_;
    std::string method_;
    std::string uri_;
    std::string version_;
    std::map<std::string, std::string> headers_;
    std::string body_;
    static const std::string empty_header_;

    /**
     * @brief Parse the raw request into its components.
     */
    void parse_request();
};

#endif // REQUEST_H 