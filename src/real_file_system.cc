#include "real_file_system.h"
#include <string>
#include <sstream>
#include <fstream>
#include <ostream>

#include <filesystem>


namespace fs = std::filesystem;

int RealFileSystem::read_file(const std::string& file_path, std::stringstream& out_file_data) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file || !file.is_open()) {
        return -1; // File not found
    }
    out_file_data << file.rdbuf(); // Read the entire file into the string stream
    return out_file_data.str().length(); // Return the length of the content read
}

int RealFileSystem::overwrite_file(const std::string& file_path, const std::stringstream& in_file_data) {
    std::ofstream file(file_path, std::ios::binary);
    file << in_file_data.rdbuf();
    file.close();
    return  0;
}

bool RealFileSystem::exists(const std::string& file_path) {
    return fs::exists(static_cast<fs::path>(file_path));
}

bool RealFileSystem::is_directory(const std::string& file_path) {return fs::is_directory(static_cast<fs::path>(file_path));}

bool RealFileSystem::is_regular_file(const std::string& base_file_path) {return fs::is_regular_file(base_file_path);}

bool RealFileSystem::remove(const std::string& base_file_path) {return fs::remove(base_file_path);}

void RealFileSystem::get_json_list_of_dir(const std::string& base_file_path, std::string& response_body) {
    response_body = "{\"file_ids\": [";
    for (const auto& entry : fs::directory_iterator(base_file_path)) {
        if (fs::is_regular_file(entry.path())) {
            response_body += entry.path().filename().string() + ",";
        }
    }
    response_body.pop_back(); // Remove trailing comma
    response_body += "]}";
}
