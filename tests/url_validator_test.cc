#include <gtest/gtest.h>
#include "utils/url_validator.h"
#include <chrono>
#include <vector>
#include <string>

using namespace std;

/**
 * @brief Test fixture for URLValidator tests
 * 
 * Provides common setup and utility methods for all URL validator tests.
 */
class URLValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
    
    void TearDown() override {
        // Cleanup code if needed
    }
    
    /**
     * @brief Helper to test validation result
     */
    void AssertValidationResult(const URLValidator::ValidationResult& result,
                              bool expected_valid,
                              const string& expected_url = "",
                              const string& expected_error = "") {
        EXPECT_EQ(result.is_valid, expected_valid);
        if (expected_valid) {
            EXPECT_EQ(result.normalized_url, expected_url);
            EXPECT_TRUE(result.error_message.empty());
        } else {
            EXPECT_FALSE(result.error_message.empty());
            if (!expected_error.empty()) {
                EXPECT_EQ(result.error_message, expected_error);
            }
        }
    }
};

// =============================================================================
// ValidationResult Tests
// =============================================================================

TEST_F(URLValidatorTest, ValidationResultDefaultConstructor) {
    URLValidator::ValidationResult result;
    EXPECT_FALSE(result.is_valid);
    EXPECT_TRUE(result.normalized_url.empty());
    EXPECT_TRUE(result.error_message.empty());
}

TEST_F(URLValidatorTest, ValidationResultValidConstructor) {
    URLValidator::ValidationResult result(true, "https://example.com/", "");
    EXPECT_TRUE(result.is_valid);
    EXPECT_EQ(result.normalized_url, "https://example.com/");
    EXPECT_TRUE(result.error_message.empty());
}

TEST_F(URLValidatorTest, ValidationResultInvalidConstructor) {
    URLValidator::ValidationResult result(false, "", "Invalid URL");
    EXPECT_FALSE(result.is_valid);
    EXPECT_TRUE(result.normalized_url.empty());
    EXPECT_EQ(result.error_message, "Invalid URL");
}

// =============================================================================
// Basic URL Validation Tests
// =============================================================================

TEST_F(URLValidatorTest, ValidateEmptyURL) {
    auto result = URLValidator::validate_and_normalize("");
    AssertValidationResult(result, false, "", "URL cannot be empty");
}

TEST_F(URLValidatorTest, ValidateValidHTTPSURL) {
    auto result = URLValidator::validate_and_normalize("https://example.com");
    AssertValidationResult(result, true, "https://example.com/");
}

TEST_F(URLValidatorTest, ValidateValidHTTPURL) {
    auto result = URLValidator::validate_and_normalize("http://example.com");
    AssertValidationResult(result, true, "http://example.com/");
}

TEST_F(URLValidatorTest, ValidateURLWithoutScheme) {
    auto result = URLValidator::validate_and_normalize("example.com");
    AssertValidationResult(result, true, "https://example.com/");
}

TEST_F(URLValidatorTest, ValidateURLWithPath) {
    auto result = URLValidator::validate_and_normalize("https://example.com/path/to/resource");
    AssertValidationResult(result, true, "https://example.com/path/to/resource");
}

TEST_F(URLValidatorTest, ValidateURLWithQuery) {
    auto result = URLValidator::validate_and_normalize("https://example.com/search?q=test&page=1");
    AssertValidationResult(result, true, "https://example.com/search?q=test&page=1");
}

TEST_F(URLValidatorTest, ValidateURLWithFragment) {
    auto result = URLValidator::validate_and_normalize("https://example.com/page#section");
    AssertValidationResult(result, true, "https://example.com/page");
}

TEST_F(URLValidatorTest, ValidateURLWithPort) {
    auto result = URLValidator::validate_and_normalize("https://example.com:8080/path");
    AssertValidationResult(result, true, "https://example.com:8080/path");
}

