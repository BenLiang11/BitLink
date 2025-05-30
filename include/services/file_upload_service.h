#ifndef FILE_UPLOAD_SERVICE_H
#define FILE_UPLOAD_SERVICE_H

#include "database/database_interface.h"
#include "models/link_data.h"
#include "utils/file_validator.h"
#include "utils/slug_generator.h"
#include "utils/qr_generator.h"
#include <memory>
#include <string>
#include <vector>

using namespace std;

/**
 * @brief Core service for file upload and sharing operations
 * 
 * Handles secure file uploads with validation, virus scanning,
 * and generates shortened links for file access.
 */
class FileUploadService {
public:
    /**
     * @brief Result of file upload operation
     */
    struct UploadResult {
        bool success;
        string code;
        string short_url;
        string qr_code_data_url;
        string file_path;
        string original_filename;
        string error_message;
        
        UploadResult() : success(false) {}
        UploadResult(const string& c, const string& url, 
                    const string& qr, const string& path,
                    const string& filename)
            : success(true), code(c), short_url(url), qr_code_data_url(qr),
              file_path(path), original_filename(filename) {}
    };
    
    /**
     * @brief Constructor
     * @param db Database interface (must be initialized)
     * @param upload_dir Directory for storing uploaded files
     * @param base_url Base URL for shortened links
     */
    FileUploadService(unique_ptr<DatabaseInterface> db,
                     const string& upload_dir,
                     const string& base_url);
    
    /**
     * @brief Upload and process a file
     * @param filename Original filename
     * @param file_data File content as byte vector
     * @return UploadResult with code and details or error
     */
    UploadResult upload_file(const string& filename,
                            const vector<uint8_t>& file_data);
    
    /**
     * @brief Get file path for a shortened code
     * @param code Shortened code
     * @return File path if found, nullopt otherwise
     */
    optional<string> get_file_path(const string& code);
    
    /**
     * @brief Get file metadata for a code
     * @param code Shortened code
     * @return LinkData if found, nullopt otherwise
     */
    optional<LinkData> get_file_metadata(const string& code);
    
    /**
     * @brief Delete an uploaded file and its record
     * @param code Shortened code
     * @return true if deleted successfully
     */
    bool delete_file(const string& code);
    
    /**
     * @brief Record a file access event
     * @param code Shortened code
     * @param ip_address Client IP address
     * @param user_agent User agent string
     * @param referrer Referrer URL (optional)
     * @return true if recorded successfully
     */
    bool record_file_access(const string& code,
                           const string& ip_address,
                           const string& user_agent,
                           const string& referrer = "");

private:
    unique_ptr<DatabaseInterface> db_;
    string upload_dir_;
    string base_url_;
    
    string generate_file_path(const string& code, const string& filename);
    bool ensure_upload_directory_exists();
    string truncate_ip_for_privacy(const string& ip_address);
    string build_short_url(const string& code);
    bool write_file_to_disk(const string& file_path, const vector<uint8_t>& file_data);
    bool remove_file_from_disk(const string& file_path);
};

#endif // FILE_UPLOAD_SERVICE_H 