#ifndef MIME_TYPES_H
#define MIME_TYPES_H

#include <string>
#include <map>

/**
 * @brief Utility for mapping file extensions to MIME types.
 */
class MimeTypes {
public:
    /**
     * @brief Get the MIME type for a file extension.
     * 
     * @param extension The file extension (without the dot).
     * @return std::string The MIME type, or "application/octet-stream" if unknown.
     */
    static std::string GetMimeType(const std::string& extension);

private:
    /**
     * @brief Map of extension to MIME type.
     */
    static const std::map<std::string, std::string> mime_map_;
};

#endif // MIME_TYPES_H 