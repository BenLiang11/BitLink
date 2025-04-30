#include "handler_server.h"
#include <boost/bind.hpp>
#include "handler_dispatcher.h"
#include <boost/log/trivial.hpp>

handler_server::handler_server(boost::asio::io_context& io_context, 
                             short port,
                             const HandlerDispatcher& handler_dispatcher)
    : server(io_context, port, handler_dispatcher), io_context_(io_context), handler_dispatcher_(handler_dispatcher) {}


void handler_server::handle_accept(session* new_session, const boost::system::error_code& error) {
    if (!error) {
        new_session->start();
        
        // Start accepting another connection
        new_session = new session(io_context_, handler_dispatcher_);
        acceptor_.async_accept(new_session->socket(),
            boost::bind(&handler_server::handle_accept, this, new_session,
                boost::asio::placeholders::error));
    } else {
        BOOST_LOG_TRIVIAL(error) << "Accept failed: " << error.message();
        delete new_session;
    }
}