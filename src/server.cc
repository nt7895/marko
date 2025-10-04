#include "server.h"
#include "session.h"
#include <boost/bind.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include "server_log.h"

using boost::asio::ip::tcp;

server::server(boost::asio::io_service& io_service, short port,
               const std::map<std::string, HandlerConfig>& handler_configs)
  : io_service_(io_service),
    acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
  
  // Initialize the handler registry with the configs
  if (!handler_registry_.Init(handler_configs)) {
    throw std::runtime_error("Failed to initialize handler registry");
  }
  
  // Start accepting connections
  start_accept();
}

void server::start_accept() {
  session* new_session = new session(io_service_, handler_registry_);
  acceptor_.async_accept(new_session->socket(),
      boost::bind(&server::handle_accept, this, new_session,
        boost::asio::placeholders::error));
}

void server::handle_accept(session* new_session, const boost::system::error_code& error) {
  server_log log;
  if (!error) {
    boost::asio::ip::tcp::endpoint remote_ep = new_session->socket().remote_endpoint();
    std::string client_ip = remote_ep.address().to_string();
    std::string client_port = std::to_string(remote_ep.port());
    log.log_new_client_connection(client_ip, client_port);
    
    new_session->start();
  } else {
    delete new_session;
  }
  start_accept();
}