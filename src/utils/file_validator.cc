#include "utils/file_validator.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <map>

using namespace std;
namespace fs = std::filesystem;

// Define allowed file extensions (common safe file types)
const set<string> FileValidator::ALLOWED_EXTENSIONS = {
    // Images
    "jpg", "jpeg", "png", "gif", "bmp", "webp", "svg", "ico",
    // Documents
    "pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx", "txt", "rtf", "odt", "ods", "odp",
    // Archives
    "zip", "rar", "7z", "tar", "gz", "bz2",
    // Audio
    "mp3", "wav", "ogg", "flac", "aac", "m4a",
    // Video
    "mp4", "avi", "mkv", "mov", "wmv", "flv", "webm",
    // Code/Text
    "json", "xml", "csv", "html", "css", "js", "py", "cpp", "h", "java", "c"
};

// Define allowed MIME types
const set<string> FileValidator::ALLOWED_MIME_TYPES = {
    // Images
    "image/jpeg", "image/png", "image/gif", "image/bmp", "image/webp", "image/svg+xml", "image/x-icon",
    // Documents
    "application/pdf", "application/msword", "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
    "application/vnd.ms-excel", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    "application/vnd.ms-powerpoint", "application/vnd.openxmlformats-officedocument.presentationml.presentation",
    "text/plain", "application/rtf", "application/vnd.oasis.opendocument.text",
    "application/vnd.oasis.opendocument.spreadsheet", "application/vnd.oasis.opendocument.presentation",
    // Archives
    "application/zip", "application/x-rar-compressed", "application/x-7z-compressed",
    "application/x-tar", "application/gzip", "application/x-bzip2",
    // Audio
    "audio/mpeg", "audio/wav", "audio/ogg", "audio/flac", "audio/aac", "audio/mp4",
    // Video
    "video/mp4", "video/x-msvideo", "video/x-matroska", "video/quicktime", "video/x-ms-wmv",
    "video/x-flv", "video/webm",
    // Text/Code
    "application/json", "application/xml", "text/xml", "text/csv", "text/html", "text/css",
    "application/javascript", "text/x-python", "text/x-c++src", "text/x-csrc", "text/x-java-source"
};

FileValidator::ValidationResult FileValidator::validate_file_type(const string& filename,
                                                                 const vector<uint8_t>& file_data) {
    ValidationResult result;
    
    if (filename.empty()) {
        result.is_valid = false;
        result.error_message = "Filename cannot be empty";
        return result;
    }
    
    if (file_data.empty()) {
        result.is_valid = false;
        result.error_message = "File data cannot be empty";
        return result;
    }
    
    // Get file extension
    string extension = get_file_extension(filename);
    if (extension.empty()) {
        result.is_valid = false;
        result.error_message = "File must have a valid extension";
        return result;
    }
    
    // Check if extension is allowed
    if (ALLOWED_EXTENSIONS.find(extension) == ALLOWED_EXTENSIONS.end()) {
        result.is_valid = false;
        result.error_message = "File type '" + extension + "' is not allowed";
        return result;
    }
    
    // Detect MIME type from content
    string detected_mime = detect_mime_type(file_data, filename);
    result.detected_mime_type = detected_mime;
    
    // Check if detected MIME type is allowed
    if (!detected_mime.empty() && ALLOWED_MIME_TYPES.find(detected_mime) == ALLOWED_MIME_TYPES.end()) {
        result.is_valid = false;
        result.error_message = "Detected MIME type '" + detected_mime + "' is not allowed";
        return result;
    }
    
    // Check magic numbers for additional security
    if (!check_magic_numbers(file_data)) {
        result.is_valid = false;
        result.error_message = "File content does not match expected format";
        return result;
    }
    
    result.is_valid = true;
    return result;
}

bool FileValidator::is_within_size_limit(size_t file_size, size_t max_size) {
    return file_size <= max_size;
}

