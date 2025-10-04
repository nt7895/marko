#ifndef HTTP_HEALTH_HANDLER_HPP
#define HTTP_HEALTH_HANDLER_HPP

#include "request_handler.hpp"
#include "config_parser.h"
#include "request_handler_registry.h"

namespace http {
namespace server {

class HealthHandler : public RequestHandler {
public:
  static RequestHandler* Init(const std::string& path_prefix, const NginxConfig* config) {
    return new HealthHandler();
  }
  
  // Register handler with static initializer function
  static bool Register();
  
  std::unique_ptr<reply> handle_request(const request& request) override;
};

} // namespace server
} // namespace http

#endif // HTTP_HEALTH_HANDLER_HPP