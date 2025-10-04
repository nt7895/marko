#include "echo_handler.hpp"
#include <sstream>

namespace http {
namespace server {

std::unique_ptr<reply> EchoHandler::handle_request(const request& request) {
    // Process request and generate content
    std::ostringstream oss;
    oss << request.method << " " << request.uri << " HTTP/" 
        << request.http_version_major << "." << request.http_version_minor << "\r\n";
    
    for (const auto& header : request.headers) {
        oss << header.name << ": " << header.value << "\r\n";
    }
    
    // blank line to separate headers from body
    oss << "\r\n";
    
    std::string content = oss.str();
    
    // Define content type header
    std::vector<header> headers;
    header content_type;
    content_type.name = "Content-Type";
    content_type.value = "text/plain";
    headers.push_back(content_type);
    
    // Create and return response
    return BuildResponse(reply::ok, content, headers);
}

bool EchoHandler::Register() {
    std::cout << "Registering EchoHandler" << std::endl;
    return RequestHandlerRegistry::RegisterHandler("EchoHandler", EchoHandler::Init);
}
  
static struct EchoHandlerRegistrar {
    EchoHandlerRegistrar() {
        std::cout << "EchoHandlerRegistrar constructor called" << std::endl;
        EchoHandler::Register();
    }
} echoHandlerRegistrar;
} // namespace server
} // namespace http