#include "session.h"
#include <boost/bind.hpp>

// Constructor for socket
session::session(boost::asio::io_service& io_service)
: socket_(io_service)
{
}

// Returns reference to socket
tcp::socket& session::socket()
{
  return socket_;
}

// Begins async read operation on socket
void session::start()
{
  socket_.async_read_some(boost::asio::buffer(data_, max_length),
      boost::bind(&session::handle_read, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

// Handles result of read
void session::handle_read(const boost::system::error_code& error,
    size_t bytes_transferred)
{
  if (!error)
  {
    request_data.assign(data_, bytes_transferred);

    std::string response =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain\r\n"
      "Content-Length: " + std::to_string(request_data.size()) + "\r\n"
      "Connection: close\r\n"
      "\r\n" +
      request_data;

    boost::asio::async_write(socket_,
        boost::asio::buffer(data_, bytes_transferred),
        boost::bind(&session::handle_write, this,
          boost::asio::placeholders::error));
  }
  else
  {
    delete this;
  }
}

// Handle result of async write 
void session::handle_write(const boost::system::error_code& error)
{
  if (!error)
  {
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }
  else
  {
    delete this;
  }
}
