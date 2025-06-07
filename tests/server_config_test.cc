#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "server_config.h"
#include "handlers/echo_handler.h"
#include "handlers/static_file_handler.h"
#include "config_parser.h"
#include "common_exceptions.h"
#include <memory>
#include <variant>

class ServerConfigTest : public ::testing::Test {
protected:
  ServerConfig config_;
  NginxConfig nginx_config_;
};

TEST_F(ServerConfigTest, ParseShortStatement) {
  // Create config with valid port
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
  echo_statement->tokens_.push_back("EchoHandler");
  nginx_config_.statements_.push_back(echo_statement);

  auto static_statement = std::make_shared<NginxConfigStatement>();
  static_statement->tokens_.push_back("location");
  static_statement->tokens_.push_back("/static");
  static_statement->tokens_.push_back("StaticHandler");
  
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
  EXPECT_EQ(config_.locations().size(), 3);
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
  echo_statement->tokens_.push_back("EchoHandler");
  nginx_config_.statements_.push_back(echo_statement);
  
  EXPECT_FALSE(config_.ParseConfig(nginx_config_));
}

TEST_F(ServerConfigTest, ParseInvalidStaticHandler) {
  // Create config with static handler missing root
  auto port_statement = std::make_shared<NginxConfigStatement>();
  port_statement->tokens_.push_back("listen");
  port_statement->tokens_.push_back("8080");
  nginx_config_.statements_.push_back(port_statement);

  auto static_statement = std::make_shared<NginxConfigStatement>();
  static_statement->tokens_.push_back("location");
  static_statement->tokens_.push_back("/static");
  static_statement->tokens_.push_back("StaticHandler");
  // Empty child block
  static_statement->child_block_ = std::make_unique<NginxConfig>();
  nginx_config_.statements_.push_back(static_statement);

  EXPECT_FALSE(config_.ParseConfig(nginx_config_));
}

TEST_F(ServerConfigTest, ParseDuplicateLocation) {
  // Create config with duplicate locations
  auto port_statement = std::make_shared<NginxConfigStatement>();
  port_statement->tokens_.push_back("listen");
  port_statement->tokens_.push_back("8080");
  nginx_config_.statements_.push_back(port_statement);

  auto echo_statement1 = std::make_shared<NginxConfigStatement>();
  echo_statement1->tokens_.push_back("location");
  echo_statement1->tokens_.push_back("/echo");
  echo_statement1->tokens_.push_back("EchoHandler");
  nginx_config_.statements_.push_back(echo_statement1);
  
  auto echo_statement2 = std::make_shared<NginxConfigStatement>();
  echo_statement2->tokens_.push_back("location");
  echo_statement2->tokens_.push_back("/echo");
  echo_statement2->tokens_.push_back("EchoHandler");
  nginx_config_.statements_.push_back(echo_statement2);

  EXPECT_THROW({
    config_.ParseConfig(nginx_config_);
  }, common::DuplicateLocationException);
}

TEST_F(ServerConfigTest, ParseTrailingSlash) {
  // Create config with trailing slash in location
  auto port_statement = std::make_shared<NginxConfigStatement>();
  port_statement->tokens_.push_back("listen");
  port_statement->tokens_.push_back("8080");
  nginx_config_.statements_.push_back(port_statement);

  auto echo_statement = std::make_shared<NginxConfigStatement>();
  echo_statement->tokens_.push_back("location");
  echo_statement->tokens_.push_back("/echo/");  // Trailing slash
  echo_statement->tokens_.push_back("EchoHandler");
  nginx_config_.statements_.push_back(echo_statement);

  EXPECT_THROW({
    config_.ParseConfig(nginx_config_);
  }, common::TrailingSlashException);
}

