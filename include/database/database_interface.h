#ifndef DATABASE_INTERFACE_H
#define DATABASE_INTERFACE_H

#include <string>
#include <vector>
#include <memory>
#include <optional>

#include "models/link_data.h"

using namespace std;

/**
 * @brief Abstract interface for database operations
 * 
 * Provides a clean abstraction for all database operations
 * needed by the URL shortening service.
 */
class DatabaseInterface {
public:
    virtual ~DatabaseInterface() = default;
    
    /**
     * @brief Initialize database connection and create tables if needed
     * @param db_path Path to database file
     * @return true if successful, false otherwise
     */
    virtual bool initialize(const string& db_path) = 0;
    
    /**
     * @brief Create necessary database tables
     * @return true if successful, false otherwise
     */
    virtual bool create_tables() = 0;
    
    // Link Operations
    
    /**
     * @brief Insert a new shortened link
     * @param code Unique identifier
     * @param destination Original URL or file path
     * @param type "url" or "file"
     * @return true if successful, false otherwise
     */
    virtual bool insert_link(const string& code, 
                            const string& destination, 
                            const string& type) = 0;
    
    /**
     * @brief Retrieve link data by code
     * @param code Unique identifier
     * @return LinkData if found, nullopt otherwise
     */
    virtual optional<LinkData> get_link(const string& code) = 0;
    
    /**
     * @brief Check if a code already exists
     * @param code Unique identifier to check
     * @return true if exists, false otherwise
     */
    virtual bool code_exists(const string& code) = 0;
    
    /**
     * @brief Delete a link by code
     * @param code Unique identifier
     * @return true if successful, false otherwise
     */
    virtual bool delete_link(const string& code) = 0;
    
    // Analytics Operations
    
    /**
     * @brief Record a click/access event
     * @param record Click record with all details
     * @return true if successful, false otherwise
     */
    virtual bool record_click(const ClickRecord& record) = 0;
    
    /**
     * @brief Get click statistics for a code
     * @param code Unique identifier
     * @return ClickStats with aggregated data
     */
    virtual ClickStats get_click_stats(const string& code) = 0;
    
    /**
     * @brief Get recent click records for a code
     * @param code Unique identifier
     * @param limit Maximum number of records
     * @return Vector of recent click records
     */
    virtual vector<ClickRecord> get_recent_clicks(const string& code, 
                                                        int limit = 100) = 0;
};

#endif // DATABASE_INTERFACE_H