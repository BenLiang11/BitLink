#include "server.h"
#include <boost/bind.hpp> 
#include <iostream>
#include "session.h"
#include "logger.h"
#include <boost/log/trivial.hpp>


server::server(boost::asio::io_context& io_context, short port, const HandlerDispatcher& handler_dispatcher)
  : io_context_(io_context),
    acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
    handler_dispatcher_(handler_dispatcher)
{
  std::cout << "Server listening on port " << port << std::endl;
  init_logging();
  BOOST_LOG_TRIVIAL(info) << "Server listening on port " << port;
  start_accept();
}

void server::start_accept()
{
  session* new_session = new session(io_context_, handler_dispatcher_);
  acceptor_.async_accept(new_session->socket(),
      boost::bind(&server::handle_accept, this, new_session,
        boost::asio::placeholders::error));
}

void server::handle_accept(session* new_session,
  const boost::system::error_code& error)
{
if (!error)
{
  try {
    BOOST_LOG_TRIVIAL(info) << "Accepted new client connection: " 
                            << new_session->socket().remote_endpoint();
  } catch (const boost::system::system_error& e) {
    BOOST_LOG_TRIVIAL(warning) << "Failed to get remote endpoint: " << e.what();
  }

  new_session->start();
}
else
{
  BOOST_LOG_TRIVIAL(error) << "Failed to accept client connection: " << error.message();
  delete new_session;
}

start_accept();
}




