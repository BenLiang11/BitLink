#include <gtest/gtest.h>
#include "utils/http_parser.h"
#include <string>
#include <map>
#include <vector>

using namespace std;

/**
 * @brief Test fixture for HTTPParser tests
 * 
 * Provides common setup and utility methods for all HTTP parser tests.
 */
class HTTPParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
    
    void TearDown() override {
        // Cleanup code if needed
    }
    
    /**
     * @brief Helper to create test multipart body
     */
    string create_multipart_body(const string& boundary, 
                                const vector<pair<string, string>>& fields,
                                const vector<tuple<string, string, string, string>>& files = {}) {
        string body;
        
        // Add regular fields
        for (const auto& field : fields) {
            body += "--" + boundary + "\r\n";
            body += "Content-Disposition: form-data; name=\"" + field.first + "\"\r\n";
            body += "\r\n";
            body += field.second + "\r\n";
        }
        
        // Add file fields
        for (const auto& file : files) {
            body += "--" + boundary + "\r\n";
            body += "Content-Disposition: form-data; name=\"" + get<0>(file) + "\"; filename=\"" + get<1>(file) + "\"\r\n";
            body += "Content-Type: " + get<2>(file) + "\r\n";
            body += "\r\n";
            body += get<3>(file) + "\r\n";
        }
        
        // Add final boundary
        body += "--" + boundary + "--\r\n";
        
        return body;
    }
};

// =============================================================================
// Form Data Parsing Tests
// =============================================================================

TEST_F(HTTPParserTest, ParseFormData_EmptyBody_ReturnsEmpty) {
    auto result = HTTPParser::parse_form_data("");
    EXPECT_TRUE(result.empty());
}

TEST_F(HTTPParserTest, ParseFormData_SingleField_Success) {
    auto result = HTTPParser::parse_form_data("name=value");
    
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result["name"], "value");
}

TEST_F(HTTPParserTest, ParseFormData_MultipleFields_Success) {
    auto result = HTTPParser::parse_form_data("name=John&age=25&city=NYC");
    
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result["name"], "John");
    EXPECT_EQ(result["age"], "25");
    EXPECT_EQ(result["city"], "NYC");
}

TEST_F(HTTPParserTest, ParseFormData_URLEncodedValues_DecodesCorrectly) {
    auto result = HTTPParser::parse_form_data("message=Hello%20World&email=test%40example.com");
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result["message"], "Hello World");
    EXPECT_EQ(result["email"], "test@example.com");
}

TEST_F(HTTPParserTest, ParseFormData_PlusSignInValues_ConvertedToSpace) {
    auto result = HTTPParser::parse_form_data("query=hello+world");
    
    EXPECT_EQ(result["query"], "hello world");
}

TEST_F(HTTPParserTest, ParseFormData_EmptyValues_Handled) {
    auto result = HTTPParser::parse_form_data("name=&value=test&empty=");
    
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result["name"], "");
    EXPECT_EQ(result["value"], "test");
    EXPECT_EQ(result["empty"], "");
}

TEST_F(HTTPParserTest, ParseFormData_NoEquals_TreatedAsKeyWithEmptyValue) {
    auto result = HTTPParser::parse_form_data("standalone&name=value");
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result["standalone"], "");
    EXPECT_EQ(result["name"], "value");
}

TEST_F(HTTPParserTest, ParseFormData_SpecialCharacters_HandledCorrectly) {
    auto result = HTTPParser::parse_form_data("symbols=%21%40%23%24%25&unicode=%C2%A9");
    
    EXPECT_EQ(result["symbols"], "!@#$%");
    EXPECT_EQ(result["unicode"], "©");
}

// =============================================================================
// URL Encoding/Decoding Tests
// =============================================================================

TEST_F(HTTPParserTest, URLDecode_SimpleText_NoChange) {
    EXPECT_EQ(HTTPParser::url_decode("hello"), "hello");
    EXPECT_EQ(HTTPParser::url_decode("test123"), "test123");
}

TEST_F(HTTPParserTest, URLDecode_PercentEncoded_DecodesCorrectly) {
    EXPECT_EQ(HTTPParser::url_decode("hello%20world"), "hello world");
    EXPECT_EQ(HTTPParser::url_decode("%21%40%23"), "!@#");
    EXPECT_EQ(HTTPParser::url_decode("%C2%A9"), "©");
}

TEST_F(HTTPParserTest, URLDecode_PlusSign_ConvertedToSpace) {
    EXPECT_EQ(HTTPParser::url_decode("hello+world"), "hello world");
    EXPECT_EQ(HTTPParser::url_decode("a+b+c"), "a b c");
}

