#ifndef SERVER_H
#define SERVER_H

#include "session.h"
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class server
{
public:

  server(boost::asio::io_context& io_context, short port, const HandlerDispatcher& handler_dispatcher);

  virtual void start_accept();

  virtual void handle_accept(session* new_session,
    const boost::system::error_code& error);

protected:
  boost::asio::io_context& io_context_;
  tcp::acceptor acceptor_;
  HandlerDispatcher handler_dispatcher_;
};

#endif