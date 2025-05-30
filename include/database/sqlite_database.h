#ifndef SQLITE_DATABASE_H
#define SQLITE_DATABASE_H

#include <sqlite3.h>
#include <mutex>
#include <string>
#include <vector>
#include <optional>

#include "database/database_interface.h"
#include "models/link_data.h"

using namespace std;
/**
 * @brief SQLite implementation of DatabaseInterface
 * 
 * Thread-safe SQLite database implementation with proper
 * connection management and prepared statements.
 */
class SQLiteDatabase : public DatabaseInterface {
public:
    SQLiteDatabase();
    ~SQLiteDatabase();
    
    // DatabaseInterface implementation
    bool initialize(const string& db_path) override;
    bool create_tables() override;
    
    bool insert_link(const string& code, 
                    const string& destination, 
                    const string& type) override;
    optional<LinkData> get_link(const string& code) override;
    bool code_exists(const string& code) override;
    bool delete_link(const string& code) override;
    
    bool record_click(const ClickRecord& record) override;
    ClickStats get_click_stats(const string& code) override;
    vector<ClickRecord> get_recent_clicks(const string& code, 
                                            int limit = 100) override;

private:
    sqlite3* db_;
    mutex db_mutex_;
    
    // Prepared statements for performance
    sqlite3_stmt* insert_link_stmt_;
    sqlite3_stmt* get_link_stmt_;
    sqlite3_stmt* code_exists_stmt_;
    sqlite3_stmt* delete_link_stmt_;
    sqlite3_stmt* insert_click_stmt_;
    sqlite3_stmt* get_stats_stmt_;
    sqlite3_stmt* get_recent_clicks_stmt_;
    
    bool prepare_statements();
    void cleanup_statements();
    string get_current_date() const;
};

#endif // SQLITE_DATABASE_H