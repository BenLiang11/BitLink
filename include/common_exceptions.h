#ifndef COMMON_EXCEPTIONS_H
#define COMMON_EXCEPTIONS_H

#include <string>
#include <stdexcept>

namespace common {

/**
 * @brief Exception for duplicate location paths in configuration.
 */
class DuplicateLocationException : public std::runtime_error {
public:
    explicit DuplicateLocationException(const std::string& location)
        : std::runtime_error("Duplicate location in config: " + location) {}
};

/**
 * @brief Exception for locations with trailing slashes.
 */
class TrailingSlashException : public std::runtime_error {
public:
    explicit TrailingSlashException(const std::string& location)
        : std::runtime_error("Location paths cannot have trailing slashes: " + location) {}
};

} // namespace common

#endif // COMMON_EXCEPTIONS_H 