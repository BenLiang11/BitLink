#include "utils/url_validator.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

// URL validation regex pattern
// This regex matches: scheme://[user:pass@]host[:port][/path][?query][#fragment]
const regex URLValidator::URL_PATTERN(
    R"(^(https?)://)"                                      // scheme
    R"((?:[a-zA-Z0-9\-._~%!$&'()*+,;=:]+@)?)"             // optional user:pass@
    R"((?:\[[a-fA-F0-9:]+\]|[a-zA-Z0-9\-._~%]+))"         // host (IPv6 in brackets or domain/IPv4)
    R"((?::[0-9]{1,5})?)"                                  // optional :port
    R"((?:/[a-zA-Z0-9\-._~%!$&'()*+,;=:@/]*)?)"           // optional /path
    R"((?:\?[a-zA-Z0-9\-._~%!$&'()*+,;=:@/?]*)?)"         // optional ?query
    R"((?:#[a-zA-Z0-9\-._~%!$&'()*+,;=:@/?]*)?$)",        // optional #fragment
    regex::icase
);

// Blocked domains (example list - can be expanded)
const set<string> URLValidator::BLOCKED_DOMAINS = {
    "localhost",
    "127.0.0.1",
    "0.0.0.0",
    "::1",
    "malware.com",
    "phishing.example",
    "spam.test",
    "blocked.domain",
    "internal.local",
    "private.internal",
    // Add more blocked domains as needed
    "10.",      // Private IP ranges (prefix)
    "192.168.", // Private IP ranges (prefix)
    "172.",     // Private IP ranges (prefix)
};

URLValidator::ValidationResult URLValidator::validate_and_normalize(const string& url) {
    ValidationResult result;
    
    if (url.empty()) {
        result.error_message = "URL cannot be empty";
        return result;
    }
    
    // Check for obviously invalid characters
    for (char c : url) {
        if (!is_valid_url_char(c)) {
            result.error_message = "URL contains invalid characters";
            return result;
        }
    }
    
    // Add scheme if missing
    string url_with_scheme = add_default_scheme(url);
    
    // Normalize the URL
    string normalized = normalize_url(url_with_scheme);
    
    // Validate format
    if (!is_valid_format(normalized)) {
        result.error_message = "Invalid URL format";
        return result;
    }
    
    // Check if scheme is allowed
    if (!is_allowed_scheme(normalized)) {
        result.error_message = "URL scheme not allowed (only http/https)";
        return result;
    }
    
    // Validate port number if present
    size_t domain_start = normalized.find("://") + 3;
    
    // Skip user:pass@ if present
    size_t at_pos = normalized.find('@', domain_start);
    if (at_pos != string::npos) {
        size_t path_start = normalized.find_first_of("/?#", domain_start);
        if (path_start == string::npos || at_pos < path_start) {
            domain_start = at_pos + 1;
        }
    }
    
    // For IPv6, skip to after the closing bracket
    if (domain_start < normalized.length() && normalized[domain_start] == '[') {
        size_t bracket_end = normalized.find(']', domain_start);
        if (bracket_end != string::npos) {
            domain_start = bracket_end + 1;
        }
    }
    
    // Now look for port colon
    size_t port_start = normalized.find(':', domain_start);
    if (port_start != string::npos) {
        size_t port_end = normalized.find_first_of("/?#", port_start);
        if (port_end == string::npos) port_end = normalized.length();
        
        string port_str = normalized.substr(port_start + 1, port_end - port_start - 1);
        if (!port_str.empty()) {
            try {
                int port = stoi(port_str);
                if (port < 1 || port > 65535) {
                    result.error_message = "Invalid port number";
                    return result;
                }
            } catch (...) {
                result.error_message = "Invalid port number";
                return result;
            }
        }
    }
    
    // Check for blocked domains
    if (is_blocked_domain(normalized)) {
        result.error_message = "Domain is blocked";
        return result;
    }
    
    // Extract and validate domain
    string domain = extract_domain(normalized);
    if (domain.empty()) {
        result.error_message = "Could not extract valid domain";
        return result;
    }
    
    // Additional domain validation
    if (domain.length() > 253) {
        result.error_message = "Domain name too long";
        return result;
    }
    
    // Check for suspicious patterns
    if (domain.find("..") != string::npos) {
        result.error_message = "Domain contains invalid patterns";
        return result;
    }
    
    // Validate IPv4 format if it's an IP address
    if (is_ip_address(domain) && domain.find(':') == string::npos) {
        if (!is_valid_ip(domain)) {
            result.error_message = "Invalid IP address format";
            return result;
        }
    }
    
    result.is_valid = true;
    result.normalized_url = normalized;
    return result;
}

bool URLValidator::is_allowed_scheme(const string& url) {
    if (url.empty()) return false;
    
    string lower_url = url;
    transform(lower_url.begin(), lower_url.end(), lower_url.begin(), ::tolower);
    
    return lower_url.substr(0, 7) == "http://" || 
           lower_url.substr(0, 8) == "https://";
}

