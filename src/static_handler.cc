#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <filesystem>
#include <iostream>
#include "static_handler.h"

namespace http {
namespace server {

// Initialize MIME type mapping
void StaticFileHandler::InitMimeTypeMap() {
    mime_type_map_[".html"] = "text/html";
    mime_type_map_[".htm"] = "text/html";
    mime_type_map_[".css"] = "text/css";
    mime_type_map_[".js"] = "application/javascript";
    mime_type_map_[".txt"] = "text/plain";
    mime_type_map_[".jpg"] = "image/jpeg";
    mime_type_map_[".jpeg"] = "image/jpeg";
    mime_type_map_[".png"] = "image/png";
    mime_type_map_[".gif"] = "image/gif";
    mime_type_map_[".svg"] = "image/svg+xml";
    mime_type_map_[".ico"] = "image/x-icon";
    mime_type_map_[".zip"] = "application/zip";
    mime_type_map_[".pdf"] = "application/pdf";
    mime_type_map_[".json"] = "application/json";
    mime_type_map_[".xml"] = "application/xml";
}
  
// Get MIME type based on file extension
std::string StaticFileHandler::GetMimeType(const std::string& file_path) {
    size_t dot_pos = file_path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string ext = file_path.substr(dot_pos);
        auto it = mime_type_map_.find(ext);
        if (it != mime_type_map_.end()) {
            return it->second;
        }
    }
    // Default MIME type for unknown extensions
    return "application/octet-stream";
}
  
// Read file content
bool StaticFileHandler::ReadFile(const std::string& file_path, std::string& content) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
  
    // Get file size
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
  
    // Read file content
    content.resize(size);
    file.read(&content[0], size);
  
    return true;
}
  
// Handle HTTP request
std::unique_ptr<reply> StaticFileHandler::handle_request(const request& request) {
    // Extract path from URI, removing any query parameters
    std::string path = request.uri;
    size_t query_pos = path.find('?');
    if (query_pos != std::string::npos) {
        path = path.substr(0, query_pos);
    }
  
    // Check if the URI starts with our path prefix
    if (path.compare(0, path_prefix_.length(), path_prefix_) != 0) {
        // URI doesn't match our prefix, return 404
        return BuildResponse(reply::not_found, "404 Not Found");
    }
    
    // Remove the path prefix to get the relative file path
    std::string relative_path = path.substr(path_prefix_.length());
    if (!relative_path.empty() && relative_path.front() == '/') {
        relative_path = relative_path.substr(1);
    }
    std::string file_path = "." + root_dir_ + "/" + relative_path;
  
    // Try to read the file
    std::string content;
    if (ReadFile(file_path, content)) {
        // File found, serve it
        std::vector<header> headers;
        header content_type;
        content_type.name = "Content-Type";
        content_type.value = GetMimeType(file_path);
        headers.push_back(content_type);
        
        return BuildResponse(reply::ok, content, headers);
    } else {
        // File not found, return 404
        return BuildResponse(reply::not_found, "404 Not Found");
    }
}

bool StaticFileHandler::Register() {
    std::cout << "Registering StaticFileHandler" << std::endl;
    return RequestHandlerRegistry::RegisterHandler("StaticHandler", StaticFileHandler::Init);
  }
  
static struct StaticFileHandlerRegistrar {
    StaticFileHandlerRegistrar() {
        std::cout << "StaticFileHandlerRegistrar constructor called" << std::endl;
        StaticFileHandler::Register();
    }
} staticFileHandlerRegistrar;

} // namespace server
} // namespace http