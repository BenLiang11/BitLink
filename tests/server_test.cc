#include "gtest/gtest.h"
#include "server.h"
#include "session.h"

class ServerTest : public ::testing::Test {
 protected:
  boost::asio::io_context io_context;
  short port = 8080;
};

// 1) Just constructing the server will call start_accept() once
TEST_F(ServerTest, ConstructionInvokesStartAccept) {
  EXPECT_NO_THROW(server s(io_context, port));
}

// 2) We can also call start_accept() by hand to cover its body
TEST_F(ServerTest, CanCallStartAccept) {
  server s(io_context, port);
  EXPECT_NO_THROW(s.start_accept());
}

// 3) Success path: handle_accept should call session::start()
TEST_F(ServerTest, HandleAcceptSuccessInvokesSessionStart) {
  server s(io_context, port);

  // A little spy to see that start() is invoked:
  struct SpySession : session {
    SpySession(boost::asio::io_context& io) : session(io), started(false) {}
    void start() override { started = true; }
    bool started;
  };

  auto* spy = new SpySession(io_context);
  boost::system::error_code ec;  // default-constructed == no error

  // should take the (!error) path and call spy->start()
  EXPECT_NO_THROW(s.handle_accept(spy, ec));
  EXPECT_TRUE(spy->started);

  // since handle_accept does *not* delete on success, we clean up here
  delete spy;
}

// 4) Error path: handle_accept should delete the session
TEST_F(ServerTest, HandleAcceptErrorDeletesSession) {
  server s(io_context, port);

  // We allocate a plain session*, and on error handle_accept() must delete it
  session* doomed = new session(io_context);
  boost::system::error_code ec =
      boost::asio::error::make_error_code(boost::asio::error::operation_aborted);

  // If this crashes or double‐frees, the test will fail.  This covers the `else delete new_session;` line.
  EXPECT_NO_THROW(s.handle_accept(doomed, ec));

  // doomed is now gone; do not touch it.
}
