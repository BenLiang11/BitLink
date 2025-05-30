#include "services/file_upload_service.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <sstream>

FileUploadService::FileUploadService(unique_ptr<DatabaseInterface> db,
                                   const string& upload_dir,
                                   const string& base_url)
    : db_(move(db)), upload_dir_(upload_dir), base_url_(base_url) {
    ensure_upload_directory_exists();
}

FileUploadService::UploadResult FileUploadService::upload_file(const string& filename,
                                                              const vector<uint8_t>& file_data) {
    UploadResult result;
    
    // Validate file type and content
    auto validation_result = FileValidator::validate_file_type(filename, file_data);
    if (!validation_result.is_valid) {
        result.error_message = validation_result.error_message;
        return result;
    }
    
    // Check file size
    if (!FileValidator::is_within_size_limit(file_data.size())) {
        result.error_message = "File size exceeds maximum limit (25MB)";
        return result;
    }
    
    // Sanitize filename
    string sanitized_filename = FileValidator::sanitize_filename(filename);
    if (sanitized_filename.empty()) {
        result.error_message = "Invalid filename";
        return result;
    }
    
    // Generate unique code
    string code = SlugGenerator::generate_unique_code(db_.get(), 8, 100);
    if (code.empty()) {
        result.error_message = "Failed to generate unique code";
        return result;
    }
    
    // Generate file path
    string file_path = generate_file_path(code, sanitized_filename);
    
    // Write file to disk
    if (!write_file_to_disk(file_path, file_data)) {
        result.error_message = "Failed to write file to disk";
        return result;
    }
    
    // Insert into database
    if (!db_->insert_link(code, file_path, "file")) {
        // Clean up file if database insertion fails
        remove_file_from_disk(file_path);
        result.error_message = "Failed to store file record in database";
        return result;
    }
    
    // Build short URL
    string short_url = build_short_url(code);
    
    // Generate QR code
    string qr_data_url;
    try {
        qr_data_url = QRCodeGenerator::generate_data_url(short_url, 200, 
                                                       QRCodeGenerator::ErrorCorrection::MEDIUM);
    } catch (const exception& e) {
        // QR code generation failed, but file upload succeeded
        qr_data_url = "";
    }
    
    result.success = true;
    result.code = code;
    result.short_url = short_url;
    result.qr_code_data_url = qr_data_url;
    result.file_path = file_path;
    result.original_filename = filename;
    
    return result;
}

optional<string> FileUploadService::get_file_path(const string& code) {
    auto link_data = db_->get_link(code);
    if (!link_data) {
        return nullopt;
    }
    
    // Only return file type links
    if (link_data->type == "file") {
        return link_data->destination;
    }
    
    return nullopt;
}

optional<LinkData> FileUploadService::get_file_metadata(const string& code) {
    auto link_data = db_->get_link(code);
    if (!link_data) {
        return nullopt;
    }
    
    // Only return file type links
    if (link_data->type == "file") {
        return link_data;
    }
    
    return nullopt;
}

bool FileUploadService::delete_file(const string& code) {
    // Get file path before deleting from database
    auto file_path = get_file_path(code);
    if (!file_path) {
        return false;
    }
    
    // Delete from database first
    if (!db_->delete_link(code)) {
        return false;
    }
    
    // Remove file from disk
    return remove_file_from_disk(file_path.value());
}

bool FileUploadService::record_file_access(const string& code,
                                          const string& ip_address,
                                          const string& user_agent,
                                          const string& referrer) {
    // Verify the code exists and is a file
    auto link_data = get_file_metadata(code);
    if (!link_data) {
        return false;
    }
    
    // Create click record with privacy-aware IP truncation
    ClickRecord record(code, truncate_ip_for_privacy(ip_address), 
                      user_agent, referrer);
    
    return db_->record_click(record);
}

string FileUploadService::generate_file_path(const string& code, const string& filename) {
    // Create subdirectory based on first two characters of code for organization
    string subdir = code.substr(0, 2);
    string full_dir = upload_dir_ + "/" + subdir;
    
    // Ensure subdirectory exists
    try {
        filesystem::create_directories(full_dir);
    } catch (const exception& e) {
        // Fall back to main upload directory
        full_dir = upload_dir_;
    }
    
    // Generate unique filename: code_originalname
    return full_dir + "/" + code + "_" + filename;
}

bool FileUploadService::ensure_upload_directory_exists() {
    try {
        filesystem::create_directories(upload_dir_);
        return true;
    } catch (const exception& e) {
        return false;
    }
}

string FileUploadService::truncate_ip_for_privacy(const string& ip_address) {
    // For IPv4: remove last octet (192.168.1.xxx -> 192.168.1.0)
    // For IPv6: remove last 64 bits
    
    if (ip_address.empty()) {
        return "0.0.0.0";
    }
    
    // Simple IPv4 handling
    size_t last_dot = ip_address.find_last_of('.');
    if (last_dot != string::npos) {
        return ip_address.substr(0, last_dot) + ".0";
    }
    
    // Simple IPv6 handling - remove after last colon group
    size_t last_colon = ip_address.find_last_of(':');
    if (last_colon != string::npos) {
        return ip_address.substr(0, last_colon) + "::0";
    }
    
    // Fallback for unrecognized format
    return "0.0.0.0";
}

string FileUploadService::build_short_url(const string& code) {
    // Remove trailing slash from base_url if present
    string clean_base = base_url_;
    if (!clean_base.empty() && clean_base.back() == '/') {
        clean_base.pop_back();
    }
    
    return clean_base + "/f/" + code;
}

bool FileUploadService::write_file_to_disk(const string& file_path, const vector<uint8_t>& file_data) {
    try {
        ofstream file(file_path, ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        file.write(reinterpret_cast<const char*>(file_data.data()), file_data.size());
        file.close();
        
        return file.good();
    } catch (const exception& e) {
        return false;
    }
}

bool FileUploadService::remove_file_from_disk(const string& file_path) {
    try {
        return filesystem::remove(file_path);
    } catch (const exception& e) {
        return false;
    }
} 