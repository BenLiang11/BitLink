#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "handler_dispatcher.h"
#include "common_exceptions.h"
#include "handlers/echo_handler.h"
#include "handlers/static_file_handler.h"
#include "handlers/not_found_handler.h"
#include "handler_registry.h"
#include <map>
#include <memory>
#include <string>

/**
 * @brief Mock Request for testing
 * 
 * Creates a simple request with a specified URI.
 */
class MockRequest : public Request {
public:
    explicit MockRequest(const std::string& test_uri) : Request("GET " + test_uri + " HTTP/1.1\r\n\r\n") {}
};

/**
 * @brief Test fixture for HandlerDispatcher tests
 * 
 * Sets up handler registrations and a dispatcher instance
 * for testing path matching and handler instantiation.
 */
class HandlerDispatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register handlers with the registry
        ASSERT_TRUE(HandlerRegistry::RegisterHandler("EchoHandler", EchoHandler::Create));
        ASSERT_TRUE(HandlerRegistry::RegisterHandler("StaticHandler", StaticFileHandler::Create));
        ASSERT_TRUE(HandlerRegistry::RegisterHandler("NotFoundHandler", NotFoundHandler::Create));
        
        // Create mock handler registrations for different paths
        // Echo handler for /echo
        echo_registration.location = "/echo";
        echo_registration.handler_name = "EchoHandler";
        echo_registration.args = {}; // EchoHandler takes no args
        
        // Static handler for /static
        static_registration.location = "/static";
        static_registration.handler_name = "StaticHandler";
        static_registration.args = {"/static", "./static_files"};
        
        // Echo handler for root path
        root_registration.location = "/";
        root_registration.handler_name = "NotFoundHandler";
        root_registration.args = {};
        
        // Set up the handler registrations map
        handler_registrations["/echo"] = echo_registration;
        handler_registrations["/static"] = static_registration;
        handler_registrations["/"] = root_registration;
        
        // Create the dispatcher with our handler registrations
        dispatcher = std::make_unique<HandlerDispatcher>(handler_registrations);
    }

    HandlerRegistration echo_registration;
    HandlerRegistration static_registration;
    HandlerRegistration root_registration;
    std::map<std::string, HandlerRegistration> handler_registrations;
    std::unique_ptr<HandlerDispatcher> dispatcher;
};

/**
 * @brief Test exact path matching
 */
TEST_F(HandlerDispatcherTest, ExactPathMatch) {
    // Test exact match to echo handler
    MockRequest req_echo("/echo");
    auto handler = dispatcher->CreateHandlerForRequest(req_echo);
    ASSERT_NE(handler, nullptr);
    EXPECT_NE(dynamic_cast<EchoHandler*>(handler.get()), nullptr);
    
    // Test exact match to static handler
    MockRequest req_static("/static");
    handler = dispatcher->CreateHandlerForRequest(req_static);
    ASSERT_NE(handler, nullptr);
    EXPECT_NE(dynamic_cast<StaticFileHandler*>(handler.get()), nullptr);
    
    // Test exact match to root handler
    MockRequest req_root("/");
    handler = dispatcher->CreateHandlerForRequest(req_root);
    ASSERT_NE(handler, nullptr);
    EXPECT_NE(dynamic_cast<NotFoundHandler*>(handler.get()), nullptr);
}

/**
 * @brief Test prefix matching for paths
 */
TEST_F(HandlerDispatcherTest, PrefixMatch) {
    // File under static should use static handler
    MockRequest req("/static/image.jpg");
    auto handler = dispatcher->CreateHandlerForRequest(req);
    ASSERT_NE(handler, nullptr);
    EXPECT_NE(dynamic_cast<StaticFileHandler*>(handler.get()), nullptr);
    
    // Subdirectory under static should also use static handler
    MockRequest req2("/static/images/logo.png");
    handler = dispatcher->CreateHandlerForRequest(req2);
    ASSERT_NE(handler, nullptr);
    EXPECT_NE(dynamic_cast<StaticFileHandler*>(handler.get()), nullptr);
}

/**
 * @brief Test longest prefix matching algorithm
 * 
 * Verifies that the dispatcher selects the handler with
 * the longest matching prefix when multiple handlers could match.
 */
