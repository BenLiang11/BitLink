#ifndef FILE_VALIDATOR_H
#define FILE_VALIDATOR_H

#include <string>
#include <vector>
#include <set>
#include <cstdint>

/**
 * @brief File validation and security utilities
 * 
 * Provides comprehensive file validation including type checking,
 * size limits, and virus scanning integration.
 */
class FileValidator {
public:
    /**
     * @brief File validation result
     */
    struct ValidationResult {
        bool is_valid;
        std::string error_message;
        std::string detected_mime_type;
        
        ValidationResult(bool valid = true, const std::string& error = "") 
            : is_valid(valid), error_message(error) {}
    };
    
    /**
     * @brief Check if file type is allowed
     * @param filename Original filename
     * @param file_data File content for MIME detection
     * @return ValidationResult with details
     */
    static ValidationResult validate_file_type(const std::string& filename,
                                              const std::vector<uint8_t>& file_data);
    
    /**
     * @brief Check if file size is within limits
     * @param file_size Size in bytes
     * @param max_size Maximum allowed size (default: 25MB)
     * @return true if within limits
     */
    static bool is_within_size_limit(size_t file_size, 
                                    size_t max_size = 25 * 1024 * 1024);
    
    /**
     * @brief Scan file for viruses using ClamAV
     * @param file_path Path to file to scan
     * @return Empty string if clean, virus name if infected
     */
    static std::string scan_for_viruses(const std::string& file_path);
    
    /**
     * @brief Get MIME type from file content
     * @param file_data File content
     * @param filename Original filename (for fallback)
     * @return Detected MIME type
     */
    static std::string detect_mime_type(const std::vector<uint8_t>& file_data,
                                       const std::string& filename);
    
    /**
     * @brief Sanitize filename for safe storage
     * @param filename Original filename
     * @return Sanitized filename
     */
    static std::string sanitize_filename(const std::string& filename);

private:
    /**
     * @brief Allowed file extensions (case-insensitive)
     */
    static const std::set<std::string> ALLOWED_EXTENSIONS;
    
    /**
     * @brief Allowed MIME types
     */
    static const std::set<std::string> ALLOWED_MIME_TYPES;
    
    /**
     * @brief Check file magic numbers for type validation
     * @param file_data File content
     * @return true if magic numbers match allowed types
     */
    static bool check_magic_numbers(const std::vector<uint8_t>& file_data);
    
    /**
     * @brief Get file extension from filename
     * @param filename File name
     * @return Extension in lowercase (without dot)
     */
    static std::string get_file_extension(const std::string& filename);
    
    /**
     * @brief Get MIME type from file extension
     * @param extension File extension (without dot)
     * @return MIME type or empty string if unknown
     */
    static std::string get_mime_type_from_extension(const std::string& extension);
    
    /**
     * @brief Detect MIME type from magic numbers
     * @param file_data File content (at least first 512 bytes)
     * @return MIME type or empty string if unknown
     */
    static std::string detect_mime_from_magic(const std::vector<uint8_t>& file_data);
    
    /**
     * @brief Check if bytes match a specific magic number pattern
     * @param data File data
     * @param offset Offset to start checking
     * @param pattern Magic number pattern to match
     * @return true if pattern matches
     */
    static bool matches_magic_pattern(const std::vector<uint8_t>& data, 
                                     size_t offset, 
                                     const std::vector<uint8_t>& pattern);
};

#endif // FILE_VALIDATOR_H 