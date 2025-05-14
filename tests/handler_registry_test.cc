#include "gtest/gtest.h"
#include "handler_registry.h"
#include "handlers/echo_handler.h" // For a concrete handler to test with
#include "handlers/static_file_handler.h" // For StaticFileHandler tests
#include "handlers/not_found_handler.h" // For NotFoundHandler tests
#include <vector>
#include <string>
#include <memory>

/**
 * @brief Mock RequestHandler for testing factory registration
 * 
 * Provides a simple implementation that stores its argument
 * to verify proper creation through the factory pattern.
 */
class MockTestHandler : public RequestHandler {
public:
    std::string id_;
    explicit MockTestHandler(const std::string& id) : id_(id) {}
    std::unique_ptr<Response> handle_request(const Request& /*req*/) override {
        // Not strictly needed for registry test, but must be implemented
        return std::make_unique<Response>(); 
    }
    static std::unique_ptr<RequestHandler> Create(const std::vector<std::string>& args) {
        if (args.empty()) return std::make_unique<MockTestHandler>("default");
        return std::make_unique<MockTestHandler>(args[0]);
    }
};

/**
 * @brief Test fixture for handler registry tests
 */
class HandlerRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register handlers for each test
        ASSERT_TRUE(HandlerRegistry::RegisterHandler("MockTestHandler", MockTestHandler::Create));
        ASSERT_TRUE(HandlerRegistry::RegisterHandler("EchoHandler", EchoHandler::Create));
        ASSERT_TRUE(HandlerRegistry::RegisterHandler("StaticHandler", StaticFileHandler::Create));
        ASSERT_TRUE(HandlerRegistry::RegisterHandler("NotFoundHandler", NotFoundHandler::Create)); 

    }
};

/**
 * @brief Test basic registration and creation of handlers
 */
TEST_F(HandlerRegistryTest, RegistrationAndCreation) {
    // Create an instance of the mock handler with no args
    std::vector<std::string> no_args;
    std::unique_ptr<RequestHandler> handler1 = HandlerRegistry::CreateHandler("MockTestHandler", no_args);
    ASSERT_NE(handler1, nullptr);
    MockTestHandler* mock_handler1 = dynamic_cast<MockTestHandler*>(handler1.get());
    ASSERT_NE(mock_handler1, nullptr);
    EXPECT_EQ(mock_handler1->id_, "default");

    // Create an instance with an argument
    std::vector<std::string> an_arg = {"arg1"};
    std::unique_ptr<RequestHandler> handler2 = HandlerRegistry::CreateHandler("MockTestHandler", an_arg);
    ASSERT_NE(handler2, nullptr);
    MockTestHandler* mock_handler2 = dynamic_cast<MockTestHandler*>(handler2.get());
    ASSERT_NE(mock_handler2, nullptr);
    EXPECT_EQ(mock_handler2->id_, "arg1");
}

/**
 * @brief Test attempting to create a non-existent handler
 */
TEST_F(HandlerRegistryTest, CreateNonExistentHandler) {
    std::vector<std::string> no_args;
    std::unique_ptr<RequestHandler> handler = HandlerRegistry::CreateHandler("NonExistentHandler", no_args);
    ASSERT_EQ(handler, nullptr);
}

/**
 * @brief Test registration with a null factory
 */
TEST_F(HandlerRegistryTest, RegisterNullFactory) {
    bool registered = HandlerRegistry::RegisterHandler("NullFactoryHandler", nullptr);
    ASSERT_FALSE(registered);
}

/**
 * @brief Test registration with an empty name
 */
TEST_F(HandlerRegistryTest, RegisterEmptyName) {
    bool registered = HandlerRegistry::RegisterHandler("", MockTestHandler::Create);
    ASSERT_FALSE(registered);
}

/**
 * @brief Test duplicate handler registration
 */
TEST_F(HandlerRegistryTest, RegisterDuplicateHandler) {
    // Try to register the same handler again
    bool registered = HandlerRegistry::RegisterHandler("MockTestHandler", MockTestHandler::Create);
    // Should return false for duplicate registration
    ASSERT_FALSE(registered);
}

/**
 * @brief Test EchoHandler integration with the registry
 */
