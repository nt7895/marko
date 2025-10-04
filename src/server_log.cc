
#include "server_log.h"
#include <algorithm>
#include <regex>

server_log::server_log() {}

void server_log::start_logging(std::string file_name) {
    std::string log_format = "[%TimeStamp%] [%ThreadID%] [%Severity%]: %Message%";
    boost::log::add_common_attributes();
    boost::log::add_console_log(std::cout,
                                boost::log::keywords::format = log_format);
    boost::log::add_file_log(boost::log::keywords::file_name = file_name,
                            boost::log::keywords::open_mode = std::ios_base::app, 
                            boost::log::keywords::format = log_format,
                            boost::log::keywords::rotation_size = 10485760,
                            boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0));
}

void server_log::log_server_startup(std::string port_num) {
    BOOST_LOG_TRIVIAL(info) << "[ServerStart] message:\"Server has started running\" port:" + port_num;
}

void server_log::log_config_parser_status(bool status) {
    if (status)
        BOOST_LOG_TRIVIAL(info) << "[ConfigFile] message:\"Server config file successfully parsed\"";
    else
        BOOST_LOG_TRIVIAL(error) << "[ConfigFile] message:\"Server config file was not successfully parsed. Exiting...\"";
}

void server_log::log_new_client_connection(std::string client_ip, std::string client_port) {
    BOOST_LOG_TRIVIAL(info) << "[ConnectionClose] message:\"Client has CONNECTED\" ip:" + client_ip 
                                + " port:" + client_port;
}

void server_log::log_close_client_connection(std::string client_ip, std::string client_port) {
    BOOST_LOG_TRIVIAL(info) << "[ConnectionClose] message:\"Client has DISCONNECTED\" ip:" + client_ip 
                                + " port:" + client_port;
}

void server_log::log_server_close() {
    BOOST_LOG_TRIVIAL(info) << "[ServerClose] message:\"Server has shutdown\"";
}

void server_log::log_request(http::server::request req, std::string client_ip, std::string client_port) {
    std::string message = "[RequestMetrics] message:\"Client sent a REQUEST to server\" request_method:" 
                            + req.method
                            + " request_path:" + req.uri
                            + " request_http_version:" + std::to_string(req.http_version_major) + "."+ std::to_string(req.http_version_minor)
                            + " request_body:" + req.body
                            + " ip:" + client_ip 
                            + " port:" + client_port;
    BOOST_LOG_TRIVIAL(info) << message;
}

void server_log::log_invalid_request(http::server::request req, std::string client_ip, std::string client_port) {
    std::string message = "[RequestMetrics] message:\"Client sent an INVALID REQUEST to server\" request_method:" 
                        + req.method
                        + " request_path:" + req.uri
                        + " request_http_version:" + std::to_string(req.http_version_major) + "."+ std::to_string(req.http_version_minor)
                        + " request_body:" + req.body
                        + " ip:" + client_ip 
                        + " port:" + client_port;
    BOOST_LOG_TRIVIAL(error) << message;
}

void server_log::log_reply(http::server::request req, http::server::reply rep, std::string handler_name, std::string client_ip, std::string client_port) {
    std::string full_response = http::server::status_strings::to_string(rep.status) + rep.content;
    
    // remove trailing return + newline
    full_response = std::regex_replace(full_response, std::regex("(\r\n)$"), "");

    // replace return + newlines with spaces
    full_response = std::regex_replace(full_response, std::regex("\r\n{1,}"), " ");
    full_response = std::regex_replace(full_response, std::regex("\r|\n"), " ");
    full_response = "\"" + full_response + "\"";

    std::string message = "[ResponseMetrics] message:\"Server sent a REPLY to client\" response_code:"
                            + std::to_string(static_cast<int>(rep.status)) 
                            + " full_response:" + full_response
                            + " request_handler:" + handler_name
                            + " request_method:" + req.method
                            + " request_path:" + req.uri
                            + " request_http_version:" + std::to_string(req.http_version_major) + "."+ std::to_string(req.http_version_minor)
                            + " request_body:" + req.body
                            + " ip:" + client_ip 
                            + " port:" + client_port ;
    BOOST_LOG_TRIVIAL(info) << message;
}