bool URLValidator::is_blocked_domain(const string& url) {
    string domain = extract_domain(url);
    if (domain.empty()) return true;
    
    string lower_domain = normalize_domain(domain);
    
    // Check exact matches
    if (BLOCKED_DOMAINS.find(lower_domain) != BLOCKED_DOMAINS.end()) {
        return true;
    }
    
    // Check prefix matches for IP ranges
    for (const string& blocked : BLOCKED_DOMAINS) {
        if (blocked.back() == '.' && lower_domain.length() >= blocked.length() &&
            lower_domain.substr(0, blocked.length()) == blocked) {
            return true;
        }
    }
    
    // Block private IP addresses
    if (is_ip_address(lower_domain)) {
        if (lower_domain.substr(0, 3) == "10." ||
            lower_domain.substr(0, 8) == "192.168." ||
            (lower_domain.substr(0, 4) == "172." && lower_domain.length() > 4)) {
            // Check 172.16-31.x.x range
            size_t dot_pos = lower_domain.find('.', 4);
            if (dot_pos != string::npos) {
                string second_octet = lower_domain.substr(4, dot_pos - 4);
                try {
                    int octet_val = stoi(second_octet);
                    if (octet_val >= 16 && octet_val <= 31) {
                        return true;
                    }
                } catch (...) {
                    // Invalid number, continue
                }
            }
            return true; // Block 10.x and 192.168.x anyway
        }
    }
    
    // Block IPv6 loopback and local addresses
    if (lower_domain == "::1") {
        return true;
    }
    
    // Allow other IPv6 addresses (don't block them)
    if (lower_domain.find(':') != string::npos) {
        return false; // IPv6 addresses are allowed unless they're ::1
    }
    
    // Block localhost-like domains
    if (lower_domain == "localhost" || 
        lower_domain.find(".localhost") != string::npos ||
        lower_domain.find("localhost.") != string::npos) {
        return true;
    }
    
    return false;
}

string URLValidator::extract_domain(const string& url) {
    if (url.empty()) return "";
    
    // Find scheme separator
    size_t scheme_pos = url.find("://");
    if (scheme_pos == string::npos) return "";
    
    size_t start = scheme_pos + 3;
    
    // Skip user:pass@ if present
    size_t at_pos = url.find('@', start);
    if (at_pos != string::npos) {
        // Make sure @ is before any path/query/fragment
        size_t path_start = url.find_first_of("/?#", start);
        if (path_start == string::npos || at_pos < path_start) {
            start = at_pos + 1;
        }
    }
    
    // Handle IPv6 addresses in brackets [address]
    if (start < url.length() && url[start] == '[') {
        size_t bracket_end = url.find(']', start + 1);
        if (bracket_end == string::npos) {
            return ""; // Invalid IPv6 format
        }
        // Extract the IPv6 address without brackets
        return url.substr(start + 1, bracket_end - start - 1);
    }
    
    // For regular domains and IPv4, find the end
    size_t end = url.find_first_of(":/?#", start);
    if (end == string::npos) {
        end = url.length();
    }
    
    string domain = url.substr(start, end - start);
    return normalize_domain(domain);
}

string URLValidator::normalize_url(const string& url) {
    string normalized = url;
    
    // Remove fragment (everything after #)
    size_t fragment_pos = normalized.find('#');
    if (fragment_pos != string::npos) {
        normalized = normalized.substr(0, fragment_pos);
    }
    
    // Convert scheme and domain to lowercase
    size_t scheme_end = normalized.find("://");
    if (scheme_end != string::npos) {
        string scheme = normalized.substr(0, scheme_end);
        transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
        
        size_t domain_start = scheme_end + 3;
        
        // Skip user:pass@ if present
        size_t at_pos = normalized.find('@', domain_start);
        if (at_pos != string::npos) {
            size_t slash_pos = normalized.find('/', domain_start);
            if (slash_pos == string::npos || at_pos < slash_pos) {
                domain_start = at_pos + 1;
            }
        }
        
        size_t domain_end = normalized.find_first_of(":/?", domain_start);
        if (domain_end == string::npos) {
            domain_end = normalized.length();
        }
        
        string domain = normalized.substr(domain_start, domain_end - domain_start);
        
        // Handle IPv6 in brackets separately
        if (!domain.empty() && domain[0] == '[') {
            size_t bracket_end = domain.find(']');
            if (bracket_end != string::npos) {
                string ipv6_part = domain.substr(1, bracket_end - 1);
                transform(ipv6_part.begin(), ipv6_part.end(), ipv6_part.begin(), ::tolower);
                domain = "[" + ipv6_part + "]" + domain.substr(bracket_end + 1);
            }
        } else {
            // Regular domain normalization
            transform(domain.begin(), domain.end(), domain.begin(), ::tolower);
            
            // Remove trailing dot
            if (!domain.empty() && domain.back() == '.') {
                domain.pop_back();
            }
        }
        
        normalized = scheme + "://" + normalized.substr(scheme_end + 3, domain_start - scheme_end - 3) + 
                    domain + normalized.substr(domain_end);
    }
    
    // Remove default ports
    normalized = remove_default_port(normalized);
    
    // Ensure path starts with /
    size_t path_start = normalized.find('/', normalized.find("://") + 3);
    if (path_start == string::npos) {
        // No path, add trailing slash
        size_t query_start = normalized.find('?');
        if (query_start == string::npos) {
            normalized += "/";
        } else {
            normalized.insert(query_start, "/");
        }
    }
    
    return normalized;
}