TEST_F(ServerConfigTest, ParseTypedArguments) {
  // Create config with various typed arguments
  auto port_statement = std::make_shared<NginxConfigStatement>();
  port_statement->tokens_.push_back("listen");
  port_statement->tokens_.push_back("8080");
  nginx_config_.statements_.push_back(port_statement);

  auto static_statement = std::make_shared<NginxConfigStatement>();
  static_statement->tokens_.push_back("location");
  static_statement->tokens_.push_back("/static");
  static_statement->tokens_.push_back("StaticHandler");
  
  // Create child block with various typed arguments
  auto static_child_block = std::make_unique<NginxConfig>();
  
  // String arg
  auto root_statement = std::make_shared<NginxConfigStatement>();
  root_statement->tokens_.push_back("root");
  root_statement->tokens_.push_back("/path/to/files");
  static_child_block->statements_.push_back(root_statement);
  
  // Integer arg
  auto cache_statement = std::make_shared<NginxConfigStatement>();
  cache_statement->tokens_.push_back("cache_time");
  cache_statement->tokens_.push_back("3600");
  static_child_block->statements_.push_back(cache_statement);
  
  // Boolean arg - true
  auto enabled_statement = std::make_shared<NginxConfigStatement>();
  enabled_statement->tokens_.push_back("cache_enabled");
  enabled_statement->tokens_.push_back("true");
  static_child_block->statements_.push_back(enabled_statement);
  
  // Boolean arg - false
  auto debug_statement = std::make_shared<NginxConfigStatement>();
  debug_statement->tokens_.push_back("debug_mode");
  debug_statement->tokens_.push_back("false");
  static_child_block->statements_.push_back(debug_statement);
  
  static_statement->child_block_ = std::move(static_child_block);
  nginx_config_.statements_.push_back(static_statement);

  EXPECT_TRUE(config_.ParseConfig(nginx_config_));
  EXPECT_EQ(config_.locations().size(), 2);
  
  const auto& location = config_.locations()[0];
  EXPECT_EQ(location.args.size(), 4);
  
  // Verify string argument
  EXPECT_TRUE(std::holds_alternative<std::string>(location.args.at("root")));
  EXPECT_EQ(std::get<std::string>(location.args.at("root")), "/path/to/files");
  
  // Verify integer argument
  EXPECT_TRUE(std::holds_alternative<int>(location.args.at("cache_time")));
  EXPECT_EQ(std::get<int>(location.args.at("cache_time")), 3600);
  
  // Verify boolean arguments
  EXPECT_TRUE(std::holds_alternative<bool>(location.args.at("cache_enabled")));
  EXPECT_TRUE(std::get<bool>(location.args.at("cache_enabled")));
  
  EXPECT_TRUE(std::holds_alternative<bool>(location.args.at("debug_mode")));
  EXPECT_FALSE(std::get<bool>(location.args.at("debug_mode")));
}

TEST_F(ServerConfigTest, CreateHandlerRegistrations) {
  // Set up valid configuration
  auto port_statement = std::make_shared<NginxConfigStatement>();
  port_statement->tokens_.push_back("listen");
  port_statement->tokens_.push_back("8080");
  nginx_config_.statements_.push_back(port_statement);

  auto echo_statement = std::make_shared<NginxConfigStatement>();
  echo_statement->tokens_.push_back("location");
  echo_statement->tokens_.push_back("/echo");
  echo_statement->tokens_.push_back("EchoHandler");
  nginx_config_.statements_.push_back(echo_statement);

  // Health Handler
  auto health_statement = std::make_shared<NginxConfigStatement>();
  health_statement->tokens_.push_back("location");
  health_statement->tokens_.push_back("/health");
  health_statement->tokens_.push_back("HealthHandler");
  nginx_config_.statements_.push_back(health_statement);

  // Static handler
  auto static_statement = std::make_shared<NginxConfigStatement>();
  static_statement->tokens_.push_back("location");
  static_statement->tokens_.push_back("/static");
  static_statement->tokens_.push_back("StaticHandler");
  
  auto static_child_block = std::make_unique<NginxConfig>();
  auto root_statement = std::make_shared<NginxConfigStatement>();
  root_statement->tokens_.push_back("root");
  root_statement->tokens_.push_back("/path/to/files");
  static_child_block->statements_.push_back(root_statement);
  static_statement->child_block_ = std::move(static_child_block);
  
  nginx_config_.statements_.push_back(static_statement);

  // URL Shortener Handler
  auto url_shortener_statement = std::make_shared<NginxConfigStatement>();
  url_shortener_statement->tokens_.push_back("location");
  url_shortener_statement->tokens_.push_back("/shorten");
  url_shortener_statement->tokens_.push_back("URLShortenerHandler");
  // Add required arguments as a child block
  auto url_shortener_child_block = std::make_unique<NginxConfig>();
  auto serving_path_stmt = std::make_shared<NginxConfigStatement>();
  serving_path_stmt->tokens_.push_back("serving_path");
  serving_path_stmt->tokens_.push_back("/shorten");
  url_shortener_child_block->statements_.push_back(serving_path_stmt);

  auto upload_dir_stmt = std::make_shared<NginxConfigStatement>();
  upload_dir_stmt->tokens_.push_back("upload_dir");
  upload_dir_stmt->tokens_.push_back("/tmp/uploads");
  url_shortener_child_block->statements_.push_back(upload_dir_stmt);

  auto db_path_stmt = std::make_shared<NginxConfigStatement>();
  db_path_stmt->tokens_.push_back("db_path");
  db_path_stmt->tokens_.push_back("/tmp/urlshortener.db");
  url_shortener_child_block->statements_.push_back(db_path_stmt);

  auto base_url_stmt = std::make_shared<NginxConfigStatement>();
  base_url_stmt->tokens_.push_back("base_url");
  base_url_stmt->tokens_.push_back("http://localhost:8080");
  url_shortener_child_block->statements_.push_back(base_url_stmt);

  url_shortener_statement->child_block_ = std::move(url_shortener_child_block);
  nginx_config_.statements_.push_back(url_shortener_statement);

  EXPECT_TRUE(config_.ParseConfig(nginx_config_));

  auto registrations = config_.CreateHandlerRegistrations();
  EXPECT_EQ(registrations.size(), 5);
  
  // Check echo handler registration
  ASSERT_TRUE(registrations.find("/echo") != registrations.end());
  EXPECT_EQ(registrations["/echo"].handler_name, "EchoHandler");
  EXPECT_TRUE(registrations["/echo"].args.empty());
  
   // Check health handler registration
  ASSERT_TRUE(registrations.find("/health") != registrations.end());
  EXPECT_EQ(registrations["/health"].handler_name, "HealthHandler");
  EXPECT_TRUE(registrations["/health"].args.empty());
  
  // Check static handler registration
  ASSERT_TRUE(registrations.find("/static") != registrations.end());
  EXPECT_EQ(registrations["/static"].handler_name, "StaticHandler");
  EXPECT_EQ(registrations["/static"].args.size(), 2);
  EXPECT_EQ(registrations["/static"].args[0], "/static");
  EXPECT_EQ(registrations["/static"].args[1], "/path/to/files");

  // Check URL shortener handler registration
  ASSERT_TRUE(registrations.find("/shorten") != registrations.end());
  EXPECT_EQ(registrations["/shorten"].handler_name, "URLShortenerHandler");
  // Should have 4 arguments: serving_path, upload_dir, db_path, base_url
  EXPECT_EQ(registrations["/shorten"].args.size(), 4);
  EXPECT_EQ(registrations["/shorten"].args[0], "/shorten");
  EXPECT_EQ(registrations["/shorten"].args[1], "/tmp/uploads");
  EXPECT_EQ(registrations["/shorten"].args[2], "/tmp/urlshortener.db");
  EXPECT_EQ(registrations["/shorten"].args[3], "http://localhost:8080");
  
}

