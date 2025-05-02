#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "handler_dispatcher.h"
#include "handlers/echo_handler.h"
#include "handlers/static_file_handler.h"
#include <map>
#include <memory>
#include <string>

class HandlerDispatcherTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create mock handlers for different paths
    echo_handler = std::make_shared<EchoHandler>();
    static_handler = std::make_shared<StaticFileHandler>("./static", "/static");
    
    // Set up path mapping
    path_to_handler["/echo"] = echo_handler;
    path_to_handler["/static/"] = static_handler;
    path_to_handler["/"] = std::make_shared<EchoHandler>(); // Root handler
    
    // Create the dispatcher with our path mapping
    dispatcher = std::make_shared<HandlerDispatcher>(path_to_handler);
  }

  std::map<std::string, std::shared_ptr<RequestHandler>> path_to_handler;
  std::shared_ptr<RequestHandler> echo_handler;
  std::shared_ptr<RequestHandler> static_handler;
  std::shared_ptr<HandlerDispatcher> dispatcher;
};

// Test exact path matching
TEST_F(HandlerDispatcherTest, ExactPathMatch) {
  // Test exact match to echo handler
  auto handler = dispatcher->GetHandler("/echo");
  EXPECT_EQ(handler, echo_handler);
  
  // Test exact match to static handler
  handler = dispatcher->GetHandler("/static/");
  EXPECT_EQ(handler, static_handler);
  
  // Test exact match to root handler
  handler = dispatcher->GetHandler("/");
  EXPECT_EQ(handler, path_to_handler["/"]);
}

// Test prefix matching
TEST_F(HandlerDispatcherTest, PrefixMatch) {
  // File under static should use static handler
  auto handler = dispatcher->GetHandler("/static/image.jpg");
  EXPECT_EQ(handler, static_handler);
  
  // Subdirectory under static should use static handler
  handler = dispatcher->GetHandler("/static/images/logo.png");
  EXPECT_EQ(handler, static_handler);
}

// // Test no match
// TEST_F(HandlerDispatcherTest, NoMatch) {
//   // Path that doesn't match any handler should return nullptr
//   auto handler = dispatcher->GetHandler("/not-found");
//   EXPECT_EQ(handler, nullptr);
// }

// Test longest prefix match
TEST_F(HandlerDispatcherTest, LongestPrefixMatch) {
  // Add a more specific static path
  path_to_handler["/static/images/"] = echo_handler; // Intentionally using echo_handler for this path
  auto new_dispatcher = std::make_shared<HandlerDispatcher>(path_to_handler);
  
  // This should match the more specific /static/images/ path, not the /static/ path
  auto handler = new_dispatcher->GetHandler("/static/images/logo.png");
  EXPECT_EQ(handler, echo_handler);
  
  // This should still match the general /static/ path
  handler = new_dispatcher->GetHandler("/static/styles.css");
  EXPECT_EQ(handler, static_handler);
}

// Test empty dispatcher
TEST_F(HandlerDispatcherTest, EmptyDispatcher) {
  std::map<std::string, std::shared_ptr<RequestHandler>> empty_map;
  HandlerDispatcher empty_dispatcher(empty_map);
  
  // Any path with an empty dispatcher should return nullptr
  auto handler = empty_dispatcher.GetHandler("/any/path");
  EXPECT_EQ(handler, nullptr);
} 