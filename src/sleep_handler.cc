#include "sleep_handler.h"
#include <sstream>
#include <unistd.h> 
#include <iostream> 
namespace http {
namespace server {

std::unique_ptr<reply> SleepHandler::handle_request(const request& request) {
    int sleep_time = 5;

    sleep(sleep_time);
    std::ostringstream oss;
    oss <<  "The request slept for " << sleep_time << " seconds\r\n" ;

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

bool SleepHandler::Register() {
    std::cout << "Registering SleepHandler" << std::endl;
    return RequestHandlerRegistry::RegisterHandler("SleepHandler", SleepHandler::Init);
}
  
static struct SleepHandlerRegistrar {
    SleepHandlerRegistrar() {
        std::cout << "SleepHandlerRegistrar constructor called" << std::endl;
        SleepHandler::Register();
    }
} sleepHandlerRegistrar;
} // namespace server
} // namespace http