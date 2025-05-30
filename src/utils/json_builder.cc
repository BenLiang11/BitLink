#include "utils/json_builder.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

using namespace std;

JSONBuilder::JSONBuilder() {
    // Constructor - fields_ is already initialized as empty map
}

JSONBuilder& JSONBuilder::add(const string& key, const string& value) {
    if (!is_valid_field_name(key)) {
        return *this; // Skip invalid field names
    }
    fields_[key] = Field(FieldType::STRING, value);
    return *this;
}

JSONBuilder& JSONBuilder::add(const string& key, const char* value) {
    if (!is_valid_field_name(key)) {
        return *this; // Skip invalid field names
    }
    fields_[key] = Field(FieldType::STRING, string(value));
    return *this;
}

JSONBuilder& JSONBuilder::add(const string& key, int value) {
    if (!is_valid_field_name(key)) {
        return *this;
    }
    fields_[key] = Field(FieldType::INTEGER, to_string(value));
    return *this;
}

JSONBuilder& JSONBuilder::add(const string& key, long value) {
    if (!is_valid_field_name(key)) {
        return *this;
    }
    fields_[key] = Field(FieldType::LONG, to_string(value));
    return *this;
}

JSONBuilder& JSONBuilder::add(const string& key, double value) {
    if (!is_valid_field_name(key)) {
        return *this;
    }
    ostringstream oss;
    oss << fixed << setprecision(6) << value;
    string result = oss.str();
    
    // Remove trailing zeros after decimal point
    if (result.find('.') != string::npos) {
        result = result.substr(0, result.find_last_not_of('0') + 1);
        if (result.back() == '.') {
            result.pop_back();
        }
    }
    
    fields_[key] = Field(FieldType::DOUBLE, result);
    return *this;
}

JSONBuilder& JSONBuilder::add(const string& key, bool value) {
    if (!is_valid_field_name(key)) {
        return *this;
    }
    fields_[key] = Field(FieldType::BOOLEAN, value ? "true" : "false");
    return *this;
}

JSONBuilder& JSONBuilder::add_null(const string& key) {
    if (!is_valid_field_name(key)) {
        return *this;
    }
    fields_[key] = Field(FieldType::NULL_VALUE, "null");
    return *this;
}

JSONBuilder& JSONBuilder::add_array(const string& key, const vector<string>& values) {
    if (!is_valid_field_name(key)) {
        return *this;
    }
    
    ostringstream array_stream;
    array_stream << "[";
    
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) array_stream << ",";
        array_stream << "\"" << escape_json_string(values[i]) << "\"";
    }
    
    array_stream << "]";
    fields_[key] = Field(FieldType::ARRAY, array_stream.str());
    return *this;
}

JSONBuilder& JSONBuilder::add_array(const string& key, const vector<int>& values) {
    if (!is_valid_field_name(key)) {
        return *this;
    }
    
    ostringstream array_stream;
    array_stream << "[";
    
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) array_stream << ",";
        array_stream << values[i];
    }
    
    array_stream << "]";
    fields_[key] = Field(FieldType::ARRAY, array_stream.str());
    return *this;
}

JSONBuilder& JSONBuilder::add_object(const string& key, const map<string, string>& object) {
    if (!is_valid_field_name(key)) {
        return *this;
    }
    
    ostringstream object_stream;
    object_stream << "{";
    
    bool first = true;
    for (const auto& pair : object) {
        if (!first) object_stream << ",";
        object_stream << "\"" << escape_json_string(pair.first) << "\":"
                     << "\"" << escape_json_string(pair.second) << "\"";
        first = false;
    }
    
    object_stream << "}";
    fields_[key] = Field(FieldType::OBJECT, object_stream.str());
    return *this;
}

JSONBuilder& JSONBuilder::add_object(const string& key, const JSONBuilder& builder) {
    if (!is_valid_field_name(key)) {
        return *this;
    }
    
    string object_json = builder.build();
    fields_[key] = Field(FieldType::OBJECT, object_json);
    return *this;
}

JSONBuilder& JSONBuilder::add_raw(const string& key, const string& json_str) {
    if (!is_valid_field_name(key)) {
        return *this;
    }
    
    // Basic validation - check if it looks like valid JSON
    if (is_valid_json(json_str)) {
        fields_[key] = Field(FieldType::RAW, json_str);
    }
    return *this;
}

