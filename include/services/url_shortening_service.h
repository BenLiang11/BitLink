#ifndef URL_SHORTENING_SERVICE_H
#define URL_SHORTENING_SERVICE_H

#include "database/database_interface.h"
#include "models/link_data.h"
#include "utils/url_validator.h"
#include "utils/slug_generator.h"
#include "utils/qr_generator.h"
#include <memory>
#include <string>

using namespace std;

/**
 * @brief Core service for URL shortening operations
 * 
 * Provides high-level interface for creating, resolving,
 * and managing shortened URLs with analytics.
 */
class URLShorteningService {
public:
    /**
     * @brief Result of URL shortening operation
     */
    struct ShortenResult {
        bool success;
        string code;
        string short_url;
        string qr_code_data_url;
        string error_message;
        
        ShortenResult() : success(false) {}
        ShortenResult(const string& c, const string& url, const string& qr)
            : success(true), code(c), short_url(url), qr_code_data_url(qr) {}
    };
    
    /**
     * @brief Constructor
     * @param db Database interface (must be initialized)
     * @param base_url Base URL for shortened links (e.g., "https://short.ly")
     */
    URLShorteningService(unique_ptr<DatabaseInterface> db, 
                        const string& base_url);
    
    /**
     * @brief Shorten a URL
     * @param url Original URL to shorten
     * @return ShortenResult with code and QR code or error
     */
    ShortenResult shorten_url(const string& url);
    
    /**
     * @brief Resolve a shortened code to original URL
     * @param code Shortened code
     * @return Original URL if found, empty string otherwise
     */
    optional<string> resolve_code(const string& code);
    
    /**
     * @brief Record an access/click event
     * @param code Shortened code
     * @param ip_address Client IP address
     * @param user_agent User agent string
     * @param referrer Referrer URL (optional)
     * @return true if recorded successfully
     */
    bool record_access(const string& code, 
                      const string& ip_address,
                      const string& user_agent,
                      const string& referrer = "");
    
    /**
     * @brief Get click statistics for a code
     * @param code Shortened code
     * @return ClickStats with aggregated data
     */
    ClickStats get_statistics(const string& code);
    
    /**
     * @brief Delete a shortened link
     * @param code Shortened code
     * @return true if deleted successfully
     */
    bool delete_link(const string& code);

private:
    unique_ptr<DatabaseInterface> db_;
    string base_url_;
    
    string truncate_ip_for_privacy(const string& ip_address);
    string build_short_url(const string& code);
};

#endif // URL_SHORTENING_SERVICE_H 