TEST_F(URLValidatorTest, ValidateURLWithDefaultPort) {
    auto result = URLValidator::validate_and_normalize("https://example.com:443/path");
    AssertValidationResult(result, true, "https://example.com/path");
}

TEST_F(URLValidatorTest, ValidateHTTPDefaultPort) {
    auto result = URLValidator::validate_and_normalize("http://example.com:80/path");
    AssertValidationResult(result, true, "http://example.com/path");
}

// =============================================================================
// URL Normalization Tests
// =============================================================================

TEST_F(URLValidatorTest, NormalizeUppercaseScheme) {
    auto result = URLValidator::validate_and_normalize("HTTPS://EXAMPLE.COM/PATH");
    AssertValidationResult(result, true, "https://example.com/PATH");
}

TEST_F(URLValidatorTest, NormalizeUppercaseDomain) {
    auto result = URLValidator::validate_and_normalize("https://EXAMPLE.COM/path");
    AssertValidationResult(result, true, "https://example.com/path");
}

TEST_F(URLValidatorTest, NormalizeMixedCase) {
    auto result = URLValidator::validate_and_normalize("HtTpS://ExAmPlE.CoM/Path");
    AssertValidationResult(result, true, "https://example.com/Path");
}

TEST_F(URLValidatorTest, NormalizeTrailingSlash) {
    auto result = URLValidator::validate_and_normalize("https://example.com");
    AssertValidationResult(result, true, "https://example.com/");
}

TEST_F(URLValidatorTest, NormalizeWithQuery) {
    auto result = URLValidator::validate_and_normalize("example.com?search=test");
    AssertValidationResult(result, true, "https://example.com/?search=test");
}

// =============================================================================
// Scheme Validation Tests
// =============================================================================

TEST_F(URLValidatorTest, AllowedSchemeHTTP) {
    EXPECT_TRUE(URLValidator::is_allowed_scheme("http://example.com"));
}

TEST_F(URLValidatorTest, AllowedSchemeHTTPS) {
    EXPECT_TRUE(URLValidator::is_allowed_scheme("https://example.com"));
}

TEST_F(URLValidatorTest, AllowedSchemeCaseInsensitive) {
    EXPECT_TRUE(URLValidator::is_allowed_scheme("HTTP://example.com"));
    EXPECT_TRUE(URLValidator::is_allowed_scheme("HTTPS://example.com"));
    EXPECT_TRUE(URLValidator::is_allowed_scheme("HtTp://example.com"));
}

TEST_F(URLValidatorTest, DisallowedSchemeFTP) {
    auto result = URLValidator::validate_and_normalize("ftp://example.com");
    AssertValidationResult(result, false);
}

TEST_F(URLValidatorTest, DisallowedSchemeFile) {
    auto result = URLValidator::validate_and_normalize("file:///etc/passwd");
    AssertValidationResult(result, false);
}

TEST_F(URLValidatorTest, DisallowedSchemeJavaScript) {
    auto result = URLValidator::validate_and_normalize("javascript:alert('xss')");
    AssertValidationResult(result, false);
}

TEST_F(URLValidatorTest, DisallowedSchemeData) {
    auto result = URLValidator::validate_and_normalize("data:text/html,<script>alert('xss')</script>");
    AssertValidationResult(result, false);
}

// =============================================================================
// Domain Extraction Tests
// =============================================================================

TEST_F(URLValidatorTest, ExtractDomainSimple) {
    EXPECT_EQ(URLValidator::extract_domain("https://example.com/path"), "example.com");
}

TEST_F(URLValidatorTest, ExtractDomainWithPort) {
    EXPECT_EQ(URLValidator::extract_domain("https://example.com:8080/path"), "example.com");
}

TEST_F(URLValidatorTest, ExtractDomainWithUserInfo) {
    EXPECT_EQ(URLValidator::extract_domain("https://user:pass@example.com/path"), "example.com");
}

TEST_F(URLValidatorTest, ExtractDomainSubdomain) {
    EXPECT_EQ(URLValidator::extract_domain("https://api.v1.example.com"), "api.v1.example.com");
}

