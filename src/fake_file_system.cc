#include "fake_file_system.h"
#include <string>
#include <sstream>
#include <fstream>
#include <ostream>
#include <iostream>

std::string FakeFileSystem::getDirectory(const std::string& filepath) {
    size_t pos = filepath.find_last_of("/");
    if (pos == std::string::npos) return ""; // No directory part
    return filepath.substr(0, pos);
}

int FakeFileSystem::read_file(const std::string& file_path, std::stringstream& out_file_data) {
    if (fake_files_.find(file_path) != fake_files_.end()) {
        out_file_data << fake_files_[file_path];
        return fake_files_[file_path].length();
    }
    return -1;
}

int FakeFileSystem::overwrite_file(const std::string& file_path, const std::stringstream& in_file_data) {
    fake_files_[file_path] = in_file_data.str();
    is_regular_file_[file_path] = true;
    is_directory_[getDirectory(file_path)] = true;
    return  0;
}

bool FakeFileSystem::exists(const std::string& file_path) {
    return fake_files_.find(file_path) != fake_files_.end() || is_directory(file_path);
}

bool FakeFileSystem::is_directory(const std::string& file_path) {
    return is_directory_.find(file_path) != is_directory_.end() && is_directory_[file_path];
}

bool FakeFileSystem::is_regular_file(const std::string& base_file_path) {
    return is_regular_file_.find(base_file_path) != is_regular_file_.end() && is_regular_file_[base_file_path];
}

bool FakeFileSystem::remove(const std::string& base_file_path) {
    if (fake_files_.find(base_file_path) == fake_files_.end()) {
        return false; // File not found
    }
    fake_files_.erase(base_file_path);
    is_directory_.erase(base_file_path);
    is_regular_file_.erase(base_file_path);
    return true;
}

void FakeFileSystem::get_json_list_of_dir(const std::string& base_file_path, std::string& response_body) {
    response_body = "{\"file_ids\": [";
    for (const auto& entry : fake_files_) {
        if (is_regular_file(entry.first)) {
            response_body += entry.first + ",";
        }
    }
    response_body.pop_back(); // Remove trailing comma
    response_body += "]}";
}