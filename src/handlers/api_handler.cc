#include "handlers/api_handler.h"
#include "handler_registry.h"
#include "mime_types.h"
#include "request.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

ApiHandler::ApiHandler(const std::string& serving_path, const std::string& root_directory)
    : serving_path_(serving_path), root_directory_(root_directory) {
    if (serving_path_.empty() || serving_path_[0] != '/') {
        throw std::invalid_argument("ApiHandler serving_path must start with '/'. Path: " + serving_path_);
    }
    if (serving_path_.length() > 1 && serving_path_.back() == '/') {
        serving_path_.pop_back();
    }
}

std::unique_ptr<Response> ApiHandler::handle_request(const Request& req) {
    auto response = std::make_unique<Response>();

    std::string request_uri = req.uri();
    std::string request_method = req.method();

    if ((request_uri.length() <= 5) || (request_uri.substr(0,5) != "/api/")) {
        response->set_status(Response::FORBIDDEN);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>403 Forbidden</h1>"
        "<p>You may only access subdirectories of the /api directory.</p>"
        "<p>For example: /api/Shoes</p></body></html>");
        return response;
    }

    std::string relative_path = request_uri.substr(5);
    fs::path fs_root_path = root_directory_;
    fs::path fs_relative_path = relative_path;
    fs::path base_file_path = fs_root_path / fs_relative_path;

    if (request_method == "POST") {
        // Handle POST request
        int id = 1;
        while (fs::exists(base_file_path / (std::to_string(id)))) {
            id++;
        }

        std::ofstream file(base_file_path.string() + "/" + std::to_string(id), std::ios::binary);
        file << req.body();
        file.close();

        response->set_status(Response::CREATED);
        response->set_header("Content-Type", "text/html");
        response->set_body("{\"id\": "+std::to_string(id)+"}");
        return response;
    }

    if (request_method == "PUT") {
        // Handle PUT request

        if (!fs::exists(base_file_path)) {
            response->set_status(Response::NOT_FOUND);
            response->set_header("Content-Type", "text/html");
            response->set_body("<html><body><h1>404 Not Found</h1><p>The requested file could not be found.</p></body></html>");
            return response;
        }

        std::ofstream file(base_file_path.string(), std::ios::binary);
        file << req.body();
        file.close();

        response->set_status(Response::OK);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>200 OK</h1><p>File updated successfully.</p></body></html>");
        return response;
    }

    if (request_method == "GET") {
        // Handle GET request
        if (!fs::exists(base_file_path)) {
            // Handle nonexistant get request.
            response->set_status(Response::NOT_FOUND);
            response->set_header("Content-Type", "text/html");
            response->set_body("<html><body><h1>404 Not Found</h1><p>The requested file could not be found.</p></body></html>");
            return response;
        }

        if (fs::is_directory(base_file_path)) {
            // Handle list operation
            std::string response_body = "{\"file_ids\": [";
            for (const auto& entry : fs::directory_iterator(base_file_path)) {
                if (fs::is_regular_file(entry.path())) {
                    response_body += entry.path().filename().string() + ",";
                }
            }
            response_body.pop_back(); // Remove trailing comma
            response_body += "]}";

            response->set_status(Response::OK);
            response->set_header("Content-Type", "application/json");
            response->set_body(response_body);
            return response;
        }

        if (fs::is_regular_file(base_file_path)) {
            // Handle get operation
            std::ifstream file(base_file_path.string(), std::ios::binary);
            if (!file) {
                response->set_status(Response::INTERNAL_SERVER_ERROR);
                response->set_header("Content-Type", "text/html");
                response->set_body("<html><body><h1>500 Internal Server Error</h1><p>Error reading file.</p></body></html>");
                return response;
            }

            std::stringstream content;
            content << file.rdbuf();

            response->set_status(Response::OK);
            response->set_header("Content-Type", "application/json");
            response->set_body(content.str());
            return response;
        }

        //Unsupported file type
        response->set_status(Response::INTERNAL_SERVER_ERROR);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>500 Internal Server Error</h1><p>Unsupported file type.</p></body></html>");
        return response;
    }

    if (request_method == "DELETE") {
        // Handle DELETE request
        if (!fs::exists(base_file_path)) {
            response->set_status(Response::NOT_FOUND);
            response->set_header("Content-Type", "text/html");
            response->set_body("<html><body><h1>404 Not Found</h1><p>The requested file could not be found.</p></body></html>");
            return response;
        }

        if (!fs::is_regular_file(base_file_path)) {
            response->set_status(Response::INTERNAL_SERVER_ERROR);
            response->set_header("Content-Type", "text/html");
            response->set_body("<html><body><h1>500 Internal Server Error</h1><p>File is not a regular file and cannot be deleted.</p></body></html>");
            return response;
        }

        if (!fs::remove(base_file_path)) {
            response->set_status(Response::INTERNAL_SERVER_ERROR);
            response->set_header("Content-Type", "text/html");
            response->set_body("<html><body><h1>500 Internal Server Error</h1><p>Error deleting file.</p></body></html>");
            return response;
        }

        response->set_status(Response::OK);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>200 OK</h1><p>File deleted successfully.</p></body></html>");
        return response;
    }

    // if (req.method() != "GET") {
    //     response->set_status(Response::NOT_FOUND);
    //     response->set_header("Content-Type", "text/html");
    //     response->set_body("<html><body><h1>405 Method Not Allowed</h1></body></html>");
    //     return response;
    // }

    response->set_status(Response::OK);
    response->set_header("Content-Type", "text/html");
    response->set_body("<html><body><h1>ApiHandler is not yet fully implemented</h1><p>Serving path is: "+this->serving_path_+
    "Root directory is: "+this->root_directory_+"</p></body></html>");
    return response;

    /*
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
    */
}

std::string ApiHandler::get_mime_type(const std::string& file_path) const {
    size_t dot_pos = file_path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string extension = file_path.substr(dot_pos + 1);
        return MimeTypes::GetMimeType(extension);
    }
    return "application/octet-stream";
}

std::unique_ptr<RequestHandler> ApiHandler::Create(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        throw std::invalid_argument("ApiHandler factory requires 2 arguments: serving_path and root_directory. Got " + std::to_string(args.size()));
    }
    
    // Check for empty arguments
    if (args[0].empty()) {
        throw std::invalid_argument("ApiHandler serving_path cannot be empty");
    }
    if (args[1].empty()) {
        throw std::invalid_argument("ApiHandler root_directory cannot be empty");
    }
    
    return std::make_unique<ApiHandler>(args[0], args[1]);
}

namespace {
    const bool api_handler_registered = HandlerRegistry::RegisterHandler("ApiHandler", ApiHandler::Create);
} 