string FileValidator::scan_for_viruses(const string& file_path) {
    // Basic implementation - in production, integrate with ClamAV
    if (file_path.empty()) {
        return "Invalid file path";
    }
    
    if (!fs::exists(file_path)) {
        return "File does not exist";
    }
    
    // For now, return empty string (clean)
    // TODO: Integrate with ClamAV for real virus scanning
    // This would involve calling clamdscan or clamd daemon
    
    // Basic suspicious file name patterns
    string filename = fs::path(file_path).filename().string();
    transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    
    vector<string> suspicious_patterns = {
        ".exe", ".scr", ".bat", ".cmd", ".com", ".pif", ".vbs", ".js"
    };
    
    for (const string& pattern : suspicious_patterns) {
        if (filename.find(pattern) != string::npos) {
            return "Suspicious file type detected: " + pattern;
        }
    }
    
    return ""; // Clean
}

string FileValidator::detect_mime_type(const vector<uint8_t>& file_data,
                                      const string& filename) {
    if (file_data.empty()) {
        return "";
    }
    
    // First try magic number detection
    string mime_from_magic = detect_mime_from_magic(file_data);
    if (!mime_from_magic.empty()) {
        return mime_from_magic;
    }
    
    // Fallback to extension-based detection
    string extension = get_file_extension(filename);
    return get_mime_type_from_extension(extension);
}

string FileValidator::sanitize_filename(const string& filename) {
    if (filename.empty()) {
        return "unnamed_file";
    }
    
    string sanitized = filename;
    
    // Remove or replace dangerous characters
    regex dangerous_chars(R"([<>:"/\\|?*])");
    sanitized = regex_replace(sanitized, dangerous_chars, "_");
    
    // Remove control characters (0-31, 127)
    sanitized.erase(remove_if(sanitized.begin(), sanitized.end(),
                             [](char c) { return c >= 0 && c <= 31 || c == 127; }),
                   sanitized.end());
    
    // Trim whitespace
    sanitized = regex_replace(sanitized, regex(R"(^\s+|\s+$)"), "");
    
    // Remove leading dots and spaces (Windows compatibility)
    sanitized = regex_replace(sanitized, regex(R"(^[. ]+)"), "");
    
    // Limit length (keep extension)
    const size_t MAX_FILENAME_LENGTH = 255;
    if (sanitized.length() > MAX_FILENAME_LENGTH) {
        size_t ext_pos = sanitized.find_last_of('.');
        if (ext_pos != string::npos && ext_pos > MAX_FILENAME_LENGTH - 20) {
            string name = sanitized.substr(0, MAX_FILENAME_LENGTH - (sanitized.length() - ext_pos));
            string ext = sanitized.substr(ext_pos);
            sanitized = name + ext;
        } else {
            sanitized = sanitized.substr(0, MAX_FILENAME_LENGTH);
        }
    }
    
    // Check for reserved Windows names
    vector<string> reserved_names = {
        "CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5",
        "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4",
        "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    
    string name_without_ext = sanitized;
    size_t dot_pos = sanitized.find_last_of('.');
    if (dot_pos != string::npos) {
        name_without_ext = sanitized.substr(0, dot_pos);
    }
    
    string upper_name = name_without_ext;
    transform(upper_name.begin(), upper_name.end(), upper_name.begin(), ::toupper);
    
    if (find(reserved_names.begin(), reserved_names.end(), upper_name) != reserved_names.end()) {
        sanitized = "_" + sanitized;
    }
    
    // Ensure we have a valid filename
    if (sanitized.empty() || sanitized == "." || sanitized == "..") {
        sanitized = "unnamed_file";
    }
    
    // Check if filename is only underscores or meaningless characters
    bool only_underscores = !sanitized.empty() && 
                           all_of(sanitized.begin(), sanitized.end(), 
                                  [](char c) { return c == '_' || isspace(c); });
    
    if (only_underscores) {
        sanitized = "unnamed_file";
    }
    
    return sanitized;
}

