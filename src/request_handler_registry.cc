#include "request_handler_registry.h"
#include <iostream>
#include "not_found_handler.hpp"

namespace http {
namespace server {

// Meyer's Singleton pattern to ensure the map exists when needed
std::map<std::string, RequestHandlerFactory>& RequestHandlerRegistry::GetFactoryMap() {
    static std::map<std::string, RequestHandlerFactory> factory_map_;
    return factory_map_;
}

bool RequestHandlerRegistry::RegisterHandler(const std::string& name, RequestHandlerFactory factory) {
    std::cout << "Registering handler: " << name << std::endl;
    GetFactoryMap()[name] = factory;
    return true;
}

bool RequestHandlerRegistry::Init(const std::map<std::string, HandlerConfig>& handler_configs) {
    std::cout << "Initializing handler registry with " << handler_configs.size() << " configs" << std::endl;
    std::cout << "Available handlers: " << GetFactoryMap().size() << std::endl;
    
    // Clear existing configurations
    handler_configs_.clear();
    
    // Deep copy each HandlerConfig with proper handling of unique_ptr
    for (const auto& [path, config] : handler_configs) {
        std::cout << "Configuring path: " << path << " with handler: " << config.type << std::endl;
        
        HandlerConfig new_config;
        new_config.type = config.type;
        
        // Deep copy the NginxConfig if it exists
        if (config.config) {
            new_config.config = std::make_unique<NginxConfig>(*config.config);
        }
        
        // Use move semantics to add to the map
        handler_configs_[path] = std::move(new_config);
    }
    
    return true;
}

std::string RequestHandlerRegistry::FindBestMatch(const std::string& uri) const {
    std::string best_match;
    
    for (const auto& [path_prefix, _] : handler_configs_) {
        // Check if this path is a prefix of the URI
        if (uri.compare(0, path_prefix.length(), path_prefix) == 0) {
            // If this is a longer match than our current best, use it
            if (path_prefix.length() > best_match.length()) {
                best_match = path_prefix;
            }
        }
    }
    
    return best_match;
}

std::unique_ptr<RequestHandler> RequestHandlerRegistry::CreateHandler(const std::string& uri, std::string& handler_name) {
    std::cout << "Creating handler for URI: " << uri << std::endl;
    
    // Find the best matching path prefix
    std::string path_prefix = FindBestMatch(uri);
    
    // If no match found, return 404 handler
    if (path_prefix.empty()) {
        std::cout << "No matching path prefix found, using NotFoundHandler" << std::endl;
        return std::make_unique<NotFoundHandler>();
    }
    
    std::cout << "Found matching path prefix: " << path_prefix << std::endl;
    
    // Get the handler config for this path
    const auto& handler_config = handler_configs_.at(path_prefix);
    std::cout << "Using handler type: " << handler_config.type << std::endl;
    handler_name = handler_config.type; // store handler name for logging

    // Look up the factory in our registry
    auto& factory_map = GetFactoryMap();
    auto factory_it = factory_map.find(handler_config.type);
    
    if (factory_it == factory_map.end()) {
        std::cerr << "Error: No factory registered for handler type: " << handler_config.type << std::endl;
        std::cout << "Available handlers: ";
        for (const auto& [name, _] : factory_map) {
            std::cout << name << " ";
        }
        std::cout << std::endl;
        
        return std::make_unique<NotFoundHandler>();
    }
    
    std::cout << "Factory found for " << handler_config.type << std::endl;
    
    // Create the handler using the factory function
    RequestHandler* handler = factory_it->second(path_prefix, handler_config.config.get());
    if (!handler) {
        std::cerr << "Error: Failed to create handler for " << handler_config.type << std::endl;
        return std::make_unique<NotFoundHandler>();
    }
    
    std::cout << "Handler created successfully" << std::endl;
    return std::unique_ptr<RequestHandler>(handler);
}

} // namespace server
} // namespace http