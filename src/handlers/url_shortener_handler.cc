#include "handlers/url_shortener_handler.h"
#include "utils/http_parser.h"
#include "utils/json_builder.h"
#include "utils/qr_generator.h"
#include "database/sqlite_database.h"
#include "handler_registry.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <optional>

using namespace std;

URLShortenerHandler::URLShortenerHandler(const string& serving_path,
                                       const string& upload_dir,
                                       const string& db_path,
                                       const string& base_url)
    : serving_path_(serving_path), upload_dir_(upload_dir), 
      db_path_(db_path), base_url_(base_url) {
    
    // Services will be initialized on first request
    url_service_ = nullptr;
    file_service_ = nullptr;
}

bool URLShortenerHandler::initialize_services() {
    if (url_service_ && file_service_) {
        return true; // Already initialized
    }
    
    try {
        // Create database instance for URL service
        auto url_db = make_unique<SQLiteDatabase>();
        if (!url_db->initialize(db_path_)) {
            cerr << "Failed to initialize URL database: " << db_path_ << endl;
            return false;
        }
        
        // Create tables if they don't exist
        if (!url_db->create_tables()) {
            cerr << "Failed to create database tables" << endl;
            return false;
        }
        
        // Create database instance for file service (separate instance)
        auto file_db = make_unique<SQLiteDatabase>();
        if (!file_db->initialize(db_path_)) {
            cerr << "Failed to initialize file database: " << db_path_ << endl;
            return false;
        }
        
        // Initialize URL shortening service
        url_service_ = make_unique<URLShorteningService>(
            move(url_db), base_url_);
        
        // Initialize file upload service  
        file_service_ = make_unique<FileUploadService>(
            move(file_db), upload_dir_, base_url_);
            
        return true;
        
    } catch (const exception& e) {
        cerr << "Service initialization failed: " << e.what() << endl;
        return false;
    }
}

unique_ptr<Response> URLShortenerHandler::handle_request(const Request& req) {
    // Initialize services on first request
    if (!initialize_services()) {
        return create_error_response("Service initialization failed", 
                                   Response::INTERNAL_SERVER_ERROR);
    }
    
    string path = req.uri();
    string method = req.method();
    
    // Route requests based on path and method
    if (path == "/" || path == serving_path_) {
        return handle_main_page(req);
    }
    else if (path == "/shorten") {
        if (method == "POST") {
            return handle_shorten_url(req);
        } else {
            return create_error_response("Method not allowed", Response::METHOD_NOT_ALLOWED);
        }
    }
    else if (path == "/upload") {
        if (method == "POST") {
            return handle_upload_file(req);
        } else {
            return create_error_response("Method not allowed", Response::METHOD_NOT_ALLOWED);
        }
    }
    else if (path.substr(0, 3) == "/r/") {
        return handle_redirect(req);
    }
    else if (path.substr(0, 7) == "/stats/") {
        return handle_stats(req);
    }
    else if (path.substr(0, 4) == "/qr/") {
        return handle_qr_code(req);
    }
    
    return create_error_response("Not Found", Response::NOT_FOUND);
}

