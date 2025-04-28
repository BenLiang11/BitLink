#include "handlers/static_file_handler.h"
#include "mime_types.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

StaticFileHandler::StaticFileHandler(const std::string& root_dir)
    : root_dir_(root_dir) {
    // Ensure root_dir_ ends with a slash
    if (!root_dir_.empty() && root_dir_.back() != '/') {
        root_dir_ += '/';
    }
}

bool StaticFileHandler::HandleRequest(const Request& request, Response* response) {
    // We only handle GET requests
    if (request.method() != "GET") {
        response->set_status(Response::NOT_FOUND);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>404 Not Found</h1><p>Method not supported.</p></body></html>");
        return false;
    }
    
    // Map the URI to a file path
    std::string file_path = MapUriToFilePath(request.uri());
    
    // Read the file into memory
    std::string content;
    if (!ReadFile(file_path, &content)) {
        response->set_status(Response::NOT_FOUND);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>404 Not Found</h1><p>The requested file could not be found.</p></body></html>");
        return false;
    }
    
    // Set the response
    response->set_status(Response::OK);
    response->set_header("Content-Type", GetMimeType(file_path));
    response->set_body(content);
    response->set_header("Connection", "close");
    
    return true;
}

std::string StaticFileHandler::GetMimeType(const std::string& path) const {
    // Get file extension
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string extension = path.substr(dot_pos + 1);
        return MimeTypes::GetMimeType(extension);
    }
    
    // If no extension, default to binary
    return "application/octet-stream";
}

std::string StaticFileHandler::MapUriToFilePath(const std::string& uri) const {
    // Skip the leading slash if it exists
    size_t start_pos = (uri.front() == '/') ? 1 : 0;
    
    // Handle default index file
    std::string path = uri.substr(start_pos);
    if (path.empty() || path.back() == '/') {
        path += "index.html";
    }
    
    // Combine with root directory
    return root_dir_ + path;
}

bool StaticFileHandler::ReadFile(const std::string& path, std::string* content) const {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Use a stringstream to read the entire file
    std::stringstream buffer;
    buffer << file.rdbuf();
    *content = buffer.str();
    
    return true;
} 