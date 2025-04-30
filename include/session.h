#ifndef SESSION_H
#define SESSION_H

#include <boost/asio.hpp>
#include <string>
#include <cassert>
#include <cstring>
#include <memory>
#include "handlers/base_handler.h"
#include "request.h"
#include "response.h"
#include "handler_dispatcher.h"

using boost::asio::ip::tcp;

class session {
public:

  session(boost::asio::io_service& io_context, const HandlerDispatcher& handler_dispatcher);
  
  /**
   * @brief Virtual destructor to ensure proper cleanup of derived classes.
   */
  virtual ~session() = default;
  
  tcp::socket& socket();
  
  /**
   * @brief Set the request handler for this session.
   * 
   * @param handler The request handler to use.
   */
  void set_handler(std::shared_ptr<RequestHandler> handler);
  
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
  enum { max_length = 8192 }; // Increased buffer size
  char data_[max_length];

  std::string request_data;
  std::shared_ptr<RequestHandler> handler_;
  const HandlerDispatcher& handler_dispatcher_;
};

#endif