bool FileValidator::check_magic_numbers(const vector<uint8_t>& file_data) {
    if (file_data.empty()) {
        return false;
    }
    
    // Check various file magic numbers
    // JPEG
    if (matches_magic_pattern(file_data, 0, {0xFF, 0xD8, 0xFF})) {
        return true;
    }
    
    // PNG
    if (matches_magic_pattern(file_data, 0, {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A})) {
        return true;
    }
    
    // GIF
    if (matches_magic_pattern(file_data, 0, {0x47, 0x49, 0x46, 0x38}) && 
        file_data.size() > 5 && (file_data[4] == 0x37 || file_data[4] == 0x39) && 
        file_data[5] == 0x61) {
        return true;
    }
    
    // PDF
    if (matches_magic_pattern(file_data, 0, {0x25, 0x50, 0x44, 0x46})) {
        return true;
    }
    
    // ZIP (and Office documents)
    if (matches_magic_pattern(file_data, 0, {0x50, 0x4B, 0x03, 0x04}) ||
        matches_magic_pattern(file_data, 0, {0x50, 0x4B, 0x05, 0x06}) ||
        matches_magic_pattern(file_data, 0, {0x50, 0x4B, 0x07, 0x08})) {
        return true;
    }
    
    // MP3
    if (matches_magic_pattern(file_data, 0, {0xFF, 0xFB}) ||
        matches_magic_pattern(file_data, 0, {0x49, 0x44, 0x33})) {
        return true;
    }
    
    // MP4
    if (file_data.size() > 8 && matches_magic_pattern(file_data, 4, {0x66, 0x74, 0x79, 0x70})) {
        return true;
    }
    
    // RAR
    if (matches_magic_pattern(file_data, 0, {0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00})) {
        return true;
    }
    
    // 7Z
    if (matches_magic_pattern(file_data, 0, {0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C})) {
        return true;
    }
    
    // BMP
    if (matches_magic_pattern(file_data, 0, {0x42, 0x4D})) {
        return true;
    }
    
    // Text files (UTF-8 BOM or plain ASCII/UTF-8)
    if (matches_magic_pattern(file_data, 0, {0xEF, 0xBB, 0xBF})) {
        return true; // UTF-8 BOM
    }
    
    // For text files, check if content is mostly printable ASCII/UTF-8
    bool is_text = true;
    size_t check_bytes = min(file_data.size(), size_t(512));
    for (size_t i = 0; i < check_bytes; i++) {
        uint8_t byte = file_data[i];
        // Allow printable ASCII, tabs, newlines, and high-bit UTF-8
        if (!(byte >= 32 && byte <= 126) && byte != 9 && byte != 10 && byte != 13 && byte < 128) {
            if (byte < 32) {
                is_text = false;
                break;
            }
        }
    }
    
    if (is_text) {
        return true;
    }
    
    // Allow unknown formats to pass through (for now)
    // In production, you might want to be more restrictive
    return true;
}

