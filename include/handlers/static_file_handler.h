#ifndef STATIC_FILE_HANDLER_H
#define STATIC_FILE_HANDLER_H

#include "handlers/base_handler.h"
#include <string>

/**
 * @brief Handler that serves static files from disk.
 * 
 * This handler maps the request URI to a file path in a specified root
 * directory, reads the file (if it exists), and sends it in the response.
 * If the file doesn't exist, it returns a 404 Not Found response.
 */
class StaticFileHandler : public RequestHandler {
public:
    /**
     * @brief Construct a new StaticFileHandler with a root directory.
     * 
     * @param root_dir The root directory from which to serve files.
     */
    explicit StaticFileHandler(const std::string& root_dir);

    /**
     * @brief Handle an HTTP request by serving a static file.
     * 
     * @param request The HTTP request to handle.
     * @param response The HTTP response to populate.
     * @return bool true if handled successfully, false otherwise.
     */
    bool HandleRequest(const Request& request, Response* response) override;

private:
    std::string root_dir_;

    /**
     * @brief Get the MIME type for a file based on its extension.
     * 
     * @param path The file path.
     * @return std::string The MIME type.
     */
    std::string GetMimeType(const std::string& path) const;

    /**
     * @brief Map a URI to a local file path.
     * 
     * @param uri The request URI.
     * @return std::string The corresponding local file path.
     */
    std::string MapUriToFilePath(const std::string& uri) const;

    /**
     * @brief Read a file into a string.
     * 
     * @param path The file path.
     * @param content The string to hold the file content.
     * @return bool true if read successfully, false otherwise.
     */
    bool ReadFile(const std::string& path, std::string* content) const;
};

#endif // STATIC_FILE_HANDLER_H 