TEST_F(URLValidatorTest, ExtractDomainIPv4) {
    EXPECT_EQ(URLValidator::extract_domain("https://192.168.1.1/path"), "192.168.1.1");
}

TEST_F(URLValidatorTest, ExtractDomainIPv6) {
    EXPECT_EQ(URLValidator::extract_domain("https://[2001:db8::1]/path"), "2001:db8::1");
}

TEST_F(URLValidatorTest, ExtractDomainInvalidURL) {
    EXPECT_TRUE(URLValidator::extract_domain("not-a-url").empty());
    EXPECT_TRUE(URLValidator::extract_domain("").empty());
}

// =============================================================================
// Blocked Domain Tests
// =============================================================================

TEST_F(URLValidatorTest, BlockedDomainLocalhost) {
    auto result = URLValidator::validate_and_normalize("https://localhost/path");
    AssertValidationResult(result, false);
}

TEST_F(URLValidatorTest, BlockedDomainLoopback) {
    auto result = URLValidator::validate_and_normalize("https://127.0.0.1/path");
    AssertValidationResult(result, false);
}

TEST_F(URLValidatorTest, BlockedDomainPrivateIP) {
    auto result = URLValidator::validate_and_normalize("https://192.168.1.1/path");
    AssertValidationResult(result, false);
    
    result = URLValidator::validate_and_normalize("https://10.0.0.1/path");
    AssertValidationResult(result, false);
    
    result = URLValidator::validate_and_normalize("https://172.16.0.1/path");
    AssertValidationResult(result, false);
}

TEST_F(URLValidatorTest, BlockedDomainIPv6Loopback) {
    auto result = URLValidator::validate_and_normalize("https://[::1]/path");
    AssertValidationResult(result, false);
}

TEST_F(URLValidatorTest, BlockedDomainExplicit) {
    auto result = URLValidator::validate_and_normalize("https://malware.com/path");
    AssertValidationResult(result, false);
}

TEST_F(URLValidatorTest, AllowedDomainPublic) {
    auto result = URLValidator::validate_and_normalize("https://google.com");
    AssertValidationResult(result, true, "https://google.com/");
    
    result = URLValidator::validate_and_normalize("https://github.com");
    AssertValidationResult(result, true, "https://github.com/");
}

// =============================================================================
// IP Address Tests
// =============================================================================

TEST_F(URLValidatorTest, ValidPublicIPv4) {
    auto result = URLValidator::validate_and_normalize("https://8.8.8.8/path");
    AssertValidationResult(result, true, "https://8.8.8.8/path");
}

TEST_F(URLValidatorTest, ValidPublicIPv6) {
    auto result = URLValidator::validate_and_normalize("https://[2001:4860:4860::8888]/path");
    AssertValidationResult(result, true, "https://[2001:4860:4860::8888]/path");
}

TEST_F(URLValidatorTest, InvalidIPv4Format) {
    auto result = URLValidator::validate_and_normalize("https://256.256.256.256/path");
    AssertValidationResult(result, false);
}

// =============================================================================
// Invalid Character Tests
// =============================================================================

TEST_F(URLValidatorTest, InvalidCharacters) {
    auto result = URLValidator::validate_and_normalize("https://example.com/<script>");
    AssertValidationResult(result, false, "", "URL contains invalid characters");
    
    string url_with_null = "https://example.com/";
    url_with_null.push_back('\x00');
    url_with_null += "null";
    result = URLValidator::validate_and_normalize(url_with_null);
    AssertValidationResult(result, false, "", "URL contains invalid characters");
    
    result = URLValidator::validate_and_normalize("https://example.com/\x1f\x7f");
    AssertValidationResult(result, false, "", "URL contains invalid characters");
}

// =============================================================================
// URL Format Tests
// =============================================================================

