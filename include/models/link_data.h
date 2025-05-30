#ifndef LINK_DATA_H
#define LINK_DATA_H

#include <string>
#include <chrono>
#include <map>
#include <optional>

using namespace std;

/**
 * @brief Shortened URL data entry
*/
struct LinkData {
    string code; // Unique 8 character code
    string destination; // The destination URL or file path
    string type; // URL or file
    chrono::system_clock::time_point created;

    LinkData() = default;
    LinkData(const string& code, const string& destination, const string& type);
};

/**
 * @brief Click Data Analytics for Shortened URLs
*/
struct ClickStats {
    int total_clicks;
    map<string, int> daily_clicks; // date(YYYY-MM-DD) -> clicks
    chrono::system_clock::time_point last_accessed;

    ClickStats() : total_clicks(0) {}
};

/**
 * @brief Individual click record for analytics
*/
struct ClickRecord {
    std::string code;
    std::string ip_truncated;    // Last octet removed for privacy
    std::string user_agent;
    std::string referrer;
    std::chrono::system_clock::time_point timestamp;
    
    ClickRecord() = default;
    ClickRecord(const std::string& c, const std::string& ip, 
                const std::string& ua, const std::string& ref);
};
  
#endif // LINK_DATA_H