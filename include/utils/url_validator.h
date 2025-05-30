#ifndef URL_VALIDATOR_H
#define URL_VALIDATOR_H

#include <string>
#include <regex>
#include <set>

/**
 * @brief URL validation and normalization utilities
 * 
 * Provides comprehensive URL validation and normalization
 * to ensure only valid URLs are shortened.
 */
class URLValidator {
public:
    /**
     * @brief URL validation result
     */
    struct ValidationResult {
        bool is_valid;
        std::string normalized_url;
        std::string error_message;
        
        ValidationResult(bool valid = false, const std::string& url = "", 
                        const std::string& error = "")
            : is_valid(valid), normalized_url(url), error_message(error) {}
    };
    
    /**
     * @brief Validate and normalize a URL
     * @param url URL string to validate
     * @return ValidationResult with normalized URL or error
     */
    static ValidationResult validate_and_normalize(const std::string& url);
    
    /**
     * @brief Check if URL scheme is allowed
     * @param url URL to check
     * @return true if scheme is allowed (http/https)
     */
    static bool is_allowed_scheme(const std::string& url);
    
    /**
     * @brief Check if URL points to a blocked domain
     * @param url URL to check
     * @return true if domain is blocked
     */
    static bool is_blocked_domain(const std::string& url);
    
    /**
     * @brief Extract domain from URL
     * @param url URL string
     * @return Domain name or empty string if invalid
     */
    static std::string extract_domain(const std::string& url);

private:
    /**
     * @brief Regular expression pattern for URL validation
     */
    static const std::regex URL_PATTERN;
    
    /**
     * @brief Set of blocked domains
     */
    static const std::set<std::string> BLOCKED_DOMAINS;
    
    /**
     * @brief Normalize URL by adding scheme, removing fragments, etc.
     * @param url URL string to normalize
     * @return Normalized URL
     */
    static std::string normalize_url(const std::string& url);
    
    /**
     * @brief Add default scheme (https) if none present
     * @param url URL string
     * @return URL with scheme
     */
    static std::string add_default_scheme(const std::string& url);
    
    /**
     * @brief Check if URL has a valid format
     * @param url URL to check
     * @return true if format is valid
     */
    static bool is_valid_format(const std::string& url);
    
    /**
     * @brief Convert domain to lowercase
     * @param domain Domain string
     * @return Lowercase domain
     */
    static std::string normalize_domain(const std::string& domain);
    
    /**
     * @brief Check if domain is an IP address
     * @param domain Domain string to check
     * @return true if it's an IP address
     */
    static bool is_ip_address(const std::string& domain);
    
    /**
     * @brief Validate IP address format
     * @param ip IP address string
     * @return true if valid IPv4 or IPv6
     */
    static bool is_valid_ip(const std::string& ip);
    
    /**
     * @brief Remove default ports from URL
     * @param url URL string
     * @return URL without default ports
     */
    static std::string remove_default_port(const std::string& url);
    
    /**
     * @brief URL decode a string
     * @param encoded URL-encoded string
     * @return Decoded string
     */
    static std::string url_decode(const std::string& encoded);
    
    /**
     * @brief Check if character is a valid URL character
     * @param c Character to check
     * @return true if valid
     */
    static bool is_valid_url_char(char c);
};

#endif // URL_VALIDATOR_H 