#ifndef SERVER_LOG
#define SERVER_LOG

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <string>
#include "request.hpp"
#include "reply.hpp"
#include <algorithm>

class server_log {
    public:
        server_log();

        // sets logging message format and begins logging to stdout and file
        void start_logging(std::string file_name); 

        // returns true if config file can be correctly parsed
        void log_config_parser_status(bool status);

        // log for starting server
        void log_server_startup(std::string port_num);

        // log for new client connection
        void log_new_client_connection(std::string client_ip, std::string client_port);

        // TODO: log for closing client connection
        void log_close_client_connection(std::string client_ip, std::string client_port); 

        // log for closing server
        void log_server_close();

        // log for receiving requests
        void log_request(http::server::request req, std::string client_ip, std::string client_port); 

        // log for receiving INVALID requests
        void log_invalid_request(http::server::request req, std::string client_ip, std::string client_port); 

        // log for replying to requests
        void log_reply(http::server::request req, http::server::reply rep, std::string handler_name, std::string client_ip, std::string client_port);
};

#endif