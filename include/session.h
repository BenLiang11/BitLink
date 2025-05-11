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

/**
 * @brief Handles an individual client session.
 * 
 * Responsible for managing the socket connection, reading requests,
 * dispatching them to the appropriate handler, and sending responses back.
 */
class session {
public:
  /**
   * @brief Constructs a new session with the given IO context and handler dispatcher.
   * 
   * @param io_context The asio IO context for async operations.
   * @param handler_dispatcher The dispatcher that matches requests to handlers.
   */
  session(boost::asio::io_service& io_context, const HandlerDispatcher& handler_dispatcher);
  
  /**
   * @brief Virtual destructor to ensure proper cleanup of derived classes.
   */
  virtual ~session() = default;
  
  /**
   * @brief Gets the socket associated with this session.
   * 
   * @return Reference to the socket.
   */
  tcp::socket& socket();
  
  /**
   * @brief Starts the session by beginning the async read operation.
   */
  virtual void start();
  
  /**
   * @brief Handles the completion of an async read operation.
   * 
   * This is where we process the request, dispatch it to the appropriate handler,
   * and begin sending the response.
   * 
   * @param error The error code, if any.
   * @param bytes_transferred The number of bytes read.
   */
  virtual void handle_read(const boost::system::error_code& error,
    size_t bytes_transferred);
  
  /**
   * @brief Handles the completion of an async write operation.
   * 
   * After a response is sent, we either start reading the next request
   * or clean up the session if there was an error.
   * 
   * @param error The error code, if any.
   */
  virtual void handle_write(const boost::system::error_code& error);
  
  /**
   * @brief Sets the buffer with data for testing.
   * 
   * @param data The data to copy into the buffer.
   * @param len The length of the data.
   */
  void set_buffer(const char* data, std::size_t len) {
    assert(len <= max_length);
    std::memcpy(data_, data, len);
  }

  /**
   * @brief Gets the last received request data.
   * 
   * @return The raw request data as a string.
   */
  const std::string& last_request() const {
    return request_data;
  }
  
private:
  tcp::socket socket_;
  enum { max_length = 8192 }; // Increased buffer size
  char data_[max_length];

  std::string request_data;
  const HandlerDispatcher& handler_dispatcher_;
};

#endif