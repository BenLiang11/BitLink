#include "handlers/static_file_handler.h"
#include "handler_registry.h"
#include "mime_types.h"
#include "request.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

StaticFileHandler::StaticFileHandler(const std::string& serving_path, const std::string& root_directory)
    : serving_path_(serving_path), root_directory_(root_directory) {
    if (serving_path_.empty() || serving_path_[0] != '/') {
        throw std::invalid_argument("StaticFileHandler serving_path must start with '/'. Path: " + serving_path_);
    }
    if (serving_path_.length() > 1 && serving_path_.back() == '/') {
        serving_path_.pop_back();
    }
}

std::unique_ptr<Response> StaticFileHandler::handle_request(const Request& req) {
    auto response = std::make_unique<Response>();

    if (req.method() != "GET") {
        response->set_status(Response::NOT_FOUND);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>405 Method Not Allowed</h1></body></html>");
        return response;
    }

    std::string request_uri = req.uri();

    if (request_uri.rfind(serving_path_, 0) != 0) {
        response->set_status(Response::INTERNAL_SERVER_ERROR);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>500 Internal Server Error</h1><p>URI mismatch.</p></body></html>");
        return response;
    }

    std::string relative_path;
    if (request_uri.length() == serving_path_.length() || (serving_path_ == "/" && request_uri.length() == 1)) {
        relative_path = "index.html";
    } else if (serving_path_ == "/") {
        relative_path = request_uri.substr(1);
    } else {
        relative_path = request_uri.substr(serving_path_.length() + 1);
    }
    
    if (relative_path.empty() || relative_path.back() == '/') {
        relative_path += "index.html";
    }

    fs::path fs_root_path = root_directory_;
    fs::path fs_relative_path = relative_path;
    fs::path full_file_path = fs_root_path / fs_relative_path;

    std::string canonical_root = fs::weakly_canonical(fs_root_path).string();
    std::string canonical_file = fs::weakly_canonical(full_file_path).string();

    if (canonical_file.rfind(canonical_root, 0) != 0) {
        response->set_status(Response::FORBIDDEN);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>403 Forbidden</h1><p>Requested resource is outside the allowed directory.</p></body></html>");
        return response;
    }
    
    std::ifstream file(full_file_path.string(), std::ios::binary);
    if (!file) {
        response->set_status(Response::NOT_FOUND);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>404 Not Found</h1><p>The requested file could not be found.</p></body></html>");
        return response;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    response->set_status(Response::OK);
    response->set_header("Content-Type", get_mime_type(full_file_path.string()));
    response->set_body(content);

    return response;
}

std::string StaticFileHandler::get_mime_type(const std::string& file_path) const {
    size_t dot_pos = file_path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string extension = file_path.substr(dot_pos + 1);
        return MimeTypes::GetMimeType(extension);
    }
    return "application/octet-stream";
}

std::unique_ptr<RequestHandler> StaticFileHandler::Create(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        throw std::invalid_argument("StaticFileHandler factory requires 2 arguments: serving_path and root_directory. Got " + std::to_string(args.size()));
    }
    
    // Check for empty arguments
    if (args[0].empty()) {
        throw std::invalid_argument("StaticFileHandler serving_path cannot be empty");
    }
    if (args[1].empty()) {
        throw std::invalid_argument("StaticFileHandler root_directory cannot be empty");
    }
    
    return std::make_unique<StaticFileHandler>(args[0], args[1]);
}

namespace {
    const bool static_file_handler_registered = HandlerRegistry::RegisterHandler("StaticHandler", StaticFileHandler::Create);
} 