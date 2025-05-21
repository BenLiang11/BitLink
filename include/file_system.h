#include <string>
#include <sstream>

#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

class FileSystem {
    public:
        virtual int read_file(const std::string& file_path, std::stringstream& out_file_data) = 0;
        virtual int overwrite_file(const std::string& file_path, const std::stringstream& in_file_data) = 0;
        virtual bool exists(const std::string& file_path) = 0;
        virtual bool is_directory(const std::string& file_path) = 0;
        virtual void get_json_list_of_dir(const std::string& base_file_path, std::string& response_body) = 0;
        virtual bool is_regular_file(const std::string& base_file_path) = 0;
        virtual bool remove(const std::string& base_file_path) = 0;

};

#endif