#include "session.h"
#include "handlers/echo_handler.h"
#include <boost/bind.hpp>

// Constructor for socket
session::session(boost::asio::io_service& io_service)
    : socket_(io_service),
      handler_(std::make_shared<EchoHandler>()) // Default to EchoHandler
{
}

// Returns reference to socket
tcp::socket& session::socket()
{
  return socket_;
}

void session::set_handler(std::shared_ptr<RequestHandler> handler)
{
  handler_ = handler;
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

    // Create a Request object from the raw data
    Request request(request_data);
    
    // Create a Response object
    Response response;
    
    // Handle the request using the appropriate handler
    if (handler_) {
      handler_->HandleRequest(request, &response);
    } else {
      // Fallback if no handler is set
      response.set_status(Response::OK);
      response.set_header("Content-Type", "text/plain");
      response.set_body(request_data);
      response.set_header("Connection", "close");
    }
    
    // Convert the response to a string
    std::string response_str = response.to_string();
    
    // Write the response
    boost::asio::async_write(socket_,
        boost::asio::buffer(response_str),
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