TEST_F(HTTPParserTest, URLDecode_InvalidEncoding_KeepsAsIs) {
    EXPECT_EQ(HTTPParser::url_decode("invalid%ZZ"), "invalid%ZZ");
    EXPECT_EQ(HTTPParser::url_decode("incomplete%2"), "incomplete%2");
    EXPECT_EQ(HTTPParser::url_decode("short%"), "short%");
}

TEST_F(HTTPParserTest, URLEncode_SimpleText_NoChange) {
    EXPECT_EQ(HTTPParser::url_encode("hello"), "hello");
    EXPECT_EQ(HTTPParser::url_encode("test123"), "test123");
}

TEST_F(HTTPParserTest, URLEncode_SpecialCharacters_EncodesCorrectly) {
    EXPECT_EQ(HTTPParser::url_encode("hello world"), "hello+world");
    EXPECT_EQ(HTTPParser::url_encode("!@#$%"), "%21%40%23%24%25");
}

TEST_F(HTTPParserTest, URLEncode_SafeCharacters_NotEncoded) {
    EXPECT_EQ(HTTPParser::url_encode("abc-def_123.test~"), "abc-def_123.test~");
}

TEST_F(HTTPParserTest, URLEncode_Unicode_EncodesCorrectly) {
    EXPECT_EQ(HTTPParser::url_encode("©"), "%C2%A9");
}

// =============================================================================
// Boundary Extraction Tests
// =============================================================================

TEST_F(HTTPParserTest, ExtractBoundary_ValidContentType_ExtractsBoundary) {
    string content_type = "multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW";
    string boundary = HTTPParser::extract_boundary(content_type);
    
    EXPECT_EQ(boundary, "----WebKitFormBoundary7MA4YWxkTrZu0gW");
}

TEST_F(HTTPParserTest, ExtractBoundary_QuotedBoundary_ExtractsCorrectly) {
    string content_type = "multipart/form-data; boundary=\"----WebKitFormBoundary7MA4YWxkTrZu0gW\"";
    string boundary = HTTPParser::extract_boundary(content_type);
    
    EXPECT_EQ(boundary, "----WebKitFormBoundary7MA4YWxkTrZu0gW");
}

TEST_F(HTTPParserTest, ExtractBoundary_AdditionalParameters_ExtractsCorrectly) {
    string content_type = "multipart/form-data; charset=utf-8; boundary=test123; other=value";
    string boundary = HTTPParser::extract_boundary(content_type);
    
    EXPECT_EQ(boundary, "test123");
}

TEST_F(HTTPParserTest, ExtractBoundary_NoBoundary_ReturnsEmpty) {
    string content_type = "application/x-www-form-urlencoded";
    string boundary = HTTPParser::extract_boundary(content_type);
    
    EXPECT_TRUE(boundary.empty());
}

TEST_F(HTTPParserTest, ExtractBoundary_EmptyContentType_ReturnsEmpty) {
    string boundary = HTTPParser::extract_boundary("");
    EXPECT_TRUE(boundary.empty());
}

// =============================================================================
// Multipart Form Parsing Tests
// =============================================================================

TEST_F(HTTPParserTest, ParseMultipartForm_EmptyBody_ReturnsEmpty) {
    map<string, HTTPParser::MultipartFile> files;
    auto result = HTTPParser::parse_multipart_form("", "boundary", files);
    
    EXPECT_TRUE(result.empty());
    EXPECT_TRUE(files.empty());
}

TEST_F(HTTPParserTest, ParseMultipartForm_SingleTextField_Success) {
    string boundary = "----WebKitFormBoundary";
    vector<pair<string, string>> fields = {{"name", "John Doe"}};
    string body = create_multipart_body(boundary, fields);
    
    map<string, HTTPParser::MultipartFile> files;
    auto result = HTTPParser::parse_multipart_form(body, boundary, files);
    
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result["name"], "John Doe");
    EXPECT_TRUE(files.empty());
}

TEST_F(HTTPParserTest, ParseMultipartForm_MultipleTextFields_Success) {
    string boundary = "----WebKitFormBoundary";
    vector<pair<string, string>> fields = {
        {"name", "John Doe"},
        {"email", "john@example.com"},
        {"message", "Hello World"}
    };
    string body = create_multipart_body(boundary, fields);
    
    map<string, HTTPParser::MultipartFile> files;
    auto result = HTTPParser::parse_multipart_form(body, boundary, files);
    
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result["name"], "John Doe");
    EXPECT_EQ(result["email"], "john@example.com");
    EXPECT_EQ(result["message"], "Hello World");
    EXPECT_TRUE(files.empty());
}

