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
  boost::asio::io_context io_context;
  short port = 8080;
  std::map<std::string, HandlerRegistration> empty_handlers;
  HandlerDispatcher handler_dispatcher{empty_handlers};
};

// Test that we can construct a handler_server
TEST_F(HandlerServerTest, ConstructorInitializesHandlerServer) {
  EXPECT_NO_THROW(handler_server s(io_context, port, handler_dispatcher));
}

// Test handle_accept success path
TEST_F(HandlerServerTest, HandleAcceptSuccessInvokesSessionStart) {
  handler_server s(io_context, port, handler_dispatcher);
  
  struct SpySession : session {
    SpySession(boost::asio::io_context& io, const HandlerDispatcher& dispatcher)
      : session(io, dispatcher), started(false) {}
    void start() override { started = true; }
    bool started;
  };

  auto* spy = new SpySession(io_context, handler_dispatcher);
  boost::system::error_code ec;
  s.handle_accept(spy, ec);
  EXPECT_TRUE(spy->started);
  delete spy;
}

// Test handle_accept error path
TEST_F(HandlerServerTest, HandleAcceptErrorDeletesSession) {
  handler_server s(io_context, port, handler_dispatcher);
  session* doomed = new session(io_context, handler_dispatcher);
  boost::system::error_code ec = boost::asio::error::make_error_code(boost::asio::error::operation_aborted);
  EXPECT_NO_THROW(s.handle_accept(doomed, ec));
}

// Test that handle_accept creates a new session and sets up another accept
TEST_F(HandlerServerTest, HandleAcceptCreatesNewSession) {
  handler_server s(io_context, port, handler_dispatcher);
  auto* new_session = new session(io_context, handler_dispatcher);
  boost::system::error_code ec;
  EXPECT_NO_THROW(s.handle_accept(new_session, ec));
  delete new_session;
}