unique_ptr<Response> URLShortenerHandler::handle_main_page(const Request& req) {
    // Serve static HTML file or generate dynamic content
    string html_content = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>URL Shortener & File Sharing</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }
        .section { margin: 30px 0; padding: 20px; border: 1px solid #ccc; border-radius: 8px; }
        input[type="url"], input[type="file"] { width: 70%; padding: 10px; margin: 10px 0; }
        button { padding: 10px 20px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer; }
        button:hover { background: #0056b3; }
        .result { margin: 20px 0; padding: 15px; background: #f8f9fa; border-radius: 4px; }
        .qr-code { margin: 10px 0; }
        .copy-btn { background: #28a745; margin-left: 10px; }
        .copy-btn:hover { background: #1e7e34; }
    </style>
</head>
<body>
    <div class="container">
        <h1>URL Shortener & File Sharing</h1>
        
        <!-- URL Shortening Form -->
        <div class="section">
            <h2>Shorten URL</h2>
            <form id="url-form">
                <input type="url" id="url-input" placeholder="Enter URL to shorten" required>
                <button type="submit">Shorten</button>
            </form>
            <div id="url-result" class="result" style="display:none;"></div>
        </div>
        
        <!-- File Upload Form -->
        <div class="section">
            <h2>Upload File</h2>
            <form id="file-form">
                <input type="file" id="file-input" required>
                <button type="submit">Upload</button>
            </form>
            <div id="file-result" class="result" style="display:none;"></div>
        </div>
    </div>
    
    <script>
        document.getElementById('url-form').addEventListener('submit', async function(e) {
            e.preventDefault();
            const url = document.getElementById('url-input').value;
            const response = await fetch('/shorten', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'url=' + encodeURIComponent(url)
            });
            const result = await response.json();
            const resultDiv = document.getElementById('url-result');
            if (result.success) {
                resultDiv.innerHTML = 
                    '<p><strong>Short URL:</strong> <a href="' + result.short_url + '" target="_blank">' + result.short_url + '</a> ' +
                    '<button class="copy-btn" onclick="copyToClipboard(\'' + result.short_url + '\')">Copy</button></p>' +
                    (result.qr_code ? '<div class="qr-code"><img src="' + result.qr_code + '" alt="QR Code"></div>' : '');
            } else {
                resultDiv.innerHTML = '<p style="color:red;">Error: ' + (result.error_message || 'Unknown error') + '</p>';
            }
            resultDiv.style.display = 'block';
        });

        document.getElementById('file-form').addEventListener('submit', async function(e) {
            e.preventDefault();
            const formData = new FormData();
            formData.append('file', document.getElementById('file-input').files[0]);
            const response = await fetch('/upload', {
                method: 'POST',
                body: formData
            });
            const result = await response.json();
            const resultDiv = document.getElementById('file-result');
            if (result.success) {
                resultDiv.innerHTML = 
                    '<p><strong>Download URL:</strong> <a href="' + result.short_url + '" target="_blank">' + result.short_url + '</a> ' +
                    '<button class="copy-btn" onclick="copyToClipboard(\'' + result.short_url + '\')">Copy</button></p>' +
                    '<p><strong>Filename:</strong> ' + result.filename + '</p>' +
                    (result.qr_code ? '<div class="qr-code"><img src="' + result.qr_code + '" alt="QR Code"></div>' : '');
            } else {
                resultDiv.innerHTML = '<p style="color:red;">Error: ' + (result.error_message || 'Unknown error') + '</p>';
            }
            resultDiv.style.display = 'block';
        });

        function copyToClipboard(text) {
            navigator.clipboard.writeText(text).then(function() {
                alert('Copied to clipboard!');
            });
        }
    </script>
</body>
</html>
)HTML";
    
    auto response = make_unique<Response>();
    response->set_status_code(Response::OK);
    response->set_header("Content-Type", "text/html");
    response->set_body(html_content);
    
    return response;
}

unique_ptr<Response> URLShortenerHandler::handle_shorten_url(const Request& req) {
    try {
        // Validate request method
        if (req.method() != "POST") {
            return create_error_response("Method not allowed", Response::METHOD_NOT_ALLOWED);
        }
        
        // Parse and validate input
        auto form_data = parse_form_data(req.body());
        auto url_it = form_data.find("url");
        
        if (url_it == form_data.end() || url_it->second.empty()) {
            return create_error_response("URL parameter required", Response::BAD_REQUEST);
        }
        
        // Use URL shortening service
        auto result = url_service_->shorten_url(url_it->second);
        
        if (!result.success) {
            // Log the error for debugging
            cerr << "URL shortening failed: " << result.error_message << endl;
            return create_error_response(result.error_message, Response::BAD_REQUEST);
        }
        
        // Success response
        JSONBuilder response_json;
        response_json.add("success", true)
                    .add("code", result.code)
                    .add("short_url", result.short_url)
                    .add("qr_code", result.qr_code_data_url);
        
        return create_json_response(response_json.build());
        
    } catch (const exception& e) {
        cerr << "Exception in handle_shorten_url: " << e.what() << endl;
        return create_error_response("Internal server error", Response::INTERNAL_SERVER_ERROR);
    }
}

unique_ptr<Response> URLShortenerHandler::handle_upload_file(const Request& req) {
    try {
        // Validate request method
        if (req.method() != "POST") {
            return create_error_response("Method not allowed", Response::METHOD_NOT_ALLOWED);
        }
        
        // Extract uploaded file from multipart request
        auto file_data = extract_uploaded_file(req);
        
        if (!file_data) {
            return create_error_response("No file uploaded", Response::BAD_REQUEST);
        }
        
        // Use file upload service
        auto result = file_service_->upload_file(file_data->filename, file_data->data);
        
        if (!result.success) {
            return create_error_response(result.error_message, Response::BAD_REQUEST);
        }
        
        // Build success response
        JSONBuilder response_json;
        response_json.add("success", true)
                    .add("code", result.code)
                    .add("short_url", result.short_url)
                    .add("filename", result.original_filename)
                    .add("qr_code", result.qr_code_data_url);
        
        return create_json_response(response_json.build());
        
    } catch (const exception& e) {
        cerr << "Exception in handle_upload_file: " << e.what() << endl;
        return create_error_response("Internal server error", Response::INTERNAL_SERVER_ERROR);
    }
}

unique_ptr<Response> URLShortenerHandler::handle_redirect(const Request& req) {
    try {
        // Extract code from path /r/<code>
        string code = extract_code_from_path(req.uri(), "/r/");
        
        if (code.empty()) {
            return create_error_response("Invalid code", Response::BAD_REQUEST);
        }
        
        // Record analytics
        string client_ip = get_client_ip(req);
        string user_agent = get_user_agent(req);
        string referrer = get_referrer(req);
        
        // Try URL shortening service first
        auto url = url_service_->resolve_code(code);
        if (url) {
            url_service_->record_access(code, client_ip, user_agent, referrer);
            return create_redirect_response(*url);
        }
        
        // Try file service
        auto file_path = file_service_->get_file_path(code);
        if (file_path) {
            file_service_->record_file_access(code, client_ip, user_agent, referrer);
            return create_file_response(*file_path);
        }
        
        return create_error_response("Code not found", Response::NOT_FOUND);
        
    } catch (const exception& e) {
        cerr << "Exception in handle_redirect: " << e.what() << endl;
        return create_error_response("Internal server error", Response::INTERNAL_SERVER_ERROR);
    }
}

unique_ptr<Response> URLShortenerHandler::handle_stats(const Request& req) {
    try {
        string code = extract_code_from_path(req.uri(), "/stats/");
        
        if (code.empty()) {
            return create_error_response("Invalid code", Response::BAD_REQUEST);
        }
        
        // Get statistics from service
        auto stats = url_service_->get_statistics(code);
        
        // Build JSON response
        JSONBuilder response_json;
        response_json.add("success", true)
                    .add("code", code)
                    .add("total_clicks", stats.total_clicks);
        
        // Add daily statistics
        vector<string> dates;
        vector<int> counts;
        for (const auto& daily : stats.daily_clicks) {
            dates.push_back(daily.first);
            counts.push_back(daily.second);
        }
        
        response_json.add_array("dates", dates)
                    .add_array("click_counts", counts);
        
        return create_json_response(response_json.build());
        
    } catch (const exception& e) {
        cerr << "Exception in handle_stats: " << e.what() << endl;
        return create_error_response("Internal server error", Response::INTERNAL_SERVER_ERROR);
    }
}

unique_ptr<Response> URLShortenerHandler::handle_qr_code(const Request& req) {
    try {
        string code = extract_code_from_path(req.uri(), "/qr/");
        
        if (code.empty()) {
            return create_error_response("Invalid code", Response::BAD_REQUEST);
        }
        
        // Build short URL for the code
        string short_url = base_url_;
        if (!short_url.empty() && short_url.back() == '/') {
            short_url.pop_back();
        }
        short_url += "/r/" + code;
        
        // Generate QR code PNG
        try {
            auto qr_png = QRCodeGenerator::generate_png(short_url, 200, 
                                                       QRCodeGenerator::ErrorCorrection::MEDIUM);
            
            auto response = make_unique<Response>();
            response->set_status_code(Response::OK);
            response->set_header("Content-Type", "image/png");
            response->set_header("Cache-Control", "public, max-age=3600");
            
            // Convert vector<uint8_t> to string
            string png_data(qr_png.begin(), qr_png.end());
            response->set_body(png_data);
            
            return response;
            
        } catch (const exception& e) {
            return create_error_response("QR code generation failed", Response::INTERNAL_SERVER_ERROR);
        }
        
    } catch (const exception& e) {
        cerr << "Exception in handle_qr_code: " << e.what() << endl;
        return create_error_response("Internal server error", Response::INTERNAL_SERVER_ERROR);
    }
}

string URLShortenerHandler::extract_code_from_path(const string& path, const string& prefix) {
    if (path.length() <= prefix.length()) {
        return "";
    }
    
    return path.substr(prefix.length());
}

string URLShortenerHandler::get_client_ip(const Request& req) {
    // Check for forwarded IP first (for reverse proxies)
    string forwarded = req.get_header("X-Forwarded-For");
    if (!forwarded.empty()) {
        size_t comma = forwarded.find(',');
        return comma != string::npos ? forwarded.substr(0, comma) : forwarded;
    }
    
    // Fallback to direct connection IP
    string real_ip = req.get_header("X-Real-IP");
    if (!real_ip.empty()) {
        return real_ip;
    }
    
    // Default fallback
    return "127.0.0.1";
}

string URLShortenerHandler::get_user_agent(const Request& req) {
    return req.get_header("User-Agent");
}

string URLShortenerHandler::get_referrer(const Request& req) {
    return req.get_header("Referer");
}

unique_ptr<Response> URLShortenerHandler::create_json_response(const string& json, Response::StatusCode status) {
    auto response = make_unique<Response>();
    response->set_status_code(status);
    response->set_header("Content-Type", "application/json");
    response->set_header("Access-Control-Allow-Origin", "*");
    response->set_body(json);
    
    return response;
}

unique_ptr<Response> URLShortenerHandler::create_error_response(const string& message, Response::StatusCode status) {
    JSONBuilder error_json;
    error_json.add("success", false)
              .add("error_message", message);
    
    return create_json_response(error_json.build(), status);
}

unique_ptr<Response> URLShortenerHandler::create_redirect_response(const string& url) {
    auto response = make_unique<Response>();
    response->set_status_code(Response::FOUND);
    response->set_header("Location", url);
    response->set_header("Cache-Control", "no-cache");
    response->set_body("");
    
    return response;
}

unique_ptr<Response> URLShortenerHandler::create_file_response(const string& file_path) {
    try {
        // Check if file exists
        if (!filesystem::exists(file_path)) {
            return create_error_response("File not found", Response::NOT_FOUND);
        }
        
        // Read file content
        ifstream file(file_path, ios::binary);
        if (!file.is_open()) {
            return create_error_response("Cannot read file", Response::INTERNAL_SERVER_ERROR);
        }
        
        // Read file into string
        string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        
        // Determine content type from file extension
        string content_type = "application/octet-stream";
        string extension = filesystem::path(file_path).extension();
        if (extension == ".txt") content_type = "text/plain";
        else if (extension == ".html") content_type = "text/html";
        else if (extension == ".css") content_type = "text/css";
        else if (extension == ".js") content_type = "application/javascript";
        else if (extension == ".json") content_type = "application/json";
        else if (extension == ".png") content_type = "image/png";
        else if (extension == ".jpg" || extension == ".jpeg") content_type = "image/jpeg";
        else if (extension == ".pdf") content_type = "application/pdf";
        
        auto response = make_unique<Response>();
        response->set_status_code(Response::OK);
        response->set_header("Content-Type", content_type);
        response->set_header("Content-Length", to_string(content.size()));
        response->set_header("Content-Disposition", 
                           "attachment; filename=\"" + filesystem::path(file_path).filename().string() + "\"");
        response->set_body(content);
        
        return response;
        
    } catch (const exception& e) {
        cerr << "Exception in create_file_response: " << e.what() << endl;
        return create_error_response("File serving error", Response::INTERNAL_SERVER_ERROR);
    }
}

map<string, string> URLShortenerHandler::parse_form_data(const string& body) {
    return HTTPParser::parse_form_data(body);
}

map<string, string> URLShortenerHandler::parse_multipart_data(const string& body, const string& boundary) {
    map<string, HTTPParser::MultipartFile> files;
    auto form_fields = HTTPParser::parse_multipart_form(body, boundary, files);
    
    // Store files for later use - implementation depends on file handling strategy
    return form_fields;
}

optional<URLShortenerHandler::FileUploadData> URLShortenerHandler::extract_uploaded_file(const Request& req) {
    // Get Content-Type header
    string content_type = req.get_header("Content-Type");
    string boundary = HTTPParser::extract_boundary(content_type);
    
    if (boundary.empty()) {
        return nullopt;
    }
    
    // Parse multipart data
    map<string, HTTPParser::MultipartFile> files;
    HTTPParser::parse_multipart_form(req.body(), boundary, files);
    
    // Find the file field (usually named "file")
    auto file_it = files.find("file");
    if (file_it == files.end()) {
        return nullopt;
    }
    
    FileUploadData upload_data;
    upload_data.filename = file_it->second.filename;
    upload_data.data = file_it->second.data;
    upload_data.content_type = file_it->second.content_type;
    
    return upload_data;
}

unique_ptr<RequestHandler> URLShortenerHandler::Create(const vector<string>& args) {
    if (args.size() < 4) {
        cerr << "URLShortenerHandler requires 4 arguments: "
             << "serving_path upload_dir db_path base_url" << endl;
        return nullptr;
    }
    
    string serving_path = args[0];
    string upload_dir = args[1]; 
    string db_path = args[2];
    string base_url = args[3];
    
    return make_unique<URLShortenerHandler>(serving_path, upload_dir, db_path, base_url);
}

// Static registration block
namespace {
    const bool url_shortener_handler_registered = HandlerRegistry::RegisterHandler("URLShortenerHandler", URLShortenerHandler::Create);
} 