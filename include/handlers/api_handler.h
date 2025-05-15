#ifndef API_HANDLER_H
#define API_HANDLER_H

#include "handlers/base_handler.h"
#include <string>
#include <vector>
#include <memory>

/**
 * @brief Handler that implements the CRUD API, storing the created data in a specified root directory.
 *
 * This handler facilitates the manipulation of Entities.
 * An instance of Entity consists of its 
 * 1)ID
 * 2)JSON data
 */
class ApiHandler : public RequestHandler {
public:
    /**
     * @brief Construct a new ApiHandler.
     *
     * @param serving_path The URL prefix this handler is registered for (e.g., "/api").
     * @param root_directory The root directory on the filesystem from which to serve files (e.g., "./files").
     *                       This path is relative to the webserver binary location as per Common API.
     */
    ApiHandler(const std::string& serving_path, const std::string& root_directory);

    /**
     * @brief Handle an HTTP request by performing the appropriate CRUD operation.
     *
     * @param req The HTTP request object.
     * @return A unique_ptr to the response object.
     */
    std::unique_ptr<Response> handle_request(const Request& req) override;

    /**
     * @brief Static factory function for creating ApiHandler instances.
     *        Registered with HandlerRegistry.
     *
     * @param args A vector of string arguments. Expected: {serving_path, root_directory}.
     * @return A unique_ptr to a new ApiHandler instance.
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

#endif // API_HANDLER_H 