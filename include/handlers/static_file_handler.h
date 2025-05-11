#ifndef STATIC_FILE_HANDLER_H
#define STATIC_FILE_HANDLER_H

#include "handlers/base_handler.h"
#include <string>
#include <vector>
#include <memory>

/**
 * @brief Handler that serves static files from a specified root directory.
 *
 * This handler maps the request URI (relative to its configured serving path)
 * to a file path in a specified root directory on the filesystem.
 * It reads the file (if it exists and is accessible) and sends it in the response.
 * If the file doesn't exist or an error occurs, it returns an appropriate error response (e.g., 404 Not Found).
 */
class StaticFileHandler : public RequestHandler {
public:
    /**
     * @brief Construct a new StaticFileHandler.
     *
     * @param serving_path The URL prefix this handler is registered for (e.g., "/static").
     * @param root_directory The root directory on the filesystem from which to serve files (e.g., "./files").
     *                       This path is relative to the webserver binary location as per Common API.
     */
    StaticFileHandler(const std::string& serving_path, const std::string& root_directory);

    /**
     * @brief Handle an HTTP request by serving a static file.
     *
     * @param req The HTTP request object.
     * @return A unique_ptr to the response object.
     */
    std::unique_ptr<Response> handle_request(const Request& req) override;

    /**
     * @brief Static factory function for creating StaticFileHandler instances.
     *        Registered with HandlerRegistry.
     *
     * @param args A vector of string arguments. Expected: {serving_path, root_directory}.
     * @return A unique_ptr to a new StaticFileHandler instance.
     * @throws std::invalid_argument if the number of arguments is incorrect.
     */
    static std::unique_ptr<RequestHandler> Create(const std::vector<std::string>& args);

private:
    std::string serving_path_;
    std::string root_directory_;

    /**
     * @brief Get the MIME type for a file based on its extension.
     *
     * @param file_path The full path to the file.
     * @return std::string The MIME type (e.g., "text/html", "image/jpeg"). Defaults to "application/octet-stream".
     */
    std::string get_mime_type(const std::string& file_path) const;

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