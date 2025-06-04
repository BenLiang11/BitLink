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
    // Accumulate data from this read
    std::string new_data(data_, bytes_transferred);
    request_data += new_data;
    
    // Check if we have complete HTTP headers (headers end with \r\n\r\n or \n\n)
    size_t header_end = request_data.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        header_end = request_data.find("\n\n");
    }
    
    bool headers_complete = (header_end != std::string::npos);
    
    // Only validate the request format when we have complete headers
    if (headers_complete && request_data.find("HTTP/") != std::string::npos) {
        // Extract just the first line for validation
        size_t first_line_end = request_data.find('\n');
        std::string first_line = (first_line_end != std::string::npos) ? 
                                request_data.substr(0, first_line_end) : request_data;
        
        // Validate the first line contains a valid HTTP method
        if (!isValidHttpRequest(first_line + " HTTP/1.1\r\n\r\n")) {
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
    }
    
    // Check if we have a complete request
    bool request_complete = false;
    
    if (headers_complete) {
        // Parse headers to get Content-Length
        std::string headers = request_data.substr(0, header_end);
        size_t content_length = 0;
        
        // Look for Content-Length header
        size_t cl_pos = headers.find("Content-Length:");
        if (cl_pos == std::string::npos) {
            cl_pos = headers.find("content-length:");
        }
        
        if (cl_pos != std::string::npos) {
            size_t cl_start = headers.find(':', cl_pos) + 1;
            size_t cl_end = headers.find('\n', cl_start);
            if (cl_end != std::string::npos) {
                std::string cl_str = headers.substr(cl_start, cl_end - cl_start);
                // Remove whitespace and \r
                cl_str.erase(0, cl_str.find_first_not_of(" \t\r"));
                cl_str.erase(cl_str.find_last_not_of(" \t\r") + 1);
                content_length = std::stoul(cl_str);
            }
        }
        
        // Calculate expected total request size
        size_t header_separator_length = (request_data.find("\r\n\r\n") != std::string::npos) ? 4 : 2;
        size_t expected_total_size = header_end + header_separator_length + content_length;
        
        // Check if we have the complete request
        if (request_data.size() >= expected_total_size) {
            request_complete = true;
            // Trim to exact size to remove any extra data
            if (request_data.size() > expected_total_size) {
                request_data = request_data.substr(0, expected_total_size);
            }
        }
    } else if (request_data.find("HTTP/") != std::string::npos && 
               request_data.find("GET ") == 0) {
        // For GET requests and other requests without body, check if headers are complete
        request_complete = headers_complete;
    }
    
    // Only process the request if it's complete
    if (request_complete) {
        BOOST_LOG_TRIVIAL(info) << "Received complete request, size: " << request_data.size() << " bytes";
        
        // Create a Request object from the raw data
        Request request(request_data);
        
        // Get the appropriate handler for the request path
        std::unique_ptr<RequestHandler> handler = handler_dispatcher_.CreateHandlerForRequest(request);
        
        // Create a Response object
        std::unique_ptr<Response> response;
        
        // Handle the request using the appropriate handler
        if (handler) {
          response = handler->handle_request(request);
        } else {
          // 404 if no handler is available
          response = std::make_unique<Response>();
          response->set_status(Response::NOT_FOUND);
          response->set_header("Content-Type", "text/html");
          response->set_body("<html><body><h1>404 Not Found</h1><p>The requested file could not be found.</p></body></html>");
          response->set_header("Connection", "close");
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
        
        // Write the response
        boost::asio::async_write(socket_,
            boost::asio::buffer(response_str),
            boost::bind(&session::handle_write, this,
              boost::asio::placeholders::error));
              
        // Note: Keep request_data as-is for last_request() compatibility
        // The next request will overwrite it when handle_write calls start reading again
    } else {
        // Request is not complete, continue reading
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
            boost::bind(&session::handle_read, this,
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred));
    }
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
    // Clear request_data for the next request
    request_data.clear();
    // Start reading the next request
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