#include "gtest/gtest.h"
#include "server_config.h"
#include "handlers/echo_handler.h"
#include "handlers/static_file_handler.h"
#include "config_parser.h"
#include <memory>

class ServerConfigTest : public ::testing::Test {
protected:
  ServerConfig config_;
  NginxConfig nginx_config_;
};

TEST_F(ServerConfigTest, ParseShortStatement) {
  // Create config with invalid port
  auto port_statement = std::make_shared<NginxConfigStatement>();
  port_statement->tokens_.push_back("listen");
  port_statement->tokens_.push_back("8080");
  nginx_config_.statements_.push_back(port_statement);
  
  // skips short statements
  auto short_statement = std::make_shared<NginxConfigStatement>();
  short_statement->tokens_.push_back("location");
  nginx_config_.statements_.push_back(short_statement);
  
  EXPECT_TRUE(config_.ParseConfig(nginx_config_));
}

TEST_F(ServerConfigTest, ParseValidConfig) {
  // Create a valid config with port and handlers
  auto port_statement = std::make_shared<NginxConfigStatement>();
  port_statement->tokens_.push_back("listen");
  port_statement->tokens_.push_back("8080");
  nginx_config_.statements_.push_back(port_statement);

  auto echo_statement = std::make_shared<NginxConfigStatement>();
  echo_statement->tokens_.push_back("location");
  echo_statement->tokens_.push_back("/echo");
  echo_statement->tokens_.push_back("echo");
  nginx_config_.statements_.push_back(echo_statement);

  auto static_statement = std::make_shared<NginxConfigStatement>();
  static_statement->tokens_.push_back("location");
  static_statement->tokens_.push_back("/static");
  static_statement->tokens_.push_back("static");
  
  // Create child block for static handler
  auto static_child_block = std::make_unique<NginxConfig>();
  auto root_statement = std::make_shared<NginxConfigStatement>();
  root_statement->tokens_.push_back("root");
  root_statement->tokens_.push_back("/path/to/files");
  static_child_block->statements_.push_back(root_statement);
  static_statement->child_block_ = std::move(static_child_block);
  
  nginx_config_.statements_.push_back(static_statement);

  EXPECT_TRUE(config_.ParseConfig(nginx_config_));
  EXPECT_EQ(config_.port(), 8080);
  EXPECT_EQ(config_.locations().size(), 2);
}

TEST_F(ServerConfigTest, ParseInvalidPort) {
  // Create config with invalid port
  auto port_statement = std::make_shared<NginxConfigStatement>();
  port_statement->tokens_.push_back("listen");
  port_statement->tokens_.push_back("invalid");
  nginx_config_.statements_.push_back(port_statement);
  
  EXPECT_FALSE(config_.ParseConfig(nginx_config_));
}

TEST_F(ServerConfigTest, ParseMissingPort) {
  // Create config without port
  auto echo_statement = std::make_shared<NginxConfigStatement>();
  echo_statement->tokens_.push_back("location");
  echo_statement->tokens_.push_back("/echo");
  echo_statement->tokens_.push_back("echo");
  nginx_config_.statements_.push_back(echo_statement);
  
  EXPECT_FALSE(config_.ParseConfig(nginx_config_));
}

// TEST_F(ServerConfigTest, ParseInvalidStaticHandler) {
//   // Create config with static handler missing root
//   auto port_statement = std::make_shared<NginxConfigStatement>();
//   port_statement->tokens_.push_back("listen");
//   port_statement->tokens_.push_back("8080");
//   nginx_config_.statements_.push_back(port_statement);

//   auto static_statement = std::make_shared<NginxConfigStatement>();
//   static_statement->tokens_.push_back("location");
//   static_statement->tokens_.push_back("/static");
//   static_statement->tokens_.push_back("static");
//   // No child block with root directive
//   static_statement->child_block_ = std::make_unique<NginxConfig>();
//   nginx_config_.statements_.push_back(static_statement);

//   EXPECT_FALSE(config_.ParseConfig(nginx_config_));
// }

TEST_F(ServerConfigTest, CreateHandlers) {
  // Set up valid configuration
  auto port_statement = std::make_shared<NginxConfigStatement>();
  port_statement->tokens_.push_back("listen");
  port_statement->tokens_.push_back("8080");
  nginx_config_.statements_.push_back(port_statement);

  auto echo_statement = std::make_shared<NginxConfigStatement>();
  echo_statement->tokens_.push_back("location");
  echo_statement->tokens_.push_back("/echo");
  echo_statement->tokens_.push_back("echo");
  nginx_config_.statements_.push_back(echo_statement);

  // First static handler
  auto static_statement = std::make_shared<NginxConfigStatement>();
  static_statement->tokens_.push_back("location");
  static_statement->tokens_.push_back("/static");
  static_statement->tokens_.push_back("static");
  
  auto static_child_block = std::make_unique<NginxConfig>();
  auto root_statement = std::make_shared<NginxConfigStatement>();
  root_statement->tokens_.push_back("root");
  root_statement->tokens_.push_back("/path/to/files");
  static_child_block->statements_.push_back(root_statement);
  static_statement->child_block_ = std::move(static_child_block);
  
  nginx_config_.statements_.push_back(static_statement);

  // Second static handler
  auto static_statement2 = std::make_shared<NginxConfigStatement>();
  static_statement2->tokens_.push_back("location");
  static_statement2->tokens_.push_back("/static1");
  static_statement2->tokens_.push_back("static");
  
  auto static_child_block2 = std::make_unique<NginxConfig>();
  auto root_statement2 = std::make_shared<NginxConfigStatement>();
  root_statement2->tokens_.push_back("root");
  root_statement2->tokens_.push_back("/data/static1");
  static_child_block2->statements_.push_back(root_statement2);
  static_statement2->child_block_ = std::move(static_child_block2);
  
  nginx_config_.statements_.push_back(static_statement2);

  EXPECT_TRUE(config_.ParseConfig(nginx_config_));

  auto handlers = config_.CreateHandlers();
  EXPECT_EQ(handlers.size(), 3);
  EXPECT_NE(handlers.find("/echo"), handlers.end());
  EXPECT_NE(handlers.find("/static"), handlers.end());
}

TEST_F(ServerConfigTest, CreateDefaultHandler) {
  // Set up config with only port
  auto port_statement = std::make_shared<NginxConfigStatement>();
  port_statement->tokens_.push_back("listen");
  port_statement->tokens_.push_back("8080");
  nginx_config_.statements_.push_back(port_statement);

  EXPECT_TRUE(config_.ParseConfig(nginx_config_));

  auto handlers = config_.CreateHandlers();
  EXPECT_EQ(handlers.size(), 1);
  EXPECT_NE(handlers.find("/"), handlers.end());
}

// TEST_F(ServerConfigTest, CreateStaticHandlerWithoutRoot) {
//   // Set up config with static handler missing root
//   auto port_statement = std::make_shared<NginxConfigStatement>();
//   port_statement->tokens_.push_back("listen");
//   port_statement->tokens_.push_back("8080");
//   nginx_config_.statements_.push_back(port_statement);

//   auto static_statement = std::make_shared<NginxConfigStatement>();
//   static_statement->tokens_.push_back("location");
//   static_statement->tokens_.push_back("/static");
//   static_statement->tokens_.push_back("static");
//   // Empty child block without root directive
//   static_statement->child_block_ = std::make_unique<NginxConfig>();
//   nginx_config_.statements_.push_back(static_statement);

//   EXPECT_FALSE(config_.ParseConfig(nginx_config_));
// }