#include "gtest/gtest.h"
#include "server.h"
#include "session.h"
#include "handler_dispatcher.h"
#include <map>
#include <memory>
#include "handlers/base_handler.h"

class ServerTest : public ::testing::Test {
 protected:
  boost::asio::io_context io_context;
  short port = 8080;
  // Create an empty handler dispatcher for testing
  std::map<std::string, std::shared_ptr<RequestHandler>> empty_handlers;
  HandlerDispatcher handler_dispatcher{empty_handlers};
};

// 1) Just constructing the server will call start_accept() once
TEST_F(ServerTest, ConstructionInvokesStartAccept) {

  EXPECT_NO_THROW(server s(io_context, port, handler_dispatcher));
}

// 2) We can also call start_accept() by hand to cover its body
TEST_F(ServerTest, CanCallStartAccept) {

  server s(io_context, port, handler_dispatcher);
  EXPECT_NO_THROW(s.start_accept());
}

// 3) Success path: handle_accept should call session::start()
TEST_F(ServerTest, HandleAcceptSuccessInvokesSessionStart) {
  server s(io_context, port, handler_dispatcher);

  // A little spy to see that start() is invoked:
  struct SpySession : session {
    SpySession(boost::asio::io_context& io, const HandlerDispatcher& handler_dispatcher) 
      : session(io, handler_dispatcher), started(false) {}
    void start() override { started = true; }
    bool started;
  };


  auto* spy = new SpySession(io_context, handler_dispatcher);
  boost::system::error_code ec;  // default-constructed == no error

  // should take the (!error) path and call spy->start()
  EXPECT_NO_THROW(s.handle_accept(spy, ec));
  EXPECT_TRUE(spy->started);

  // since handle_accept does *not* delete on success, we clean up here
  delete spy;
}

// 4) Error path: handle_accept should delete the session
TEST_F(ServerTest, HandleAcceptErrorDeletesSession) {
  server s(io_context, port, handler_dispatcher);

  // We allocate a plain session*, and on error handle_accept() must delete it
  session* doomed = new session(io_context, handler_dispatcher);
  boost::system::error_code ec =
      boost::asio::error::make_error_code(boost::asio::error::operation_aborted);

  // If this crashes or double‐frees, the test will fail.  This covers the `else delete new_session;` line.
  EXPECT_NO_THROW(s.handle_accept(doomed, ec));

  // doomed is now gone; do not touch it.
}