TEST_F(HTTPParserTest, ParseMultipartForm_SingleFile_Success) {
    string boundary = "----WebKitFormBoundary";
    vector<pair<string, string>> fields = {};
    vector<tuple<string, string, string, string>> files = {
        make_tuple("upload", "test.txt", "text/plain", "file content here")
    };
    string body = create_multipart_body(boundary, fields, files);
    
    map<string, HTTPParser::MultipartFile> parsed_files;
    auto result = HTTPParser::parse_multipart_form(body, boundary, parsed_files);
    
    EXPECT_TRUE(result.empty());
    EXPECT_EQ(parsed_files.size(), 1);
    EXPECT_TRUE(parsed_files.find("upload") != parsed_files.end());
    
    const auto& file = parsed_files["upload"];
    EXPECT_EQ(file.filename, "test.txt");
    EXPECT_EQ(file.content_type, "text/plain");
    
    string file_content(file.data.begin(), file.data.end());
    EXPECT_EQ(file_content, "file content here");
}

TEST_F(HTTPParserTest, ParseMultipartForm_MixedFieldsAndFiles_Success) {
    string boundary = "----WebKitFormBoundary";
    vector<pair<string, string>> fields = {
        {"description", "Test upload"},
        {"category", "documents"}
    };
    vector<tuple<string, string, string, string>> files = {
        make_tuple("file1", "doc1.txt", "text/plain", "Document 1 content"),
        make_tuple("file2", "doc2.txt", "text/plain", "Document 2 content")
    };
    string body = create_multipart_body(boundary, fields, files);
    
    map<string, HTTPParser::MultipartFile> parsed_files;
    auto result = HTTPParser::parse_multipart_form(body, boundary, parsed_files);
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result["description"], "Test upload");
    EXPECT_EQ(result["category"], "documents");
    
    EXPECT_EQ(parsed_files.size(), 2);
    EXPECT_TRUE(parsed_files.find("file1") != parsed_files.end());
    EXPECT_TRUE(parsed_files.find("file2") != parsed_files.end());
    
    EXPECT_EQ(parsed_files["file1"].filename, "doc1.txt");
    EXPECT_EQ(parsed_files["file2"].filename, "doc2.txt");
}

TEST_F(HTTPParserTest, ParseMultipartForm_BinaryFile_HandlesCorrectly) {
    string boundary = "----WebKitFormBoundary";
    
    // Create binary data
    vector<uint8_t> binary_data = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}; // PNG header
    string binary_string(binary_data.begin(), binary_data.end());
    
    vector<tuple<string, string, string, string>> files = {
        make_tuple("image", "test.png", "image/png", binary_string)
    };
    string body = create_multipart_body(boundary, {}, files);
    
    map<string, HTTPParser::MultipartFile> parsed_files;
    auto result = HTTPParser::parse_multipart_form(body, boundary, parsed_files);
    
    EXPECT_EQ(parsed_files.size(), 1);
    const auto& file = parsed_files["image"];
    EXPECT_EQ(file.filename, "test.png");
    EXPECT_EQ(file.content_type, "image/png");
    EXPECT_EQ(file.data, binary_data);
}

// =============================================================================
// Edge Cases and Error Handling Tests
// =============================================================================

TEST_F(HTTPParserTest, ParseMultipartForm_InvalidBoundary_ReturnsEmpty) {
    string body = "some content without proper boundary";
    map<string, HTTPParser::MultipartFile> files;
    auto result = HTTPParser::parse_multipart_form(body, "nonexistent", files);
    
    EXPECT_TRUE(result.empty());
    EXPECT_TRUE(files.empty());
}

TEST_F(HTTPParserTest, ParseMultipartForm_MalformedContent_HandlesGracefully) {
    string body = "--boundary\r\nmalformed content without proper headers\r\n--boundary--";
    map<string, HTTPParser::MultipartFile> files;
    auto result = HTTPParser::parse_multipart_form(body, "boundary", files);
    
    // Should not crash and return empty results
    EXPECT_TRUE(result.empty());
    EXPECT_TRUE(files.empty());
}

TEST_F(HTTPParserTest, ParseFormData_ExtremeLongField_HandlesCorrectly) {
    string long_value(10000, 'A'); // 10KB value
    string form_data = "field=" + HTTPParser::url_encode(long_value);
    
    auto result = HTTPParser::parse_form_data(form_data);
    EXPECT_EQ(result["field"], long_value);
}

