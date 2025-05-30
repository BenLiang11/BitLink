#include "models/link_data.h"

LinkData::LinkData(const string& code, const string& destination, const string& type)
    : code(code), destination(destination), type(type), created(chrono::system_clock::now()) {}

ClickRecord::ClickRecord(const string& c, const string& ip, const string& ua, const string& ref)
    : code(c), ip_truncated(ip), user_agent(ua), referrer(ref), timestamp(chrono::system_clock::now()) {}