#include "mime_types.h"
#include <algorithm>

// Initialize the MIME type map
const std::map<std::string, std::string> MimeTypes::mime_map_ = {
    {"html", "text/html"},
    {"htm", "text/html"},
    {"css", "text/css"},
    {"js", "application/javascript"},
    {"json", "application/json"},
    {"txt", "text/plain"},
    {"md", "text/markdown"},
    {"png", "image/png"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"gif", "image/gif"},
    {"svg", "image/svg+xml"},
    {"ico", "image/x-icon"},
    {"pdf", "application/pdf"},
    {"zip", "application/zip"},
    {"tar", "application/x-tar"},
    {"gz", "application/gzip"},
    {"mp3", "audio/mpeg"},
    {"mp4", "video/mp4"},
};

std::string MimeTypes::GetMimeType(const std::string& extension) {
    // Convert to lowercase for case-insensitive comparison
    std::string ext_lower = extension;
    std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    // Look up in the map
    auto it = mime_map_.find(ext_lower);
    if (it != mime_map_.end()) {
        return it->second;
    }
    
    // Default MIME type for unknown extensions
    return "application/octet-stream";
} 