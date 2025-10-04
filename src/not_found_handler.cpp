#include "not_found_handler.hpp"

namespace http {
namespace server {

std::unique_ptr<reply> NotFoundHandler::handle_request(const request& request) {
  // Create a simple 404 error message
  std::string content = "404 Not Found\n";
  content += "The requested resource '" + request.uri + "' was not found on this server.";
  
  // Define headers for the response
  std::vector<header> headers;
  header content_type;
  content_type.name = "Content-Type";
  content_type.value = "text/plain";
  headers.push_back(content_type);
  
  // Create and return a 404 response
  return BuildResponse(reply::not_found, content, headers);
}

bool NotFoundHandler::Register() {
  std::cout << "Registering NotFoundHandler" << std::endl;
  return RequestHandlerRegistry::RegisterHandler("NotFoundHandler", NotFoundHandler::Init);
}

// Use function call to register, not static initialization
static struct NotFoundHandlerRegistrar {
  NotFoundHandlerRegistrar() {
      std::cout << "NotFoundHandlerRegistrar constructor called" << std::endl;
      NotFoundHandler::Register();
  }
} notFoundHandlerRegistrar;

} 
} 