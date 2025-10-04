#pragma once
#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include "request_handler.hpp"
#include "request_handler_registry.h"

class server_config_test; // Forward declaration for your tests
class server_request_parser_test;
class server_session_test;
class server_session_logic_test;

class session {
public:
  session(boost::asio::io_service& io_service, 
          http::server::RequestHandlerRegistry& handler_registry);
  boost::asio::ip::tcp::socket& socket();
  void start();

  // ------------------------------------------------------------------
  // Unit-testable core logic 
  enum class SessionAction { ReadAgain, WriteResponse, Close };

  // Processes one read. Returns Close on error, or WriteResponse.
  SessionAction process_read(const boost::system::error_code& error,
                            const char* begin,
                            const char* end,
                            std::vector<boost::asio::const_buffer>& out_buffers);

  // Processes one write. Returns ReadAgain on success, Close on error.
  SessionAction process_write(const boost::system::error_code& error);

private:
  void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
  void handle_write(const boost::system::error_code& error);

  friend class server_config_test;
  friend class server_session_test; 
  friend class server_request_parser_test;
  friend class server_session_logic_test;
  // Additional friends as necessary for other tests

  boost::asio::ip::tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
  http::server::RequestHandlerRegistry& handler_registry_;
};