string FileValidator::get_file_extension(const string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == string::npos || dot_pos == filename.length() - 1) {
        return "";
    }
    
    string extension = filename.substr(dot_pos + 1);
    transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return extension;
}
string FileValidator::get_mime_type_from_extension(const string& extension) {
    static const std::map<string, string> extension_to_mime = {
        // Images
        {"jpg", "image/jpeg"}, {"jpeg", "image/jpeg"}, {"png", "image/png"},
        {"gif", "image/gif"}, {"bmp", "image/bmp"}, {"webp", "image/webp"},
        {"svg", "image/svg+xml"}, {"ico", "image/x-icon"},
        
        // Documents
        {"pdf", "application/pdf"}, {"doc", "application/msword"},
        {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {"xls", "application/vnd.ms-excel"},
        {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
        {"ppt", "application/vnd.ms-powerpoint"},
        {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
        {"txt", "text/plain"}, {"rtf", "application/rtf"},
        
        // Archives
        {"zip", "application/zip"}, {"rar", "application/x-rar-compressed"},
        {"7z", "application/x-7z-compressed"}, {"tar", "application/x-tar"},
        {"gz", "application/gzip"}, {"bz2", "application/x-bzip2"},
        
        // Audio
        {"mp3", "audio/mpeg"}, {"wav", "audio/wav"}, {"ogg", "audio/ogg"},
        {"flac", "audio/flac"}, {"aac", "audio/aac"}, {"m4a", "audio/mp4"},
        
        // Video
        {"mp4", "video/mp4"}, {"avi", "video/x-msvideo"}, {"mkv", "video/x-matroska"},
        {"mov", "video/quicktime"}, {"wmv", "video/x-ms-wmv"}, {"flv", "video/x-flv"},
        {"webm", "video/webm"},
        
        // Code/Text
        {"json", "application/json"}, {"xml", "application/xml"}, {"csv", "text/csv"},
        {"html", "text/html"}, {"css", "text/css"}, {"js", "application/javascript"},
        {"py", "text/x-python"}, {"cpp", "text/x-c++src"}, {"h", "text/x-csrc"},
        {"java", "text/x-java-source"}, {"c", "text/x-csrc"}
    };
    
    auto it = extension_to_mime.find(extension);
    return (it != extension_to_mime.end()) ? it->second : "";
}

string FileValidator::detect_mime_from_magic(const vector<uint8_t>& file_data) {
    if (file_data.empty()) {
        return "";
    }
    
    // JPEG
    if (matches_magic_pattern(file_data, 0, {0xFF, 0xD8, 0xFF})) {
        return "image/jpeg";
    }
    
    // PNG
    if (matches_magic_pattern(file_data, 0, {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A})) {
        return "image/png";
    }
    
    // GIF
    if (matches_magic_pattern(file_data, 0, {0x47, 0x49, 0x46, 0x38})) {
        return "image/gif";
    }
    
    // PDF
    if (matches_magic_pattern(file_data, 0, {0x25, 0x50, 0x44, 0x46})) {
        return "application/pdf";
    }
    
    // ZIP
    if (matches_magic_pattern(file_data, 0, {0x50, 0x4B, 0x03, 0x04})) {
        return "application/zip";
    }
    
    // MP3
    if (matches_magic_pattern(file_data, 0, {0xFF, 0xFB}) ||
        matches_magic_pattern(file_data, 0, {0x49, 0x44, 0x33})) {
        return "audio/mpeg";
    }
    
    // BMP
    if (matches_magic_pattern(file_data, 0, {0x42, 0x4D})) {
        return "image/bmp";
    }
    
    // Check for text content
    if (matches_magic_pattern(file_data, 0, {0xEF, 0xBB, 0xBF})) {
        return "text/plain"; // UTF-8 BOM
    }
    
    // Check if it's likely text
    size_t check_bytes = min(file_data.size(), size_t(512));
    bool likely_text = true;
    for (size_t i = 0; i < check_bytes; i++) {
        uint8_t byte = file_data[i];
        if (!(byte >= 32 && byte <= 126) && byte != 9 && byte != 10 && byte != 13) {
            if (byte < 128) {
                likely_text = false;
                break;
            }
        }
    }
    
    if (likely_text) {
        return "text/plain";
    }
    
    return ""; // Unknown
}

bool FileValidator::matches_magic_pattern(const vector<uint8_t>& data, 
                                         size_t offset, 
                                         const vector<uint8_t>& pattern) {
    if (data.size() < offset + pattern.size()) {
        return false;
    }
    
    for (size_t i = 0; i < pattern.size(); i++) {
        if (data[offset + i] != pattern[i]) {
            return false;
        }
    }
    
    return true;
} 