// =============================================================================
// Helper Function Tests
// =============================================================================

TEST_F(HTTPParserTest, HexToInt_ValidHexDigits_ConvertsCorrectly) {
    // This tests the private function indirectly through url_decode
    EXPECT_EQ(HTTPParser::url_decode("%41"), "A"); // 0x41 = 'A'
    EXPECT_EQ(HTTPParser::url_decode("%2D"), "-"); // 0x2D = '-'
    EXPECT_EQ(HTTPParser::url_decode("%7e"), "~"); // 0x7E = '~' (lowercase hex)
}

TEST_F(HTTPParserTest, Trim_VariousWhitespace_TrimsCorrectly) {
    // Test indirectly through boundary extraction
    string content_type = "multipart/form-data; boundary=  test123  ";
    string boundary = HTTPParser::extract_boundary(content_type);
    EXPECT_EQ(boundary, "test123");
}

// =============================================================================
// Security and Boundary Tests
// =============================================================================

TEST_F(HTTPParserTest, ParseMultipartForm_OversizedFile_Ignored) {
    string boundary = "----WebKitFormBoundary";
    
    // Create file larger than MAX_FILE_SIZE (25MB)
    string large_content(26 * 1024 * 1024, 'A'); // 26MB
    vector<tuple<string, string, string, string>> files = {
        make_tuple("largefile", "huge.txt", "text/plain", large_content)
    };
    string body = create_multipart_body(boundary, {}, files);
    
    map<string, HTTPParser::MultipartFile> parsed_files;
    auto result = HTTPParser::parse_multipart_form(body, boundary, parsed_files);
    
    // File should be ignored due to size limit
    EXPECT_TRUE(parsed_files.empty());
}

TEST_F(HTTPParserTest, ParseMultipartForm_OversizedField_Ignored) {
    string boundary = "----WebKitFormBoundary";
    
    // Create field larger than MAX_FIELD_SIZE (1MB)
    string large_field(2 * 1024 * 1024, 'B'); // 2MB
    vector<pair<string, string>> fields = {{"largefield", large_field}};
    string body = create_multipart_body(boundary, fields);
    
    map<string, HTTPParser::MultipartFile> files;
    auto result = HTTPParser::parse_multipart_form(body, boundary, files);
    
    // Field should be ignored due to size limit
    EXPECT_TRUE(result.empty());
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST_F(HTTPParserTest, FullWorkflow_FormSubmissionWithFile_ParsesCompletely) {
    // Simulate a complete form submission with both text fields and file upload
    string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    
    vector<pair<string, string>> fields = {
        {"title", "My Document"},
        {"description", "This is a test document with special chars: !@#$%"},
        {"tags", "test,document,upload"}
    };
    
    vector<tuple<string, string, string, string>> files = {
        make_tuple("document", "report.pdf", "application/pdf", "PDF content here")
    };
    
    string body = create_multipart_body(boundary, fields, files);
    
    map<string, HTTPParser::MultipartFile> parsed_files;
    auto parsed_fields = HTTPParser::parse_multipart_form(body, boundary, parsed_files);
    
    // Verify all fields parsed correctly
    EXPECT_EQ(parsed_fields.size(), 3);
    EXPECT_EQ(parsed_fields["title"], "My Document");
    EXPECT_EQ(parsed_fields["description"], "This is a test document with special chars: !@#$%");
    EXPECT_EQ(parsed_fields["tags"], "test,document,upload");
    
    // Verify file parsed correctly
    EXPECT_EQ(parsed_files.size(), 1);
    const auto& file = parsed_files["document"];
    EXPECT_EQ(file.filename, "report.pdf");
    EXPECT_EQ(file.content_type, "application/pdf");
    
    string file_content(file.data.begin(), file.data.end());
    EXPECT_EQ(file_content, "PDF content here");
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(HTTPParserTest, Performance_LargeFormData_ProcessesEfficiently) {
    // Create form data with many fields
    string form_data;
    for (int i = 0; i < 1000; ++i) {
        if (!form_data.empty()) form_data += "&";
        form_data += "field" + to_string(i) + "=value" + to_string(i);
    }
    
    auto start = chrono::high_resolution_clock::now();
    auto result = HTTPParser::parse_form_data(form_data);
    auto end = chrono::high_resolution_clock::now();
    
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    EXPECT_EQ(result.size(), 1000);
    EXPECT_LT(duration.count(), 100); // Should complete within 100ms
    
    // Verify some values
    EXPECT_EQ(result["field0"], "value0");
    EXPECT_EQ(result["field999"], "value999");
} 