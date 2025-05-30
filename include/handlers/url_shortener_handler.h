#ifndef URL_SHORTENER_HANDLER_H
#define URL_SHORTENER_HANDLER_H

#include "handlers/base_handler.h"
#include "services/url_shortening_service.h"
#include "services/file_upload_service.h"
#include "request.h"
#include "response.h"
#include <memory>
#include <string>
#include <map>

/**
 * @brief HTTP handler for URL shortening and file upload operations
 * 
 * Handles all HTTP endpoints for the QR-code short-link service:
 * - GET / - Main interface page
 * - POST /shorten - Shorten a URL
 * - POST /upload - Upload a file
 * - GET /r/<code> - Redirect to original URL or serve file
 * - GET /stats/<code> - Get click statistics
 * - GET /qr/<code> - Get QR code image
 */
class URLShortenerHandler : public RequestHandler {
public:
    /**
     * @brief Constructor
     * @param serving_path Base path this handler serves (e.g., "/")
     * @param upload_dir Directory for file uploads
     * @param db_path Path to SQLite database
     * @param base_url Base URL for shortened links
     */
    URLShortenerHandler(const std::string& serving_path,
                       const std::string& upload_dir,
                       const std::string& db_path,
                       const std::string& base_url);
    
    /**
     * @brief Handle HTTP request
     * @param req HTTP request object
     * @return HTTP response
     */
    std::unique_ptr<Response> handle_request(const Request& req) override;
    
    /**
     * @brief Factory method for handler creation
     * @param args Configuration arguments
     * @return Handler instance
     */
    static std::unique_ptr<RequestHandler> Create(const std::vector<std::string>& args);
    
    /**
     * @brief Get handler name
     * @return Handler name string
     */
    std::string name() const override { return "URLShortenerHandler"; }

private:
    std::string serving_path_;
    std::string upload_dir_;
    std::string db_path_;
    std::string base_url_;
    
    std::unique_ptr<URLShorteningService> url_service_;
    std::unique_ptr<FileUploadService> file_service_;
    
    // Route handlers
    std::unique_ptr<Response> handle_main_page(const Request& req);
    std::unique_ptr<Response> handle_shorten_url(const Request& req);
    std::unique_ptr<Response> handle_upload_file(const Request& req);
    std::unique_ptr<Response> handle_redirect(const Request& req);
    std::unique_ptr<Response> handle_stats(const Request& req);
    std::unique_ptr<Response> handle_qr_code(const Request& req);
    
    // Utility methods
    std::string extract_code_from_path(const std::string& path, const std::string& prefix);
    std::string get_client_ip(const Request& req);
    std::string get_user_agent(const Request& req);
    std::string get_referrer(const Request& req);
    
    // HTTP utility methods
    std::unique_ptr<Response> create_json_response(const std::string& json, 
                                                  Response::StatusCode status = Response::OK);
    std::unique_ptr<Response> create_error_response(const std::string& message,
                                                   Response::StatusCode status);
    std::unique_ptr<Response> create_redirect_response(const std::string& url);
    std::unique_ptr<Response> create_file_response(const std::string& file_path);
    
    // Request parsing
    std::map<std::string, std::string> parse_form_data(const std::string& body);
    std::map<std::string, std::string> parse_multipart_data(const std::string& body,
                                                           const std::string& boundary);
    
    // File upload processing
    struct FileUploadData {
        std::string filename;
        std::vector<uint8_t> data;
        std::string content_type;
    };
    
    std::optional<FileUploadData> extract_uploaded_file(const Request& req);
    
    // Initialize services
    bool initialize_services();
};

#endif // URL_SHORTENER_HANDLER_H 