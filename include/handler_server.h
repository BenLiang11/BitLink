#ifndef HANDLER_SERVER_H
#define HANDLER_SERVER_H

#include "server.h"
#include "handlers/base_handler.h"
#include <map>
#include <memory>
#include "handler_dispatcher.h"

class handler_server : public server {
public:
    handler_server(boost::asio::io_context& io_context, 
                  short port,
                  const HandlerDispatcher& handler_dispatcher);

    void handle_accept(session* new_session, const boost::system::error_code& error) override;
    
      
private:
    boost::asio::io_context& io_context_;
    HandlerDispatcher handler_dispatcher_;
};

#endif // HANDLER_SERVER_H 