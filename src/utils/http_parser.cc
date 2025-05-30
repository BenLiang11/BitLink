#include "utils/http_parser.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

using namespace std;

// Constants
const size_t HTTPParser::MAX_FIELD_SIZE;
const size_t HTTPParser::MAX_FILE_SIZE;

map<string, string> HTTPParser::parse_form_data(const string& body) {
    map<string, string> result;
    
    if (body.empty()) {
        return result;
    }
    
    // Split by & to get key-value pairs
    stringstream ss(body);
    string pair;
    
    while (getline(ss, pair, '&')) {
        if (pair.empty()) continue;
        
        // Find the = separator
        size_t eq_pos = pair.find('=');
        if (eq_pos == string::npos) {
            // No value, treat as key with empty value
            string key = url_decode(pair);
            if (!key.empty()) {
                result[key] = "";
            }
            continue;
        }
        
        string key = url_decode(pair.substr(0, eq_pos));
        string value = url_decode(pair.substr(eq_pos + 1));
        
        if (!key.empty()) {
            result[key] = value;
        }
    }
    
    return result;
}

map<string, string> HTTPParser::parse_multipart_form(
    const string& body,
    const string& boundary,
    map<string, MultipartFile>& files) {
    
    map<string, string> result;
    files.clear();
    
    if (body.empty() || boundary.empty()) {
        return result;
    }
    
    // Construct boundary markers
    string start_boundary = "--" + boundary;
    string end_boundary = "--" + boundary + "--";
    
    size_t pos = 0;
    
    // Find first boundary
    pos = body.find(start_boundary, pos);
    if (pos == string::npos) {
        return result;
    }
    
    pos += start_boundary.length();
    
    while (pos < body.length()) {
        // Skip CRLF after boundary
        if (pos < body.length() && body[pos] == '\r') pos++;
        if (pos < body.length() && body[pos] == '\n') pos++;
        
        // Find end of headers (empty line)
        size_t headers_end = body.find("\r\n\r\n", pos);
        if (headers_end == string::npos) {
            headers_end = body.find("\n\n", pos);
            if (headers_end == string::npos) break;
            headers_end += 2;
        } else {
            headers_end += 4;
        }
        
        // Extract headers
        string headers = body.substr(pos, headers_end - pos - 2);
        pos = headers_end;
        
        // Find next boundary
        size_t next_boundary = body.find(start_boundary, pos);
        if (next_boundary == string::npos) {
            // Look for end boundary
            next_boundary = body.find(end_boundary, pos);
            if (next_boundary == string::npos) break;
        }
        
        // Extract content (remove trailing CRLF before boundary)
        string content = body.substr(pos, next_boundary - pos);
        if (content.length() >= 2 && content.substr(content.length() - 2) == "\r\n") {
            content = content.substr(0, content.length() - 2);
        } else if (!content.empty() && content.back() == '\n') {
            content = content.substr(0, content.length() - 1);
        }
        
        // Parse headers to extract field name and filename
        string field_name, filename, content_type;
        
        // Parse Content-Disposition header
        size_t cd_pos = headers.find("Content-Disposition:");
        if (cd_pos != string::npos) {
            size_t cd_end = headers.find('\n', cd_pos);
            string cd_header = headers.substr(cd_pos, cd_end - cd_pos);
            parse_content_disposition(cd_header, field_name, filename);
        }
        
        // Parse Content-Type header
        size_t ct_pos = headers.find("Content-Type:");
        if (ct_pos != string::npos) {
            size_t ct_end = headers.find('\n', ct_pos);
            string ct_header = headers.substr(ct_pos + 13, ct_end - ct_pos - 13);
            content_type = trim(ct_header);
        }
        
        if (!field_name.empty()) {
            if (!filename.empty()) {
                // This is a file field
                vector<uint8_t> file_data(content.begin(), content.end());
                
                // Check file size limit
                if (file_data.size() <= MAX_FILE_SIZE) {
                    files[field_name] = MultipartFile(filename, content_type, file_data);
                }
            } else {
                // This is a regular field
                if (content.length() <= MAX_FIELD_SIZE) {
                    result[field_name] = content;
                }
            }
        }
        
        // Move to next part
        pos = next_boundary;
        if (body.substr(pos, end_boundary.length()) == end_boundary) {
            break; // Reached final boundary
        }
        pos += start_boundary.length();
    }
    
    return result;
}

