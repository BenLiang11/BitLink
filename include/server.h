#ifndef SERVER_H
#define SERVER_H

#include "session.h"
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class server
{
public:
  server(boost::asio::io_service& io_service, short port);

  virtual void start_accept();

  virtual void handle_accept(session* new_session,
    const boost::system::error_code& error);

protected:
  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;
};

#endif