#include "file_system.h"
#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>

#ifndef FAKE_FILE_SYSTEM_H
#define FAKE_FILE_SYSTEM_H

class FakeFileSystem : public FileSystem {
public:
    // Method to read a file from the real file system
    int read_file(const std::string& file_path, std::stringstream& out_file_data) override;
    int overwrite_file(const std::string& file_path, const std::stringstream& in_file_data) override;
    bool exists(const std::string& file_path) override;
    bool is_directory(const std::string& file_path) override;
    void get_json_list_of_dir(const std::string& base_file_path, std::string& response_body) override;
    bool is_regular_file(const std::string& base_file_path) override;
    bool remove(const std::string& base_file_path) override;
private:
    std::unordered_map<std::string, std::string> fake_files_;
    std::unordered_map<std::string, bool> is_directory_;
    std::unordered_map<std::string, bool> is_regular_file_;
    std::string getDirectory(const std::string& filepath);
};

#endif // FAKE_FILE_SYSTEM_H