TEST_F(URLValidatorTest, InvalidURLFormat) {
    auto result = URLValidator::validate_and_normalize("not a url at all");
    AssertValidationResult(result, false);
    
    result = URLValidator::validate_and_normalize("https://");
    AssertValidationResult(result, false);
    
    result = URLValidator::validate_and_normalize("://example.com");
    AssertValidationResult(result, false);
}

TEST_F(URLValidatorTest, URLWithSpaces) {
    auto result = URLValidator::validate_and_normalize("https://example.com/path with spaces");
    AssertValidationResult(result, false, "", "URL contains invalid characters");
}

// =============================================================================
// Complex URL Tests
// =============================================================================

TEST_F(URLValidatorTest, ComplexValidURL) {
    auto result = URLValidator::validate_and_normalize(
        "https://user:pass@api.v2.example.com:8080/v1/users/123?include=profile&format=json");
    AssertValidationResult(result, true, 
        "https://user:pass@api.v2.example.com:8080/v1/users/123?include=profile&format=json");
}

TEST_F(URLValidatorTest, URLWithPercentEncoding) {
    auto result = URLValidator::validate_and_normalize("https://example.com/search?q=hello%20world");
    AssertValidationResult(result, true, "https://example.com/search?q=hello%20world");
}

TEST_F(URLValidatorTest, URLWithSpecialCharacters) {
    auto result = URLValidator::validate_and_normalize("https://example.com/path?key=value&other=test");
    AssertValidationResult(result, true, "https://example.com/path?key=value&other=test");
}

// =============================================================================
// Edge Case Tests
// =============================================================================

TEST_F(URLValidatorTest, VeryLongDomain) {
    string long_domain = "https://";
    for (int i = 0; i < 260; i++) {
        long_domain += "a";
    }
    long_domain += ".com";
    
    auto result = URLValidator::validate_and_normalize(long_domain);
    AssertValidationResult(result, false, "", "Domain name too long");
}

TEST_F(URLValidatorTest, DomainWithDoubleDots) {
    auto result = URLValidator::validate_and_normalize("https://example..com/path");
    AssertValidationResult(result, false, "", "Domain contains invalid patterns");
}

TEST_F(URLValidatorTest, DomainWithTrailingDot) {
    auto result = URLValidator::validate_and_normalize("https://example.com./path");
    AssertValidationResult(result, true, "https://example.com/path");
}

// =============================================================================
// Security Tests
// =============================================================================

TEST_F(URLValidatorTest, SecurityXSSAttempt) {
    auto result = URLValidator::validate_and_normalize("javascript:alert('xss')");
    AssertValidationResult(result, false);
}

TEST_F(URLValidatorTest, SecurityDataURI) {
    auto result = URLValidator::validate_and_normalize("data:text/html,<script>alert(1)</script>");
    AssertValidationResult(result, false);
}

TEST_F(URLValidatorTest, SecurityFileAccess) {
    auto result = URLValidator::validate_and_normalize("file:///etc/passwd");
    AssertValidationResult(result, false);
}

TEST_F(URLValidatorTest, SecurityLocalFileAccess) {
    auto result = URLValidator::validate_and_normalize("file://localhost/etc/passwd");
    AssertValidationResult(result, false);
}

TEST_F(URLValidatorTest, SecurityInternalNetworkAccess) {
    // Test various internal network ranges
    vector<string> internal_urls = {
        "https://192.168.0.1/",
        "https://10.0.0.1/",
        "https://172.16.0.1/",
        "https://172.31.255.255/",
        "https://127.0.0.1/",
        "https://localhost/"
    };
    
    for (const string& url : internal_urls) {
        auto result = URLValidator::validate_and_normalize(url);
        EXPECT_FALSE(result.is_valid) << "Internal URL should be blocked: " << url;
    }
}

// =============================================================================
// Unicode and International Domain Tests
// =============================================================================