TEST_F(HandlerRegistryTest, EchoHandlerIntegration) {
    // Create an EchoHandler with no args (valid case)
    std::vector<std::string> no_args;
    std::unique_ptr<RequestHandler> echo_handler = HandlerRegistry::CreateHandler("EchoHandler", no_args);
    ASSERT_NE(echo_handler, nullptr);
    EchoHandler* casted_echo_handler = dynamic_cast<EchoHandler*>(echo_handler.get());
    ASSERT_NE(casted_echo_handler, nullptr);
    
    // Test with invalid arguments (EchoHandler takes no args)
    std::vector<std::string> invalid_args = {"extra_arg"};
    EXPECT_THROW(HandlerRegistry::CreateHandler("EchoHandler", invalid_args), std::invalid_argument);
}

/**
 * @brief Test StaticHandler integration with the registry
 */
TEST_F(HandlerRegistryTest, StaticHandlerIntegration) {
    // Create a StaticHandler with valid args
    std::vector<std::string> static_args = {"/static", "./test_files"}; // Example args
    std::unique_ptr<RequestHandler> static_handler = HandlerRegistry::CreateHandler("StaticHandler", static_args);
    ASSERT_NE(static_handler, nullptr);
    StaticFileHandler* casted_static_handler = dynamic_cast<StaticFileHandler*>(static_handler.get());
    ASSERT_NE(casted_static_handler, nullptr);

    // Test creation with missing arguments
    std::vector<std::string> bad_static_args = {"/static"};
    EXPECT_THROW(HandlerRegistry::CreateHandler("StaticHandler", bad_static_args), std::invalid_argument);
    
    // Test creation with too many arguments
    std::vector<std::string> too_many_args = {"/static", "./test_files", "extra"};
    EXPECT_THROW(HandlerRegistry::CreateHandler("StaticHandler", too_many_args), std::invalid_argument);
}

/**
 * @brief Test creating multiple handler instances
 */
TEST_F(HandlerRegistryTest, MultipleHandlerInstances) {
    // Create multiple static handlers with different configurations
    std::vector<std::string> static_args1 = {"/static1", "./test_files1"};
    std::vector<std::string> static_args2 = {"/static2", "./test_files2"};
    
    auto handler1 = HandlerRegistry::CreateHandler("StaticHandler", static_args1);
    auto handler2 = HandlerRegistry::CreateHandler("StaticHandler", static_args2);
    
    // Verify they were created successfully
    ASSERT_NE(handler1, nullptr);
    ASSERT_NE(handler2, nullptr);
    
    // Verify they are different instances
    EXPECT_NE(handler1.get(), handler2.get());
} 

/**
 * @brief Test NotFoundHandler creation with no arguments
 */
TEST_F(HandlerRegistryTest, NotFoundHandler_Create_NoArguments) {
    std::vector<std::string> args;  // No arguments
    auto handler = HandlerRegistry::CreateHandler("NotFoundHandler", args);
    ASSERT_NE(handler, nullptr);
    NotFoundHandler* casted_handler = dynamic_cast<NotFoundHandler*>(handler.get());
    ASSERT_NE(casted_handler, nullptr);
}

/**
 * @brief Test NotFoundHandler creation with arguments
 */

// TEST_F(HandlerRegistryTest, NotFoundHandler_Create_WithArguments) {
//     std::vector<std::string> args = {"unexpected_argument"};  // Invalid argument
//     auto handler = HandlerRegistry::CreateHandler("NotFoundHandler", args);
//     ASSERT_NE(handler, nullptr);
//     NotFoundHandler* casted_handler = dynamic_cast<NotFoundHandler*>(handler.get());
//     ASSERT_NE(casted_handler, nullptr);
// }
/**
 * @brief Test NotFoundHandler's request handling and response
 */
TEST_F(HandlerRegistryTest, NotFoundHandler_HandleRequest) {
    std::vector<std::string> args;  // No arguments
    auto handler = HandlerRegistry::CreateHandler("NotFoundHandler", args);

    // Create a request with a body
    std::string raw_request = 
        "GET /nonexistent HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Some request body";

    Request req(raw_request);
    // Handle the request
    auto response = handler->handle_request(req);

    // Check the response status code and body
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->status(), Response::NOT_FOUND);  // Should return 404
    EXPECT_EQ(response->body(), "<html><body><h1>404 Not Found</h1><p>The requested file could not be found.</p></body></html>");
    EXPECT_EQ(response->headers().at("Content-Type"), "text/html");
    EXPECT_EQ(response->headers().at("Connection"), "close");
}