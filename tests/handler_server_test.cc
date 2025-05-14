#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "handler_server.h"
#include "session.h"
#include "handler_dispatcher.h"
#include <map>
#include <memory>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
using ::testing::Return;
using ::testing::_;
class HandlerServerTest : public ::testing::Test {
protected:
  boost::asio::io_service io_service;
  short port = 8080;
  std::map<std::string, HandlerRegistration> empty_handlers;
  HandlerDispatcher handler_dispatcher{empty_handlers};
};
// Test that we can construct a handler_server
TEST_F(HandlerServerTest, ConstructorInitializesHandlerServer) {
  EXPECT_NO_THROW(handler_server s(io_service, port, handler_dispatcher));
}
// Test handle_accept success path
TEST_F(HandlerServerTest, HandleAcceptSuccessInvokesSessionStart) {
  handler_server s(io_service, port, handler_dispatcher);
  // Create a spy session to verify start() is called
  struct SpySession : session {
    SpySession(boost::asio::io_service& io, const HandlerDispatcher& handler_dispatcher) 
      : session(io, handler_dispatcher), started(false) {}
    void start() override { started = true; }
    bool started;
  };
  auto* spy = new SpySession(io_service, handler_dispatcher);
  boost::system::error_code ec; // default-constructed = no error
  // Should take the (!error) path and call spy->start()
  s.handle_accept(spy, ec);
  EXPECT_TRUE(spy->started);
  // Since handle_accept does not delete on success, we clean up
  delete spy;
}
// Test handle_accept error path
TEST_F(HandlerServerTest, HandleAcceptErrorDeletesSession) {
  handler_server s(io_service, port, handler_dispatcher);
  // We allocate a session that will be deleted by handle_accept on error
  session* doomed = new session(io_service, handler_dispatcher);
  boost::system::error_code ec = 
      boost::asio::error::make_error_code(boost::asio::error::operation_aborted);
  // If this crashes or double-frees, the test will fail.
  EXPECT_NO_THROW(s.handle_accept(doomed, ec));
  // doomed is now gone; do not touch it
}
// Test that handle_accept creates a new session and sets up another accept
TEST_F(HandlerServerTest, HandleAcceptCreatesNewSession) {
  // This test is tricky because we'd ideally want to verify that 
  // acceptor_.async_accept is called correctly, but that's internal.
  // We'll just verify it doesn't crash.
  
  handler_server s(io_service, port, handler_dispatcher);
  
  auto* new_session = new session(io_service, handler_dispatcher);
  boost::system::error_code ec; // no error
  
  EXPECT_NO_THROW(s.handle_accept(new_session, ec));
  
  // Clean up
  delete new_session;
}
