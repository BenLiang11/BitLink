#include "services/url_shortening_service.h"
#include <algorithm>
#include <sstream>

URLShorteningService::URLShorteningService(unique_ptr<DatabaseInterface> db, 
                                         const string& base_url)
    : db_(move(db)), base_url_(base_url) {
}

URLShorteningService::ShortenResult URLShorteningService::shorten_url(const string& url) {
    ShortenResult result;
    
    // Validate URL
    auto validation_result = URLValidator::validate_and_normalize(url);
    if (!validation_result.is_valid) {
        result.error_message = validation_result.error_message;
        return result;
    }
    
    // Generate unique code
    string code = SlugGenerator::generate_unique_code(db_.get(), 8, 100);
    if (code.empty()) {
        result.error_message = "Failed to generate unique code";
        return result;
    }
    
    // Insert into database
    if (!db_->insert_link(code, validation_result.normalized_url, "url")) {
        result.error_message = "Failed to store shortened URL";
        return result;
    }
    
    // Build short URL
    string short_url = build_short_url(code);
    
    // Generate QR code
    string qr_data_url;
    try {
        qr_data_url = QRCodeGenerator::generate_data_url(short_url, 200, 
                                                       QRCodeGenerator::ErrorCorrection::MEDIUM);
    } catch (const exception& e) {
        // QR code generation failed, but link creation succeeded
        qr_data_url = "";
    }
    
    result.success = true;
    result.code = code;
    result.short_url = short_url;
    result.qr_code_data_url = qr_data_url;
    
    return result;
}

optional<string> URLShorteningService::resolve_code(const string& code) {
    auto link_data = db_->get_link(code);
    if (!link_data) {
        return nullopt;
    }
    
    // Only return URL type links for resolution
    if (link_data->type == "url") {
        return link_data->destination;
    }
    
    return nullopt;
}

bool URLShorteningService::record_access(const string& code, 
                                        const string& ip_address,
                                        const string& user_agent,
                                        const string& referrer) {
    // Verify the code exists
    if (!db_->code_exists(code)) {
        return false;
    }
    
    // Create click record with privacy-aware IP truncation
    ClickRecord record(code, truncate_ip_for_privacy(ip_address), 
                      user_agent, referrer);
    
    return db_->record_click(record);
}

ClickStats URLShorteningService::get_statistics(const string& code) {
    return db_->get_click_stats(code);
}

bool URLShorteningService::delete_link(const string& code) {
    return db_->delete_link(code);
}

string URLShorteningService::truncate_ip_for_privacy(const string& ip_address) {
    // For IPv4: remove last octet (192.168.1.xxx -> 192.168.1.0)
    // For IPv6: remove last 64 bits
    
    if (ip_address.empty()) {
        return "0.0.0.0";
    }
    
    // Simple IPv4 handling
    size_t last_dot = ip_address.find_last_of('.');
    if (last_dot != string::npos) {
        return ip_address.substr(0, last_dot) + ".0";
    }
    
    // Simple IPv6 handling - remove after last colon group
    size_t last_colon = ip_address.find_last_of(':');
    if (last_colon != string::npos) {
        return ip_address.substr(0, last_colon) + "::0";
    }
    
    // Fallback for unrecognized format
    return "0.0.0.0";
}

string URLShorteningService::build_short_url(const string& code) {
    // Remove trailing slash from base_url if present
    string clean_base = base_url_;
    if (!clean_base.empty() && clean_base.back() == '/') {
        clean_base.pop_back();
    }
    
    return clean_base + "/r/" + code;
} 