string URLValidator::add_default_scheme(const string& url) {
    if (url.empty()) return url;
    
    // Check if scheme is already present
    if (url.find("://") != string::npos) {
        return url;
    }
    
    // Add https as default
    return "https://" + url;
}

bool URLValidator::is_valid_format(const string& url) {
    return regex_match(url, URL_PATTERN);
}

string URLValidator::normalize_domain(const string& domain) {
    string normalized = domain;
    transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove trailing dot
    if (!normalized.empty() && normalized.back() == '.') {
        normalized.pop_back();
    }
    
    return normalized;
}

bool URLValidator::is_ip_address(const string& domain) {
    // Simple check for IPv4 pattern (numbers and dots)
    if (domain.empty()) return false;
    
    // IPv6 (contains colons)
    if (domain.find(':') != string::npos) {
        return true;
    }
    
    // IPv4 (only digits, dots, and exactly 3 dots)
    int dot_count = 0;
    for (char c : domain) {
        if (c == '.') {
            dot_count++;
        } else if (!isdigit(c)) {
            return false;
        }
    }
    
    return dot_count == 3;
}

bool URLValidator::is_valid_ip(const string& ip) {
    if (ip.empty()) return false;
    
    // IPv6 (simplified check)
    if (ip.find(':') != string::npos) {
        // Basic IPv6 validation - should have colons and valid hex characters
        if (ip.length() < 2 || ip.length() > 39) return false;
        for (char c : ip) {
            if (!isdigit(c) && !isxdigit(c) && c != ':') {
                return false;
            }
        }
        return true;
    }
    
    // IPv4 validation
    istringstream iss(ip);
    string octet;
    int count = 0;
    
    while (getline(iss, octet, '.')) {
        if (octet.empty() || octet.length() > 3) return false;
        
        // Check all characters are digits
        for (char c : octet) {
            if (!isdigit(c)) return false;
        }
        
        // No leading zeros (except for "0")
        if (octet.length() > 1 && octet[0] == '0') return false;
        
        try {
            int value = stoi(octet);
            if (value < 0 || value > 255) return false;
        } catch (...) {
            return false;
        }
        
        count++;
    }
    
    return count == 4;
}

string URLValidator::remove_default_port(const string& url) {
    size_t scheme_end = url.find("://");
    if (scheme_end == string::npos) return url;
    
    string scheme = url.substr(0, scheme_end);
    transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
    
    // Find port
    size_t domain_start = scheme_end + 3;
    size_t at_pos = url.find('@', domain_start);
    if (at_pos != string::npos) {
        domain_start = at_pos + 1;
    }
    
    size_t port_start = url.find(':', domain_start);
    if (port_start == string::npos) return url;
    
    size_t port_end = url.find_first_of("/?#", port_start);
    if (port_end == string::npos) port_end = url.length();
    
    string port = url.substr(port_start + 1, port_end - port_start - 1);
    
    // Check if it's a default port
    bool is_default = false;
    if (scheme == "http" && port == "80") is_default = true;
    if (scheme == "https" && port == "443") is_default = true;
    
    if (is_default) {
        return url.substr(0, port_start) + url.substr(port_end);
    }
    
    return url;
}

string URLValidator::url_decode(const string& encoded) {
    string decoded;
    decoded.reserve(encoded.length());
    
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            string hex = encoded.substr(i + 1, 2);
            char* end;
            long value = strtol(hex.c_str(), &end, 16);
            if (end == hex.c_str() + 2) {
                decoded += static_cast<char>(value);
                i += 2;
            } else {
                decoded += encoded[i];
            }
        } else if (encoded[i] == '+') {
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }
    
    return decoded;
}

bool URLValidator::is_valid_url_char(char c) {
    // Allow standard ASCII characters used in URLs, but be more restrictive
    if ((c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9')) {
        return true;
    }
    
    // Allow specific URL-safe characters, but exclude dangerous ones like < > 
    switch (c) {
        case '-': case '.': case '_': case '~':
        case ':': case '/': case '?': case '#':
        case '[': case ']': case '@': case '!':
        case '$': case '&': case '\'': case '(':
        case ')': case '*': case '+': case ',':
        case ';': case '=': case '%':
            return true;
        // Explicitly reject dangerous characters
        case '<': case '>': case '"': case '\\':
        case '{': case '}': case '|': case '^':
        case '`': case ' ':
            return false;
        default:
            // Reject control characters (< 32) and DEL character (127)
            // Only allow printable ASCII characters that we haven't explicitly listed
            return c >= 32 && c < 127;
    }
} 