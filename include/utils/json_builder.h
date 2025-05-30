#ifndef JSON_BUILDER_H
#define JSON_BUILDER_H

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <memory>

/**
 * @brief Simple JSON building utilities
 * 
 * Provides a lightweight JSON builder for creating
 * API responses without external dependencies. Supports
 * nested objects, arrays, and proper JSON escaping.
 */
class JSONBuilder {
public:
    /**
     * @brief Start building a JSON object
     */
    JSONBuilder();
    
    /**
     * @brief Add string field
     * @param key Field name
     * @param value String value
     * @return Reference to this builder for chaining
     */
    JSONBuilder& add(const std::string& key, const std::string& value);
    
    /**
     * @brief Add string field from const char*
     * @param key Field name
     * @param value C-string value
     * @return Reference to this builder for chaining
     */
    JSONBuilder& add(const std::string& key, const char* value);
    
    /**
     * @brief Add integer field
     * @param key Field name
     * @param value Integer value
     * @return Reference to this builder for chaining
     */
    JSONBuilder& add(const std::string& key, int value);
    
    /**
     * @brief Add long integer field
     * @param key Field name
     * @param value Long integer value
     * @return Reference to this builder for chaining
     */
    JSONBuilder& add(const std::string& key, long value);
    
    /**
     * @brief Add double field
     * @param key Field name
     * @param value Double value
     * @return Reference to this builder for chaining
     */
    JSONBuilder& add(const std::string& key, double value);
    
    /**
     * @brief Add boolean field
     * @param key Field name
     * @param value Boolean value
     * @return Reference to this builder for chaining
     */
    JSONBuilder& add(const std::string& key, bool value);
    
    /**
     * @brief Add null field
     * @param key Field name
     * @return Reference to this builder for chaining
     */
    JSONBuilder& add_null(const std::string& key);
    
    /**
     * @brief Add array of strings
     * @param key Field name
     * @param values String array
     * @return Reference to this builder for chaining
     */
    JSONBuilder& add_array(const std::string& key, const std::vector<std::string>& values);
    
    /**
     * @brief Add array of integers
     * @param key Field name
     * @param values Integer array
     * @return Reference to this builder for chaining
     */
    JSONBuilder& add_array(const std::string& key, const std::vector<int>& values);
    
    /**
     * @brief Add nested object
     * @param key Field name
     * @param object Map of key-value pairs
     * @return Reference to this builder for chaining
     */
    JSONBuilder& add_object(const std::string& key, const std::map<std::string, std::string>& object);
    
    /**
     * @brief Add nested JSON builder object
     * @param key Field name
     * @param builder Another JSONBuilder instance
     * @return Reference to this builder for chaining
     */
    JSONBuilder& add_object(const std::string& key, const JSONBuilder& builder);
    
    /**
     * @brief Add raw JSON string (must be valid JSON)
     * @param key Field name
     * @param json_str Raw JSON string
     * @return Reference to this builder for chaining
     */
    JSONBuilder& add_raw(const std::string& key, const std::string& json_str);
    
    /**
     * @brief Build final JSON string
     * @param pretty_print Whether to format with indentation
     * @return JSON string
     */
    std::string build(bool pretty_print = false) const;
    
    /**
     * @brief Clear all fields and start fresh
     * @return Reference to this builder
     */
    JSONBuilder& clear();
    
    /**
     * @brief Check if builder is empty
     * @return true if no fields have been added
     */
    bool empty() const;
    
    /**
     * @brief Get number of fields
     * @return Number of key-value pairs
     */
    size_t size() const;
    
    // Static utility methods
    
    /**
     * @brief Create simple success response
     * @param message Success message (default: "Success")
     * @return JSON string
     */
    static std::string success(const std::string& message = "Success");
    
    /**
     * @brief Create simple error response
     * @param message Error message
     * @param error_code Optional error code
     * @return JSON string
     */
    static std::string error(const std::string& message, int error_code = -1);
    
    /**
     * @brief Create data response with success flag
     * @param data Data object as JSONBuilder
     * @param success Success flag (default: true)
     * @return JSON string
     */
    static std::string data_response(const JSONBuilder& data, bool success = true);
    
    /**
     * @brief Escape a string for JSON encoding
     * @param str String to escape
     * @return JSON-escaped string
     */
    static std::string escape_string(const std::string& str);
    
    /**
     * @brief Validate if a string contains valid JSON
     * @param json_str String to validate
     * @return true if valid JSON structure
     */
    static bool is_valid_json(const std::string& json_str);

private:
    enum class FieldType {
        STRING,
        INTEGER,
        LONG,
        DOUBLE,
        BOOLEAN,
        NULL_VALUE,
        ARRAY,
        OBJECT,
        RAW
    };
    
    struct Field {
        FieldType type;
        std::string value;
        
        Field() : type(FieldType::STRING), value("") {}
        Field(FieldType t, const std::string& v) : type(t), value(v) {}
    };
    
    std::map<std::string, Field> fields_;
    
    /**
     * @brief Escape JSON string with proper character encoding
     * @param str String to escape
     * @return Escaped string
     */
    std::string escape_json_string(const std::string& str) const;
    
    /**
     * @brief Format JSON with indentation
     * @param json_str Raw JSON string
     * @param indent_size Number of spaces per indent level
     * @return Pretty-printed JSON
     */
    std::string format_json(const std::string& json_str, int indent_size = 2);
    
    /**
     * @brief Validate field name for JSON compatibility
     * @param key Field name to validate
     * @return true if valid
     */
    bool is_valid_field_name(const std::string& key);
};

#endif // JSON_BUILDER_H 