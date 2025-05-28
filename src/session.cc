#include "session.h"
#include "handlers/echo_handler.h"
#include <boost/bind.hpp>
#include "logger.h"
#include <boost/log/trivial.hpp>

bool isValidHttpRequest(const std::string& request_data) {
    if (request_data.empty()) return false;
    
    // Check for basic HTTP request format
    if (request_data.find("HTTP/") == std::string::npos) return false;
    
    // Check for proper line endings
    if (request_data.find("\r\n") == std::string::npos && 
        request_data.find("\n") == std::string::npos) return false;
    
    // Check starts with valid method
    if (request_data.find("GET ") != 0 && 
        request_data.find("POST ") != 0 && 
        request_data.find("PUT ") != 0 && 
        request_data.find("DELETE ") != 0) return false;
    
    return true;
}
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
// No longer need to set_handler since we create a unique_ptr for each request
// void session::set_handler(std::shared_ptr<RequestHandler> handler)
// {
//   handler_ = handler;
// }
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
    // Add validation here
    if (!isValidHttpRequest(request_data)) {
        // Send 400 response
        auto response = std::make_unique<Response>();
        response->set_status(Response::BAD_REQUEST);
        response->set_header("Content-Type", "application/json");
        response->set_body("{\"error\": \"Bad Request\", \"message\": \"Malformed HTTP request\"}");
        
        std::string response_str = response->to_string();
        boost::asio::async_write(socket_, boost::asio::buffer(response_str),
            boost::bind(&session::handle_write, this, boost::asio::placeholders::error));
        return;
    }
    BOOST_LOG_TRIVIAL(info) << "Received request: " << request_data;
    // Create a Request object from the raw data
    Request request(request_data);
    // Get the appropriate handler for the request path
    // Instead of reusing a shared_ptr, create a new unique_ptr for each request
    std::unique_ptr<RequestHandler> handler = handler_dispatcher_.CreateHandlerForRequest(request);
    // Create a Response object
    std::unique_ptr<Response> response;
    BOOST_LOG_TRIVIAL(info) << "Handling request: " << request.uri();
    // Handle the request using the appropriate handler
    if (handler) {
      response = handler->handle_request(request);
      BOOST_LOG_TRIVIAL(info) << "Successfully handled request: " << request.uri();
    } else {
      // 404 if no handler is available
      response = std::make_unique<Response>();
      response->set_status(Response::NOT_FOUND);
      response->set_header("Content-Type", "text/html");
      response->set_body("<html><body><h1>404 Not Found</h1><p>The requested file could not be found.</p></body></html>");
      response->set_header("Connection", "close");
      BOOST_LOG_TRIVIAL(info) << "Unsuccessfully handled request --> 404 Response:  " << request.uri();
    }
     // Log the response metrics
    BOOST_LOG_TRIVIAL(info)
    << "[ResponseMetrics]"
    << " code:" << static_cast<int>(response->status())
    << " method:" << request.method()
    << " path:" << request.uri()
    << " handler:" << (handler ? typeid(*handler).name() : "none");





    // Convert the response to a string
    std::string response_str = response->to_string();
    BOOST_LOG_TRIVIAL(info) << "Sending response: " << response_str;
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