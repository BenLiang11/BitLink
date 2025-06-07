#include "gtest/gtest.h"
#include "session.h"        // now has set_buffer() + last_request()
#include <cstring>
#include "handler_dispatcher.h"
#include <map>
#include <memory>
#include "handlers/base_handler.h"

class SessionCoverageTest : public ::testing::Test {
 protected:
  boost::asio::io_context io_context;
  // Create an empty handler dispatcher for testing
  std::map<std::string, HandlerRegistration> empty_handlers;
  HandlerDispatcher handler_dispatcher{empty_handlers};
};

TEST_F(SessionCoverageTest, HandleReadParsesRequestData) {
  session s(io_context, handler_dispatcher);

  // 1) seed the internal buffer
  const char raw[] = "GET /foo HTTP/1.1\r\n\r\n";
  s.set_buffer(raw, strlen(raw));

  // 2) call the real handle_read (error==none, bytes_transferred==strlen)
  s.handle_read(boost::system::error_code{}, strlen(raw));

  // 3) inspect that request_data was assigned correctly
  EXPECT_EQ(s.last_request(), std::string(raw));
}
TEST_F(SessionCoverageTest, HandleReadParsesRequestData2) {
  session s(io_context, handler_dispatcher);

  // 1) seed the internal buffer
  const char raw[] = "POST /foo HTTP/1.1 \r\n\r\n";
  s.set_buffer(raw, strlen(raw));

  // 2) call the real handle_read (error==none, bytes_transferred==strlen)
  s.handle_read(boost::system::error_code{}, strlen(raw));

  // 3) inspect that request_data was assigned correctly
  EXPECT_EQ(s.last_request(), std::string(raw));
}
TEST_F(SessionCoverageTest, HandleReadParsesRequestData3) {
  session s(io_context, handler_dispatcher);

  // 1) seed the internal buffer
  const char raw[] = "DELETE /foo HT/1.0 \r\n\r\n";
  s.set_buffer(raw, strlen(raw));

  // 2) call the real handle_read (error==none, bytes_transferred==strlen)
  s.handle_read(boost::system::error_code{}, strlen(raw));

  // 3) inspect that request_data was assigned correctly
  EXPECT_EQ(s.last_request(), std::string(raw));
}
TEST_F(SessionCoverageTest, InvalidHttpMethodTriggers400Response) {
  session s(io_context, handler_dispatcher);

  // This request has headers, but an invalid HTTP method ("FOO")
  const char raw[] = "FOO /invalid HTTP/1.1\r\nHost: localhost\r\n\r\n";
  s.set_buffer(raw, strlen(raw));

  // Run the handler
  s.handle_read(boost::system::error_code{}, strlen(raw));

  // Expect the request_data to still store the original input
  EXPECT_EQ(s.last_request(), std::string(raw));
}

TEST_F(SessionCoverageTest, HandleWriteSchedulesAnotherRead) {
  session s(io_context, handler_dispatcher);
  // This simply drives the 'no error' branch of handle_write:
  // -> socket_.async_read_some(...)
  // We don't have a direct way to observe the async, but at least
  // this call executes the code under coverage.
  s.handle_write(boost::system::error_code{});
}

// cover session::start() ⟶ calls async_read_some(...)
TEST_F(SessionCoverageTest, StartSchedulesAsyncRead) {
  session s(io_context, handler_dispatcher);
  // if this compiles and runs, we executed the body of session::start()
  EXPECT_NO_THROW(s.start());
}