#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include "handlers/base_handler.h" // For RequestHandler base class

/**
 * @brief Registry for mapping handler names to factory functions.
 *
 * Allows dynamic instantiation of handlers by name. Handler arguments from
 * the config file are passed as a vector of strings to the factory.
 */
class HandlerRegistry {
public:
    /**
     * @brief Type alias for a handler factory function.
     * The factory takes a vector of strings (parsed arguments from config)
     * and returns a unique_ptr to a RequestHandler instance.
     */
    using HandlerFactory = std::function<std::unique_ptr<RequestHandler>(const std::vector<std::string>& args)>;

    /**
     * @brief Registers a handler factory with a given name.
     *
     * @param name The unique name of the handler (e.g., "EchoHandler").
     * @param factory The factory function that creates an instance of the handler.
     * @return true if registration was successful, false otherwise (e.g., name collision).
     *         Note: For simplicity, this example always returns true if no exception.
     */
    static bool RegisterHandler(const std::string& name, HandlerFactory factory);

    /**
     * @brief Creates a handler instance given its name and arguments.
     *
     * @param name The name of the handler to create.
     * @param args A vector of string arguments for the handler's constructor.
     * @return A unique_ptr to the created RequestHandler, or nullptr if the name is not found.
     */
    static std::unique_ptr<RequestHandler> CreateHandler(const std::string& name, const std::vector<std::string>& args);

private:
    // Static map to store handler names and their factories.
    // The function returns a reference to a static local variable to ensure it's initialized once.
    static std::unordered_map<std::string, HandlerFactory>& get_registry_map();
}; 