TEST_F(HandlerDispatcherTest, LongestPrefixMatch) {
    // Add a more specific static path
    HandlerRegistration specific_registration;
    specific_registration.location = "/static/images";
    specific_registration.handler_name = "EchoHandler"; // Intentionally using EchoHandler for this path
    specific_registration.args = {};
    
    auto registrations_with_specific = handler_registrations;
    registrations_with_specific["/static/images"] = specific_registration;
    
    auto new_dispatcher = std::make_unique<HandlerDispatcher>(registrations_with_specific);
    
    // This should match the more specific /static/images path, not the /static path
    MockRequest req("/static/images/logo.png");
    auto handler = new_dispatcher->CreateHandlerForRequest(req);
    ASSERT_NE(handler, nullptr);
    EXPECT_NE(dynamic_cast<EchoHandler*>(handler.get()), nullptr);
    
    // This should still match the general /static path
    MockRequest req2("/static/styles.css");
    handler = new_dispatcher->CreateHandlerForRequest(req2);
    ASSERT_NE(handler, nullptr);
    EXPECT_NE(dynamic_cast<StaticFileHandler*>(handler.get()), nullptr);
}

/**
 * @brief Test per-request handler instantiation
 * 
 * Verifies that each request gets a new handler instance,
 * even when requesting the same path repeatedly.
 */
TEST_F(HandlerDispatcherTest, PerRequestInstantiation) {
    // Create two requests for the same path
    MockRequest req1("/echo");
    MockRequest req2("/echo");
    
    // Get handlers for both requests
    auto handler1 = dispatcher->CreateHandlerForRequest(req1);
    auto handler2 = dispatcher->CreateHandlerForRequest(req2);
    
    // Both should be created successfully
    ASSERT_NE(handler1, nullptr);
    ASSERT_NE(handler2, nullptr);
    
    // They should be different instances
    EXPECT_NE(handler1.get(), handler2.get());
}

/**
 * @brief Test duplicate location detection
 */
TEST_F(HandlerDispatcherTest, DuplicateLocationDetection) {
    // Create a map with duplicate locations
    auto duplicate_registrations = handler_registrations;
    
    HandlerRegistration duplicate_echo;
    duplicate_echo.location = "/echo"; // Duplicate of existing path
    duplicate_echo.handler_name = "EchoHandler";
    duplicate_echo.args = {};
    
    duplicate_registrations["/echo_duplicate"] = duplicate_echo;
    
    // Creating a dispatcher with duplicate locations should throw
    EXPECT_THROW({
        std::make_unique<HandlerDispatcher>(duplicate_registrations);
    }, common::DuplicateLocationException);
}

/**
 * @brief Test trailing slash rejection
 */
TEST_F(HandlerDispatcherTest, TrailingSlashRejection) {
    // Create a registration with trailing slash
    auto slash_registrations = handler_registrations;
    
    HandlerRegistration slash_registration;
    slash_registration.location = "/trailing/"; // Has trailing slash
    slash_registration.handler_name = "EchoHandler";
    slash_registration.args = {};
    
    slash_registrations["/trailing/"] = slash_registration;
    
    // Creating a dispatcher with trailing slash should throw
    EXPECT_THROW({
        std::make_unique<HandlerDispatcher>(slash_registrations);
    }, common::TrailingSlashException);
}

/**
 * @brief Test empty dispatcher behavior
 */
TEST_F(HandlerDispatcherTest, EmptyDispatcher) {
    std::map<std::string, HandlerRegistration> empty_registrations;
    HandlerDispatcher empty_dispatcher(empty_registrations);
    
    // Any path with an empty dispatcher should return nullptr
    MockRequest req("/any/path");
    auto handler = empty_dispatcher.CreateHandlerForRequest(req);
    EXPECT_EQ(handler, nullptr);
}

/**
 * @brief Test path component matching
 * 
 * Verifies that the dispatcher correctly identifies path components
 * and doesn't match partial path segments.
 */
TEST_F(HandlerDispatcherTest, PathComponentMatching) {
    // Create a dispatcher with a specific path
    std::map<std::string, HandlerRegistration> registrations;
    
    HandlerRegistration echo_reg;
    echo_reg.location = "/echo";
    echo_reg.handler_name = "EchoHandler";
    echo_reg.args = {};
    registrations["/echo"] = echo_reg;

    HandlerRegistration not_found_handler = {"/", "NotFoundHandler", {}};
    registrations["/"] = not_found_handler;
    
    HandlerDispatcher path_dispatcher(registrations);
    
    // This should match
    MockRequest valid_req("/echo/test");
    auto handler1 = path_dispatcher.CreateHandlerForRequest(valid_req);
    ASSERT_NE(handler1, nullptr);
    EXPECT_NE(dynamic_cast<EchoHandler*>(handler1.get()), nullptr);
    
    // This should not match (it's a partial path component match)
    MockRequest invalid_req("/echo123");
    auto handler2 = path_dispatcher.CreateHandlerForRequest(invalid_req);
    ASSERT_NE(handler2, nullptr);
    EXPECT_NE(dynamic_cast<NotFoundHandler*>(handler2.get()), nullptr);
} 