string JSONBuilder::build(bool pretty_print) const {
    ostringstream json_stream;
    json_stream << "{";
    
    bool first = true;
    for (const auto& pair : fields_) {
        if (!first) {
            json_stream << ",";
            if (pretty_print) json_stream << "\n  ";
        } else if (pretty_print) {
            json_stream << "\n  ";
        }
        
        // Add the key
        json_stream << "\"" << escape_json_string(pair.first) << "\":";
        if (pretty_print) json_stream << " ";
        
        // Add the value based on type
        const Field& field = pair.second;
        switch (field.type) {
            case FieldType::STRING:
                json_stream << "\"" << escape_json_string(field.value) << "\"";
                break;
            case FieldType::INTEGER:
            case FieldType::LONG:
            case FieldType::DOUBLE:
            case FieldType::BOOLEAN:
            case FieldType::NULL_VALUE:
                json_stream << field.value;
                break;
            case FieldType::ARRAY:
            case FieldType::OBJECT:
            case FieldType::RAW:
                json_stream << field.value;
                break;
        }
        
        first = false;
    }
    
    if (pretty_print && !fields_.empty()) {
        json_stream << "\n";
    }
    json_stream << "}";
    
    return json_stream.str();
}

JSONBuilder& JSONBuilder::clear() {
    fields_.clear();
    return *this;
}

bool JSONBuilder::empty() const {
    return fields_.empty();
}

size_t JSONBuilder::size() const {
    return fields_.size();
}

// Static utility methods

string JSONBuilder::success(const string& message) {
    JSONBuilder builder;
    builder.add("success", true)
           .add("message", message);
    return builder.build();
}

string JSONBuilder::error(const string& message, int error_code) {
    JSONBuilder builder;
    builder.add("success", false)
           .add("error", message);
    
    if (error_code != -1) {
        builder.add("error_code", error_code);
    }
    
    return builder.build();
}

string JSONBuilder::data_response(const JSONBuilder& data, bool success) {
    JSONBuilder builder;
    builder.add("success", success)
           .add_object("data", data);
    return builder.build();
}

string JSONBuilder::escape_string(const string& str) {
    JSONBuilder builder; // Just to access the private method
    return builder.escape_json_string(str);
}

bool JSONBuilder::is_valid_json(const string& json_str) {
    if (json_str.empty()) return false;
    
    // Basic structural validation
    string trimmed = json_str;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
    
    if (trimmed.empty()) return false;
    
    // Check for basic JSON structures
    if ((trimmed.front() == '{' && trimmed.back() == '}') ||
        (trimmed.front() == '[' && trimmed.back() == ']') ||
        (trimmed.front() == '"' && trimmed.back() == '"') ||
        trimmed == "true" || trimmed == "false" || trimmed == "null") {
        return true;
    }
    
    // Check if it's a number
    if (isdigit(trimmed.front()) || trimmed.front() == '-') {
        try {
            stod(trimmed);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    return false;
}

// Private methods

string JSONBuilder::escape_json_string(const string& str) const {
    ostringstream escaped;
    
    for (char c : str) {
        switch (c) {
            case '"':
                escaped << "\\\"";
                break;
            case '\\':
                escaped << "\\\\";
                break;
            case '\b':
                escaped << "\\b";
                break;
            case '\f':
                escaped << "\\f";
                break;
            case '\n':
                escaped << "\\n";
                break;
            case '\r':
                escaped << "\\r";
                break;
            case '\t':
                escaped << "\\t";
                break;
            default:
                if (c >= 0 && c < 32) {
                    // Control characters
                    escaped << "\\u" << hex << setw(4) << setfill('0') << static_cast<int>(c);
                } else {
                    escaped << c;
                }
                break;
        }
    }
    
    return escaped.str();
}

string JSONBuilder::format_json(const string& json_str, int indent_size) {
    // Simple JSON formatting implementation
    ostringstream formatted;
    int indent_level = 0;
    bool in_string = false;
    bool escaped = false;
    
    for (size_t i = 0; i < json_str.length(); ++i) {
        char c = json_str[i];
        
        if (in_string) {
            formatted << c;
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_string = false;
            }
            continue;
        }
        
        switch (c) {
            case '"':
                formatted << c;
                in_string = true;
                break;
            case '{':
            case '[':
                formatted << c << "\n";
                indent_level++;
                formatted << string(indent_level * indent_size, ' ');
                break;
            case '}':
            case ']':
                formatted << "\n";
                indent_level--;
                formatted << string(indent_level * indent_size, ' ') << c;
                break;
            case ',':
                formatted << c << "\n" << string(indent_level * indent_size, ' ');
                break;
            case ':':
                formatted << c << " ";
                break;
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                // Skip whitespace
                break;
            default:
                formatted << c;
                break;
        }
    }
    
    return formatted.str();
}

bool JSONBuilder::is_valid_field_name(const string& key) {
    return !key.empty() && key.length() <= 1000; // Basic validation
} 