#include "session.h"
#include <boost/bind.hpp>
#include <iostream>
#include "reply.hpp"
#include "request_parser.hpp"
#include "request.hpp"
#include "server_log.h"

using boost::asio::ip::tcp;

session::session(boost::asio::io_service& io_service, 
                 http::server::RequestHandlerRegistry& handler_registry)
  : socket_(io_service), handler_registry_(handler_registry) {
}

tcp::socket& session::socket() {
  return socket_;
}

void session::start() {
  socket_.async_read_some(boost::asio::buffer(data_, max_length),
      boost::bind(&session::handle_read, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

void session::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
  server_log log;
  if (!error) {
    boost::asio::ip::tcp::endpoint remote_ep = socket_.remote_endpoint();
    std::string client_ip = remote_ep.address().to_string();
    std::string client_port = std::to_string(remote_ep.port());
    std::string handler_name = "";

    http::server::request req;
    http::server::request_parser rp;
    auto req_parse_results = rp.http::server::request_parser::parse(req, data_, data_ + bytes_transferred);
    bool is_malformed = std::get<0>(req_parse_results); 

    std::unique_ptr<http::server::reply> rep = std::make_unique<http::server::reply>();
    if (is_malformed) { 
      // malformed request
      rep = rep->build_malformed_req_response();
      log.log_invalid_request(req, client_ip, client_port);
    } else {
      // well formed HTTP request

      // Checks for Content-Length header (request bodies)
      size_t content_length = 0;
      for (const auto& header : req.headers) {
        if (header.name == "Content-Length") {
          try {
            content_length = std::stoi(header.value);
          } catch (...) {
            // Invalid Content-Length, ignore
          }
          break;
        }
      }
      
      // Parses the request body
      if (content_length > 0) {
        rp.http::server::request_parser::parse_request_body(data_, bytes_transferred, req.body, content_length);
      }

    boost::asio::ip::tcp::endpoint remote_ep = socket_.remote_endpoint();
    std::string client_ip = remote_ep.address().to_string();
    std::string client_port = std::to_string(remote_ep.port());

    // Create handler for the request
    std::unique_ptr<http::server::RequestHandler> handler = handler_registry_.CreateHandler(req.uri, handler_name);
    
    // Log the request
    if (handler) {
      log.log_request(req, client_ip, client_port);
    } else {
      log.log_invalid_request(req, client_ip, client_port);
    }

      // Generate and send the reply
      rep = handler->handle_request(req);
    }
    std::vector<boost::asio::const_buffer> buffers = rep->to_buffers();
    boost::asio::async_write(socket_,
      buffers,
      boost::bind(&session::handle_write, this,
        boost::asio::placeholders::error));
    log.log_reply(req, *rep, handler_name, client_ip, client_port);

  } else {
    delete this;
  }
}

void session::handle_write(const boost::system::error_code& error) {
  if (!error) {
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  } else {
    delete this;
  }
}

// ------------------------------------------------------------------
// Public core implementations
session::SessionAction
session::process_read(const boost::system::error_code& error,
                      const char* begin,
                      const char* end,
                      std::vector<boost::asio::const_buffer>& out) {
  if (error) {
    return SessionAction::Close;
  }

  http::server::request req;
  http::server::request_parser rp;
  rp.http::server::request_parser::parse(req, begin, end);

  if (req.method == "GET" &&
      req.http_version_major == 1 &&
      req.http_version_minor == 1) {

      // Create an appropriate handler for this request
      std::string handler_name;
      auto handler = handler_registry_.CreateHandler(req.uri, handler_name);

      // Generate the reply
      auto rep = handler->handle_request(req);
      out = rep->to_buffers();
      return SessionAction::WriteResponse;
  } else {
    // For non-GET requests, clear the buffers as per the original behavior
    out.clear();
    return SessionAction::WriteResponse;
  }
}

// ------------------------------------------------------------------
// Public core write logic:
session::SessionAction
session::process_write(const boost::system::error_code& error) {
  if (!error) {
    return SessionAction::ReadAgain;
  } else {
    return SessionAction::Close;
  }
}