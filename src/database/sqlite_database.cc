#include "database/sqlite_database.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

using namespace std;

SQLiteDatabase::SQLiteDatabase() 
    : db_(nullptr)
    , insert_link_stmt_(nullptr)
    , get_link_stmt_(nullptr) 
    , code_exists_stmt_(nullptr)
    , delete_link_stmt_(nullptr)
    , insert_click_stmt_(nullptr)
    , get_stats_stmt_(nullptr)
    , get_recent_clicks_stmt_(nullptr) {
}

SQLiteDatabase::~SQLiteDatabase() {
    cleanup_statements();
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool SQLiteDatabase::initialize(const string& db_path) {
    lock_guard<mutex> lock(db_mutex_);
    
    // Validate database path
    if (db_path.empty()) {
        cerr << "Database path cannot be empty" << endl;
        return false;
    }
    
    // Open database connection
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        cerr << "Failed to open database: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    // Enable foreign key constraints
    rc = sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to enable foreign keys: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    // Set WAL mode for better concurrency
    rc = sqlite3_exec(db_, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to set WAL mode: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    // Create tables
    if (!create_tables()) {
        return false;
    }
    
    // Prepare statements
    if (!prepare_statements()) {
        return false;
    }
    
    return true;
}

bool SQLiteDatabase::create_tables() {
    const char* create_links_table = R"SQL(
        CREATE TABLE IF NOT EXISTS links (
            code TEXT PRIMARY KEY,
            destination TEXT NOT NULL,
            type TEXT NOT NULL CHECK (type IN ('url', 'file')),
            created_at INTEGER NOT NULL
        )
    )SQL";
    
    const char* create_clicks_table = R"SQL(
        CREATE TABLE IF NOT EXISTS clicks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            code TEXT NOT NULL,
            ip_truncated TEXT,
            user_agent TEXT,
            referrer TEXT,
            timestamp INTEGER NOT NULL,
            FOREIGN KEY (code) REFERENCES links(code) ON DELETE CASCADE
        )
    )SQL";
    
    const char* create_indexes = R"SQL(
        CREATE INDEX IF NOT EXISTS idx_clicks_code ON clicks(code);
        CREATE INDEX IF NOT EXISTS idx_clicks_timestamp ON clicks(timestamp);
        CREATE INDEX IF NOT EXISTS idx_links_created ON links(created_at);
    )SQL";
    
    // Execute table creation
    int rc = sqlite3_exec(db_, create_links_table, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to create links table: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    rc = sqlite3_exec(db_, create_clicks_table, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to create clicks table: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    rc = sqlite3_exec(db_, create_indexes, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to create indexes: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    return true;
}

bool SQLiteDatabase::prepare_statements() {
    // Prepare INSERT statement for links
    const char* insert_link_sql = "INSERT INTO links (code, destination, type, created_at) VALUES (?, ?, ?, ?)";
    int rc = sqlite3_prepare_v2(db_, insert_link_sql, -1, &insert_link_stmt_, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to prepare insert_link statement: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    // Prepare SELECT statement for getting link by code
    const char* get_link_sql = "SELECT code, destination, type, created_at FROM links WHERE code = ?";
    rc = sqlite3_prepare_v2(db_, get_link_sql, -1, &get_link_stmt_, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to prepare get_link statement: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    // Prepare EXISTS check for code
    const char* code_exists_sql = "SELECT 1 FROM links WHERE code = ? LIMIT 1";
    rc = sqlite3_prepare_v2(db_, code_exists_sql, -1, &code_exists_stmt_, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to prepare code_exists statement: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    // Prepare DELETE statement for links
    const char* delete_link_sql = "DELETE FROM links WHERE code = ?";
    rc = sqlite3_prepare_v2(db_, delete_link_sql, -1, &delete_link_stmt_, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to prepare delete_link statement: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    // Prepare INSERT statement for clicks
    const char* insert_click_sql = "INSERT INTO clicks (code, ip_truncated, user_agent, referrer, timestamp) VALUES (?, ?, ?, ?, ?)";
    rc = sqlite3_prepare_v2(db_, insert_click_sql, -1, &insert_click_stmt_, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to prepare insert_click statement: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    // Prepare query for getting click statistics
    const char* get_stats_sql = R"SQL(
        SELECT 
            COUNT(*) as total_clicks,
            MAX(timestamp) as last_accessed
        FROM clicks 
        WHERE code = ?
    )SQL";
    rc = sqlite3_prepare_v2(db_, get_stats_sql, -1, &get_stats_stmt_, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to prepare get_stats statement: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    // Prepare query for getting recent clicks
    const char* get_recent_clicks_sql = R"SQL(
        SELECT code, ip_truncated, user_agent, referrer, timestamp 
        FROM clicks 
        WHERE code = ? 
        ORDER BY timestamp DESC 
        LIMIT ?
    )SQL";
    rc = sqlite3_prepare_v2(db_, get_recent_clicks_sql, -1, &get_recent_clicks_stmt_, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to prepare get_recent_clicks statement: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    return true;
}

void SQLiteDatabase::cleanup_statements() {
    if (insert_link_stmt_) {
        sqlite3_finalize(insert_link_stmt_);
        insert_link_stmt_ = nullptr;
    }
    if (get_link_stmt_) {
        sqlite3_finalize(get_link_stmt_);
        get_link_stmt_ = nullptr;
    }
    if (code_exists_stmt_) {
        sqlite3_finalize(code_exists_stmt_);
        code_exists_stmt_ = nullptr;
    }
    if (delete_link_stmt_) {
        sqlite3_finalize(delete_link_stmt_);
        delete_link_stmt_ = nullptr;
    }
    if (insert_click_stmt_) {
        sqlite3_finalize(insert_click_stmt_);
        insert_click_stmt_ = nullptr;
    }
    if (get_stats_stmt_) {
        sqlite3_finalize(get_stats_stmt_);
        get_stats_stmt_ = nullptr;
    }
    if (get_recent_clicks_stmt_) {
        sqlite3_finalize(get_recent_clicks_stmt_);
        get_recent_clicks_stmt_ = nullptr;
    }
}

string SQLiteDatabase::get_current_date() const {
    auto now = chrono::system_clock::now();
    auto time_t_now = chrono::system_clock::to_time_t(now);
    auto tm_now = *gmtime(&time_t_now);
    
    ostringstream oss;
    oss << put_time(&tm_now, "%Y-%m-%d");
    return oss.str();
}

bool SQLiteDatabase::insert_link(const string& code, const string& destination, const string& type) {
    lock_guard<mutex> lock(db_mutex_);
    
    // Validate input parameters
    if (code.empty()) {
        cerr << "Link code cannot be empty" << endl;
        return false;
    }
    
    // Reset statement
    sqlite3_reset(insert_link_stmt_);
    sqlite3_clear_bindings(insert_link_stmt_);
    
    // Get current timestamp
    auto now = chrono::system_clock::now();
    auto timestamp = chrono::duration_cast<chrono::seconds>(now.time_since_epoch()).count();
    
    // Bind parameters
    int rc = sqlite3_bind_text(insert_link_stmt_, 1, code.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind code: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    rc = sqlite3_bind_text(insert_link_stmt_, 2, destination.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind destination: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    rc = sqlite3_bind_text(insert_link_stmt_, 3, type.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind type: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    rc = sqlite3_bind_int64(insert_link_stmt_, 4, timestamp);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind timestamp: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    // Execute statement
    rc = sqlite3_step(insert_link_stmt_);
    if (rc != SQLITE_DONE) {
        cerr << "Failed to insert link: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    return true;
}

optional<LinkData> SQLiteDatabase::get_link(const string& code) {
    lock_guard<mutex> lock(db_mutex_);
    
    // Reset statement
    sqlite3_reset(get_link_stmt_);
    sqlite3_clear_bindings(get_link_stmt_);
    
    // Bind code parameter
    int rc = sqlite3_bind_text(get_link_stmt_, 1, code.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind code: " << sqlite3_errmsg(db_) << endl;
        return nullopt;
    }
    
    // Execute query
    rc = sqlite3_step(get_link_stmt_);
    if (rc == SQLITE_ROW) {
        // Extract data from result row
        const char* retrieved_code = reinterpret_cast<const char*>(sqlite3_column_text(get_link_stmt_, 0));
        const char* destination = reinterpret_cast<const char*>(sqlite3_column_text(get_link_stmt_, 1));
        const char* type = reinterpret_cast<const char*>(sqlite3_column_text(get_link_stmt_, 2));
        int64_t timestamp = sqlite3_column_int64(get_link_stmt_, 3);
        
        // Convert timestamp back to chrono::time_point
        auto created = chrono::system_clock::from_time_t(timestamp);
        
        // Create LinkData object
        LinkData link_data;
        link_data.code = retrieved_code ? retrieved_code : "";
        link_data.destination = destination ? destination : "";
        link_data.type = type ? type : "";
        link_data.created = created;
        
        return link_data;
    } else if (rc == SQLITE_DONE) {
        // No rows found
        return nullopt;
    } else {
        cerr << "Failed to execute get_link query: " << sqlite3_errmsg(db_) << endl;
        return nullopt;
    }
}

bool SQLiteDatabase::code_exists(const string& code) {
    lock_guard<mutex> lock(db_mutex_);
    
    // Reset statement
    sqlite3_reset(code_exists_stmt_);
    sqlite3_clear_bindings(code_exists_stmt_);
    
    // Bind code parameter
    int rc = sqlite3_bind_text(code_exists_stmt_, 1, code.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind code: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    // Execute query
    rc = sqlite3_step(code_exists_stmt_);
    if (rc == SQLITE_ROW) {
        return true;  // Code exists
    } else if (rc == SQLITE_DONE) {
        return false; // Code doesn't exist
    } else {
        cerr << "Failed to execute code_exists query: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
}

bool SQLiteDatabase::delete_link(const string& code) {
    lock_guard<mutex> lock(db_mutex_);
    
    // Reset statement
    sqlite3_reset(delete_link_stmt_);
    sqlite3_clear_bindings(delete_link_stmt_);
    
    // Bind code parameter
    int rc = sqlite3_bind_text(delete_link_stmt_, 1, code.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind code: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    // Execute statement
    rc = sqlite3_step(delete_link_stmt_);
    if (rc != SQLITE_DONE) {
        cerr << "Failed to delete link: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    // Check if any rows were affected
    int changes = sqlite3_changes(db_);
    return changes > 0;
}

bool SQLiteDatabase::record_click(const ClickRecord& record) {
    lock_guard<mutex> lock(db_mutex_);
    
    // Reset statement
    sqlite3_reset(insert_click_stmt_);
    sqlite3_clear_bindings(insert_click_stmt_);
    
    // Convert timestamp to Unix time
    auto timestamp = chrono::duration_cast<chrono::seconds>(record.timestamp.time_since_epoch()).count();
    
    // Bind parameters
    int rc = sqlite3_bind_text(insert_click_stmt_, 1, record.code.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind code: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    rc = sqlite3_bind_text(insert_click_stmt_, 2, record.ip_truncated.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind ip_truncated: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    rc = sqlite3_bind_text(insert_click_stmt_, 3, record.user_agent.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind user_agent: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    rc = sqlite3_bind_text(insert_click_stmt_, 4, record.referrer.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind referrer: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    rc = sqlite3_bind_int64(insert_click_stmt_, 5, timestamp);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind timestamp: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    // Execute statement
    rc = sqlite3_step(insert_click_stmt_);
    if (rc != SQLITE_DONE) {
        cerr << "Failed to insert click record: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
    
    return true;
}

ClickStats SQLiteDatabase::get_click_stats(const string& code) {
    lock_guard<mutex> lock(db_mutex_);
    
    ClickStats stats;
    
    // Get basic stats (total clicks and last accessed)
    sqlite3_reset(get_stats_stmt_);
    sqlite3_clear_bindings(get_stats_stmt_);
    
    int rc = sqlite3_bind_text(get_stats_stmt_, 1, code.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind code for stats: " << sqlite3_errmsg(db_) << endl;
        return stats;
    }
    
    rc = sqlite3_step(get_stats_stmt_);
    if (rc == SQLITE_ROW) {
        stats.total_clicks = sqlite3_column_int(get_stats_stmt_, 0);
        int64_t last_accessed_timestamp = sqlite3_column_int64(get_stats_stmt_, 1);
        
        if (last_accessed_timestamp > 0) {
            stats.last_accessed = chrono::system_clock::from_time_t(last_accessed_timestamp);
        }
    } else if (rc != SQLITE_DONE) {
        cerr << "Failed to get basic stats: " << sqlite3_errmsg(db_) << endl;
        return stats;
    }
    
    // Get daily click counts
    const char* daily_stats_sql = R"SQL(
        SELECT 
            date(timestamp, 'unixepoch') as click_date,
            COUNT(*) as daily_count
        FROM clicks 
        WHERE code = ?
        GROUP BY date(timestamp, 'unixepoch')
        ORDER BY click_date DESC
        LIMIT 30
    )SQL";
    
    sqlite3_stmt* daily_stats_stmt;
    rc = sqlite3_prepare_v2(db_, daily_stats_sql, -1, &daily_stats_stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to prepare daily stats query: " << sqlite3_errmsg(db_) << endl;
        return stats;
    }
    
    rc = sqlite3_bind_text(daily_stats_stmt, 1, code.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind code for daily stats: " << sqlite3_errmsg(db_) << endl;
        sqlite3_finalize(daily_stats_stmt);
        return stats;
    }
    
    while ((rc = sqlite3_step(daily_stats_stmt)) == SQLITE_ROW) {
        const char* date_str = reinterpret_cast<const char*>(sqlite3_column_text(daily_stats_stmt, 0));
        int count = sqlite3_column_int(daily_stats_stmt, 1);
        
        if (date_str) {
            stats.daily_clicks[string(date_str)] = count;
        }
    }
    
    if (rc != SQLITE_DONE) {
        cerr << "Failed to get daily stats: " << sqlite3_errmsg(db_) << endl;
    }
    
    sqlite3_finalize(daily_stats_stmt);
    return stats;
}

vector<ClickRecord> SQLiteDatabase::get_recent_clicks(const string& code, int limit) {
    lock_guard<mutex> lock(db_mutex_);
    
    vector<ClickRecord> records;
    
    // Reset statement
    sqlite3_reset(get_recent_clicks_stmt_);
    sqlite3_clear_bindings(get_recent_clicks_stmt_);
    
    // Bind parameters
    int rc = sqlite3_bind_text(get_recent_clicks_stmt_, 1, code.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind code: " << sqlite3_errmsg(db_) << endl;
        return records;
    }
    
    rc = sqlite3_bind_int(get_recent_clicks_stmt_, 2, limit);
    if (rc != SQLITE_OK) {
        cerr << "Failed to bind limit: " << sqlite3_errmsg(db_) << endl;
        return records;
    }
    
    // Execute query and collect results
    while ((rc = sqlite3_step(get_recent_clicks_stmt_)) == SQLITE_ROW) {
        ClickRecord record;
        
        const char* retrieved_code = reinterpret_cast<const char*>(sqlite3_column_text(get_recent_clicks_stmt_, 0));
        const char* ip_truncated = reinterpret_cast<const char*>(sqlite3_column_text(get_recent_clicks_stmt_, 1));
        const char* user_agent = reinterpret_cast<const char*>(sqlite3_column_text(get_recent_clicks_stmt_, 2));
        const char* referrer = reinterpret_cast<const char*>(sqlite3_column_text(get_recent_clicks_stmt_, 3));
        int64_t timestamp = sqlite3_column_int64(get_recent_clicks_stmt_, 4);
        
        record.code = retrieved_code ? retrieved_code : "";
        record.ip_truncated = ip_truncated ? ip_truncated : "";
        record.user_agent = user_agent ? user_agent : "";
        record.referrer = referrer ? referrer : "";
        record.timestamp = chrono::system_clock::from_time_t(timestamp);
        
        records.push_back(record);
    }
    
    if (rc != SQLITE_DONE) {
        cerr << "Failed to get recent clicks: " << sqlite3_errmsg(db_) << endl;
        records.clear();
    }
    
    return records;
} 