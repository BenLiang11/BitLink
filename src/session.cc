#include "session.h"
#include "handlers/echo_handler.h"
#include <boost/bind.hpp>
#include "logger.h"
#include <boost/log/trivial.hpp>

// Constructor for socket

session::session(boost::asio::io_context& io_context, const HandlerDispatcher& handler_dispatcher)
    : socket_(io_context),
      handler_dispatcher_(handler_dispatcher)
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

    BOOST_LOG_TRIVIAL(info) << "Received request: " << request_data;

    // Create a Request object from the raw data
    Request request(request_data);

    // Get the appropriate handler for the request path
    std::shared_ptr<RequestHandler> handler = handler_dispatcher_.GetHandler(request.uri());
    set_handler(handler);

    // Create a Response object
    Response response;
    
    // Handle the request using the appropriate handler
    if (handler_) {
      handler_->HandleRequest(request, &response);
    } else {
      // 404 if no handler is set
      response.set_status(Response::NOT_FOUND);
      response.set_header("Content-Type", "text/html");
      response.set_body("<html><body><h1>404 Not Found</h1><p>The requested file could not be found.</p></body></html>");
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
    BOOST_LOG_TRIVIAL(error) << "Read failed: " << error.message();
    delete this;
  }
}

// Handle result of async write 
void session::handle_write(const boost::system::error_code& error)
{
  if (!error)
  {
    BOOST_LOG_TRIVIAL(info) << "Sent response back to client.";
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }
  else
  {
    BOOST_LOG_TRIVIAL(error) << "Write failed: " << error.message();
    delete this;
  }
}
