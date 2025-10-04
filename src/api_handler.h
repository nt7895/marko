#ifndef API_HANDLER_H
#define API_HANDLER_H

#include <string>
#include <memory>
#include <boost/json.hpp>
#include "request_handler.hpp"
#include "config_parser.h"
#include "request_handler_registry.h"
#include "entity_processor.h"

namespace http {
namespace server {

class APIHandler : public RequestHandler {
public:
  static RequestHandler* Init(const std::string& path_prefix, const NginxConfig* config) {
    if (!config) {
      std::cerr << "Error: API handler requires configuration" << std::endl;
      return nullptr;
    }
    
    std::string data_path = config->FindConfigToken("data_path");
    if (data_path.empty()) {
      std::cerr << "Error: API handler requires 'data_path' parameter" << std::endl;
      return nullptr;
    }
    
    auto entity_processor = std::make_unique<EntityProcessor>(data_path);
    return new APIHandler(path_prefix, std::move(entity_processor));
  }


  static bool Register();
  
  // Constructor with dependency injection for testing
  APIHandler(const std::string& path_prefix, std::unique_ptr<EntityProcessor> entity_processor);

  // Main request handler
  std::unique_ptr<reply> handle_request(const request& request) override;

private:
  std::string path_prefix_;
  std::unique_ptr<EntityProcessor> entity_processor_;
  
  // Helper methods
  bool ParseUri(const std::string& uri, std::string& entity_type, std::string& id);
  bool IsValidJson(const std::string& json_data);
  
  // Request handlers for different CRUD operations
  std::unique_ptr<reply> HandleCreate(const std::string& entity_type, const std::string& json_data);
  std::unique_ptr<reply> HandleRetrieve(const std::string& entity_type, const std::string& id);
  std::unique_ptr<reply> HandleUpdate(const std::string& entity_type, const std::string& id, const std::string& json_data);
  std::unique_ptr<reply> HandleDelete(const std::string& entity_type, const std::string& id);
  std::unique_ptr<reply> HandleList(const std::string& entity_type);
};

}
}

#endif // API_HANDLER_H