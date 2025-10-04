#pragma once
#include <boost/asio.hpp>
#include "request_handler_registry.h"

class server {
public:
  server(boost::asio::io_service& io_service, short port, 
         const std::map<std::string, HandlerConfig>& handler_configs);

private:
  void start_accept();
  void handle_accept(class session* new_session, const boost::system::error_code& error);

  boost::asio::io_service& io_service_;
  boost::asio::ip::tcp::acceptor acceptor_;
  http::server::RequestHandlerRegistry handler_registry_;
};