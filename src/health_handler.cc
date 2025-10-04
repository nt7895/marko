#include "health_handler.h"
#include <sstream>
#include <iostream>

namespace http {
namespace server {

std::unique_ptr<reply> HealthHandler::handle_request(const request& request) {
    // ok payload
    std::string content = "OK\r\n";
    
    // Create and return response
    return BuildResponse(reply::ok, content);
}

bool HealthHandler::Register() {
    std::cout << "Registering HealthHandler" << std::endl;
    return RequestHandlerRegistry::RegisterHandler("HealthHandler", HealthHandler::Init);
}
  
static struct HealthHandlerRegistrar {
    HealthHandlerRegistrar() {
        std::cout << "HealthHandlerRegistrar constructor called" << std::endl;
        HealthHandler::Register();
    }
} healthHandlerRegistrar;
} // namespace server
} // namespace http