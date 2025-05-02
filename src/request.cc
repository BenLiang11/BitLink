#include "request.h"
#include <sstream>
#include <algorithm>
#include <iostream>

// Initialize the empty header string
const std::string Request::empty_header_ = "";

Request::Request(const std::string& raw_request)
    : raw_request_(raw_request) {
    parse_request();
}

const std::string& Request::get_header(const std::string& name) const {
    // First try exact match
    auto it = headers_.find(name);
    if (it != headers_.end()) {
        return it->second;
    }
    
    // Case-insensitive search
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), 
                  [](unsigned char c){ return std::tolower(c); });
    
    for (const auto& header : headers_) {
        std::string lower_header = header.first;
        std::transform(lower_header.begin(), lower_header.end(), lower_header.begin(), 
                      [](unsigned char c){ return std::tolower(c); });
        
        if (lower_header == lower_name) {
            return header.second;
        }
    }
    
    return empty_header_;
}

void Request::parse_request() {
    std::istringstream request_stream(raw_request_);
    std::string line;

    // Parse the request line (GET /path HTTP/1.1)
    if (std::getline(request_stream, line)) {
        // Remove carriage return if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::istringstream line_stream(line);
        line_stream >> method_ >> uri_ >> version_;
    }

    // Parse headers
    while (std::getline(request_stream, line) && !line.empty()) {
        // Remove carriage return if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Empty line denotes end of headers
        if (line.empty()) {
            break;
        }

        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string header_name = line.substr(0, colon_pos);
            
            // Skip the colon and any following whitespace
            size_t value_pos = colon_pos + 1;
            while (value_pos < line.size() && std::isspace(line[value_pos])) {
                ++value_pos;
            }
            
            std::string header_value = line.substr(value_pos);
            headers_[header_name] = header_value;
        }
    }

    // The rest is the body
    std::stringstream body_stream;
    while (std::getline(request_stream, line)) {
        body_stream << line << "\n";
    }
    body_ = body_stream.str();
    
    // Remove trailing newline if present
    if (!body_.empty() && body_.back() == '\n') {
        body_.pop_back();
    }
}

std::string Request::get_file_path(const std::string& api_path) const {
    // Remove leading slash if present
    std::string path = api_path;
    if (!path.empty() && path[0] == '/') {
        path.erase(0, 1);

    }

    // Remove the api path from uri to get the file path
    size_t pos = uri_.find(path);
    
    if (pos != std::string::npos) {
        return uri_.substr(pos + path.length());
    }

    return "";
}