TEST_F(URLValidatorTest, InternationalDomainName) {
    // Test with punycode (xn--) domains
    auto result = URLValidator::validate_and_normalize("https://xn--nxasmq6b.com/");
    AssertValidationResult(result, true, "https://xn--nxasmq6b.com/");
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(URLValidatorTest, PerformanceTest) {
    vector<string> test_urls = {
        "https://example.com",
        "http://test.org/path",
        "https://api.service.com:8080/v1/endpoint?param=value",
        "invalid-url",
        "https://192.168.1.1",
        "ftp://blocked.com"
    };
    
    auto start = chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; i++) {
        for (const string& url : test_urls) {
            URLValidator::validate_and_normalize(url);
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    // Should complete 6000 validations in reasonable time (less than 1 second)
    EXPECT_LT(duration.count(), 1000) << "Performance test took " << duration.count() << "ms";
}

// =============================================================================
// Individual Method Tests
// =============================================================================

TEST_F(URLValidatorTest, IsAllowedSchemeMethod) {
    EXPECT_TRUE(URLValidator::is_allowed_scheme("http://example.com"));
    EXPECT_TRUE(URLValidator::is_allowed_scheme("https://example.com"));
    EXPECT_FALSE(URLValidator::is_allowed_scheme("ftp://example.com"));
    EXPECT_FALSE(URLValidator::is_allowed_scheme(""));
    EXPECT_FALSE(URLValidator::is_allowed_scheme("invalid"));
}

TEST_F(URLValidatorTest, IsBlockedDomainMethod) {
    EXPECT_TRUE(URLValidator::is_blocked_domain("https://localhost/"));
    EXPECT_TRUE(URLValidator::is_blocked_domain("https://127.0.0.1/"));
    EXPECT_TRUE(URLValidator::is_blocked_domain("https://192.168.1.1/"));
    EXPECT_FALSE(URLValidator::is_blocked_domain("https://google.com/"));
    EXPECT_FALSE(URLValidator::is_blocked_domain("https://github.com/"));
}

// =============================================================================
// Regression Tests
// =============================================================================

TEST_F(URLValidatorTest, RegressionIPv6BracketHandling) {
    auto result = URLValidator::validate_and_normalize("https://[2001:db8::1]:8080/path");
    AssertValidationResult(result, true, "https://[2001:db8::1]:8080/path");
}

TEST_F(URLValidatorTest, RegressionEmptyPathWithQuery) {
    auto result = URLValidator::validate_and_normalize("https://example.com?query=test");
    AssertValidationResult(result, true, "https://example.com/?query=test");
}

TEST_F(URLValidatorTest, RegressionUserInfoHandling) {
    auto result = URLValidator::validate_and_normalize("https://user:password@example.com:8080/path");
    AssertValidationResult(result, true, "https://user:password@example.com:8080/path");
}

TEST_F(URLValidatorTest, RegressionCaseNormalization) {
    auto result = URLValidator::validate_and_normalize("HTTPS://EXAMPLE.COM/PATH");
    AssertValidationResult(result, true, "https://example.com/PATH");
}

// =============================================================================
// Boundary Tests
// =============================================================================

TEST_F(URLValidatorTest, BoundaryMaxPortNumber) {
    auto result = URLValidator::validate_and_normalize("https://example.com:65535/");
    AssertValidationResult(result, true, "https://example.com:65535/");
}

TEST_F(URLValidatorTest, BoundaryInvalidPortNumber) {
    auto result = URLValidator::validate_and_normalize("https://example.com:99999/");
    AssertValidationResult(result, false);
}

TEST_F(URLValidatorTest, BoundaryMinimalValidURL) {
    auto result = URLValidator::validate_and_normalize("a.b");
    AssertValidationResult(result, true, "https://a.b/");
}

TEST_F(URLValidatorTest, BoundaryLongPath) {
    string long_path = "https://example.com/";
    for (int i = 0; i < 100; i++) {
        long_path += "very-long-path-segment/";
    }
    
    auto result = URLValidator::validate_and_normalize(long_path);
    AssertValidationResult(result, true, long_path);
} 