TEST_F(ServerConfigTest, ParseQuotedArgumentThrows) {
  // Create config with port
  auto port_statement = std::make_shared<NginxConfigStatement>();
  port_statement->tokens_.push_back("listen");
  port_statement->tokens_.push_back("8080");
  nginx_config_.statements_.push_back(port_statement);

  // Static handler with quoted root argument
  auto static_statement = std::make_shared<NginxConfigStatement>();
  static_statement->tokens_.push_back("location");
  static_statement->tokens_.push_back("/static");
  static_statement->tokens_.push_back("StaticHandler");
  auto static_child_block = std::make_unique<NginxConfig>();
  auto root_statement = std::make_shared<NginxConfigStatement>();
  root_statement->tokens_.push_back("root");
  root_statement->tokens_.push_back("\"/quoted/path\""); // Quoted string
  static_child_block->statements_.push_back(root_statement);
  static_statement->child_block_ = std::move(static_child_block);
  nginx_config_.statements_.push_back(static_statement);

  EXPECT_THROW({
    config_.ParseConfig(nginx_config_);
  }, std::invalid_argument);
}
TEST_F(ServerConfigTest, ParseApiHandler) {
  auto port_stmt = std::make_shared<NginxConfigStatement>();
  port_stmt->tokens_ = {"listen", "8080"};
  nginx_config_.statements_.push_back(port_stmt);

  auto api_stmt = std::make_shared<NginxConfigStatement>();
  api_stmt->tokens_ = {"location", "/api", "ApiHandler"};
  auto api_block = std::make_unique<NginxConfig>();
  auto data_path_stmt = std::make_shared<NginxConfigStatement>();
  data_path_stmt->tokens_ = {"data_path", "/tmp/data"};
  api_block->statements_.push_back(data_path_stmt);
  api_stmt->child_block_ = std::move(api_block);
  nginx_config_.statements_.push_back(api_stmt);

  ASSERT_TRUE(config_.ParseConfig(nginx_config_));
  auto registrations = config_.CreateHandlerRegistrations();
  EXPECT_EQ(registrations["/api"].handler_name, "ApiHandler");
}
