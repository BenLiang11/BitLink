#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <cstdint>

/**
 * @brief HTTP request parsing utilities
 * 
 * Provides utilities for parsing HTTP request bodies,
 * including form data and multipart uploads.
 */
class HTTPParser {
public:
    /**
     * @brief Parse URL-encoded form data
     * @param body Request body containing form data
     * @return Map of field names to values
     */
    static std::map<std::string, std::string> parse_form_data(const std::string& body);
    
    /**
     * @brief Parsed multipart file data
     */
    struct MultipartFile {
        std::string filename;
        std::string content_type;
        std::vector<uint8_t> data;
        
        MultipartFile() = default;
        MultipartFile(const std::string& fname, const std::string& ctype, 
                     const std::vector<uint8_t>& fdata)
            : filename(fname), content_type(ctype), data(fdata) {}
    };
    
    /**
     * @brief Parse multipart/form-data request
     * @param body Request body
     * @param boundary Multipart boundary string
     * @param files Output map for file data
     * @return Map of field names to values
     */
    static std::map<std::string, std::string> parse_multipart_form(
        const std::string& body,
        const std::string& boundary,
        std::map<std::string, MultipartFile>& files);
    
    /**
     * @brief Extract boundary from Content-Type header
     * @param content_type Content-Type header value
     * @return Boundary string or empty if not found
     */
    static std::string extract_boundary(const std::string& content_type);
    
    /**
     * @brief URL decode a string
     * @param encoded URL-encoded string
     * @return Decoded string
     */
    static std::string url_decode(const std::string& encoded);
    
    /**
     * @brief URL encode a string
     * @param decoded Plain string
     * @return URL-encoded string
     */
    static std::string url_encode(const std::string& decoded);

private:
    /**
     * @brief Convert hex character to integer
     * @param c Hex character (0-9, A-F, a-f)
     * @return Integer value or -1 if invalid
     */
    static int hex_to_int(char c);
    
    /**
     * @brief Remove leading and trailing whitespace
     * @param str String to trim
     * @return Trimmed string
     */
    static std::string trim(const std::string& str);
    
    /**
     * @brief Parse Content-Disposition header
     * @param header Content-Disposition header value
     * @param name Output parameter for field name
     * @param filename Output parameter for filename (if present)
     * @return true if parsed successfully
     */
    static bool parse_content_disposition(const std::string& header, 
                                        std::string& name, 
                                        std::string& filename);
    
    /**
     * @brief Extract quoted string value from header
     * @param value Header value potentially containing quoted string
     * @return Unquoted string
     */
    static std::string extract_quoted_value(const std::string& value);
    
    // Constants for parsing
    static const size_t MAX_FIELD_SIZE = 1024 * 1024;  // 1MB max field size
    static const size_t MAX_FILE_SIZE = 25 * 1024 * 1024;  // 25MB max file size
};

#endif // HTTP_PARSER_H 