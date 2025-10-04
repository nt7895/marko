#ifndef STATIC_HANDLER_H
#define STATIC_HANDLER_H

#include <string>
#include <map>
#include "request_handler.hpp"
#include "config_parser.h"
#include "request_handler_registry.h"

namespace http {
namespace server {

class StaticFileHandler: public RequestHandler {
public:
  static RequestHandler* Init(const std::string& path_prefix, const NginxConfig* config) {
    if (!config) {
      std::cerr << "Error: Static handler requires configuration" << std::endl;
      return nullptr;
    }
    
    std::string root_dir = config->FindConfigToken("root");
    if (root_dir.empty()) {
      std::cerr << "Error: Static handler requires 'root' parameter" << std::endl;
      return nullptr;
    }
    
    return new StaticFileHandler(root_dir, path_prefix);
  }
  
  // Register handler with static initializer function
  static bool Register();
  
  StaticFileHandler(const std::string& root_dir, const std::string& path_prefix) 
  : root_dir_(root_dir), path_prefix_(path_prefix) {
    InitMimeTypeMap();
  }

  std::unique_ptr<reply> handle_request(const request& request) override;

private:
  std::string root_dir_;
  std::string path_prefix_;
  std::map<std::string, std::string> mime_type_map_;
  
  void InitMimeTypeMap();
  std::string GetMimeType(const std::string& file_path);
  bool ReadFile(const std::string& file_path, std::string& content);
};

} // namespace server
} // namespace http

#endif // STATIC_HANDLER_H