string HTTPParser::extract_boundary(const string& content_type) {
    if (content_type.empty()) {
        return "";
    }
    
    // Look for boundary parameter
    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == string::npos) {
        return "";
    }
    
    boundary_pos += 9; // Length of "boundary="
    
    // Extract boundary value (may be quoted)
    string boundary_value;
    if (boundary_pos < content_type.length()) {
        if (content_type[boundary_pos] == '"') {
            // Quoted boundary
            boundary_pos++; // Skip opening quote
            size_t end_quote = content_type.find('"', boundary_pos);
            if (end_quote != string::npos) {
                boundary_value = content_type.substr(boundary_pos, end_quote - boundary_pos);
            }
        } else {
            // Unquoted boundary (ends at semicolon or end of string)
            size_t end_pos = content_type.find(';', boundary_pos);
            if (end_pos == string::npos) {
                end_pos = content_type.length();
            }
            boundary_value = content_type.substr(boundary_pos, end_pos - boundary_pos);
            boundary_value = trim(boundary_value);
        }
    }
    
    return boundary_value;
}

string HTTPParser::url_decode(const string& encoded) {
    string decoded;
    decoded.reserve(encoded.length());
    
    for (size_t i = 0; i < encoded.length(); ++i) {
        char c = encoded[i];
        
        if (c == '%' && i + 2 < encoded.length()) {
            // URL-encoded character
            int hex1 = hex_to_int(encoded[i + 1]);
            int hex2 = hex_to_int(encoded[i + 2]);
            
            if (hex1 != -1 && hex2 != -1) {
                decoded += static_cast<char>((hex1 << 4) | hex2);
                i += 2; // Skip the two hex digits
            } else {
                decoded += c; // Invalid encoding, keep as-is
            }
        } else if (c == '+') {
            // Plus sign represents space in form data
            decoded += ' ';
        } else {
            decoded += c;
        }
    }
    
    return decoded;
}

string HTTPParser::url_encode(const string& decoded) {
    ostringstream encoded;
    encoded << hex << uppercase;
    
    for (char c : decoded) {
        // Check if character needs encoding
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else if (c == ' ') {
            encoded << '+'; // Space encoded as plus in form data
        } else {
            // Encode as %XX - cast to unsigned int for proper hex formatting
            encoded << '%' << setw(2) << setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(c));
        }
    }
    
    return encoded.str();
}

int HTTPParser::hex_to_int(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return -1; // Invalid hex character
}

string HTTPParser::trim(const string& str) {
    if (str.empty()) {
        return str;
    }
    
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == string::npos) {
        return ""; // String contains only whitespace
    }
    
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

bool HTTPParser::parse_content_disposition(const string& header, 
                                          string& name, 
                                          string& filename) {
    name.clear();
    filename.clear();
    
    if (header.empty()) {
        return false;
    }
    
    // Find name parameter
    size_t name_pos = header.find("name=");
    if (name_pos != string::npos) {
        name_pos += 5; // Length of "name="
        
        // Extract value (which may be quoted)
        if (name_pos < header.length()) {
            size_t end_pos;
            if (header[name_pos] == '"') {
                // Quoted value
                name_pos++; // Skip opening quote
                end_pos = header.find('"', name_pos);
                if (end_pos != string::npos) {
                    name = header.substr(name_pos, end_pos - name_pos);
                }
            } else {
                // Unquoted value (ends at semicolon or end)
                end_pos = header.find(';', name_pos);
                if (end_pos == string::npos) {
                    end_pos = header.length();
                }
                name = trim(header.substr(name_pos, end_pos - name_pos));
            }
        }
    }
    
    // Find filename parameter
    size_t filename_pos = header.find("filename=");
    if (filename_pos != string::npos) {
        filename_pos += 9; // Length of "filename="
        
        // Extract value (which may be quoted)
        if (filename_pos < header.length()) {
            size_t end_pos;
            if (header[filename_pos] == '"') {
                // Quoted value
                filename_pos++; // Skip opening quote
                end_pos = header.find('"', filename_pos);
                if (end_pos != string::npos) {
                    filename = header.substr(filename_pos, end_pos - filename_pos);
                }
            } else {
                // Unquoted value (ends at semicolon or end)
                end_pos = header.find(';', filename_pos);
                if (end_pos == string::npos) {
                    end_pos = header.length();
                }
                filename = trim(header.substr(filename_pos, end_pos - filename_pos));
            }
        }
    }
    
    return !name.empty();
}

string HTTPParser::extract_quoted_value(const string& value) {
    string trimmed = trim(value);
    
    if (trimmed.length() >= 2 && 
        trimmed.front() == '"' && 
        trimmed.back() == '"') {
        return trimmed.substr(1, trimmed.length() - 2);
    }
    
    return trimmed;
} 