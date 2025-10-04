#ifndef HTTP_REQUEST_HANDLER_REGISTRY_H
#define HTTP_REQUEST_HANDLER_REGISTRY_H

#include <memory>
#include <string>
#include <map>
#include <functional>
#include "request_handler.hpp"
#include "config_parser.h"

namespace http {
namespace server {

// Forward declare
class RequestHandler;

// Factory function type definition
typedef std::function<RequestHandler*(const std::string&, const NginxConfig*)> RequestHandlerFactory;

// Registry class to create request handlers based on configuration
class RequestHandlerRegistry {
public:
    RequestHandlerRegistry() {}
    virtual ~RequestHandlerRegistry() {}
    
    // Initialize the registry with handler configurations
    bool Init(const std::map<std::string, HandlerConfig>& handler_configs);
    
    // Create a handler for the given request URI
    std::unique_ptr<RequestHandler> CreateHandler(const std::string& uri, std::string& handler_name);
    
    // Static method to register handler factories - ensures the map exists
    static bool RegisterHandler(const std::string& name, RequestHandlerFactory factory);
    
    // Get access to the factory map (Meyer's Singleton pattern)
    static std::map<std::string, RequestHandlerFactory>& GetFactoryMap();
    
private:
    // Map of URI prefixes to handler configs
    std::map<std::string, HandlerConfig> handler_configs_;
    
    // Find the best matching path prefix for a URI
    std::string FindBestMatch(const std::string& uri) const;
};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HANDLER_REGISTRY_H