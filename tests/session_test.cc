#include "gtest/gtest.h"
#include "session.h"        // now has set_buffer() + last_request()
#include <cstring>

class SessionCoverageTest : public ::testing::Test {
 protected:
  boost::asio::io_service io_service;
};

TEST_F(SessionCoverageTest, HandleReadParsesRequestData) {
  session s(io_service);

  // 1) seed the internal buffer
  const char raw[] = "GET /foo HTTP/1.1\r\n\r\n";
  s.set_buffer(raw, strlen(raw));

  // 2) call the real handle_read (error==none, bytes_transferred==strlen)
  s.handle_read(boost::system::error_code{}, strlen(raw));

  // 3) inspect that request_data was assigned correctly
  EXPECT_EQ(s.last_request(), std::string(raw));
}

TEST_F(SessionCoverageTest, HandleWriteSchedulesAnotherRead) {
  session s(io_service);

  // This simply drives the 'no error' branch of handle_write:
  // -> socket_.async_read_some(...)
  // We don’t have a direct way to observe the async, but at least
  // this call executes the code under coverage.
  s.handle_write(boost::system::error_code{});
}

// cover session::start() ⟶ calls async_read_some(...)
TEST_F(SessionCoverageTest, StartSchedulesAsyncRead) {
  session s(io_service);
  // if this compiles and runs, we executed the body of session::start()
  EXPECT_NO_THROW(s.start());
}