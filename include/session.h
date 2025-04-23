#ifndef SESSION_H
#define SESSION_H

#include <boost/asio.hpp>
#include <string>
#include <cassert>
#include <cstring>

using boost::asio::ip::tcp;

class session {
public:
  session(boost::asio::io_service& io_service);
  tcp::socket& socket();
  virtual void start();
  virtual void handle_read(const boost::system::error_code& error,
    size_t bytes_transferred);
  virtual void handle_write(const boost::system::error_code& error);
  void set_buffer(const char* data, std::size_t len) {
    assert(len <= max_length);
    std::memcpy(data_, data, len);
  }

  // after handle_read, this contains exactly the request
  const std::string& last_request() const {
    return request_data;
  }
private:

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];

  std::string request_data;
  
};

#endif