#ifndef SLUG_GENERATOR_H
#define SLUG_GENERATOR_H

#include <string>
#include <random>
#include "database/database_interface.h"

using namespace std;

/**
 * @brief Generates unique alphanumeric codes for shortened links
 * 
 * Uses cryptographically secure random number generation
 * to create URL-safe identifiers.
 */
class SlugGenerator {
public:
    /**
     * @brief Generate a random alphanumeric code
     * @param length Length of code to generate (default: 8)
     * @return Random code string
     */
    static string generate_code(size_t length = 8);
    
    /**
     * @brief Generate a unique code not in database
     * @param db Database interface to check uniqueness
     * @param length Length of code to generate
     * @param max_attempts Maximum attempts before giving up
     * @return Unique code or empty string if failed
     */
    static string generate_unique_code(DatabaseInterface* db, 
                                        size_t length = 8,
                                        int max_attempts = 100);

private:
    static const string CHARSET;
    static random_device rd_;
    static mt19937 gen_;
    static uniform_int_distribution<> dis_;
};

#endif // SLUG_GENERATOR_H
