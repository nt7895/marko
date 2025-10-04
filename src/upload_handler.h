#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <random>
#include <chrono>
#include <regex>
#include "request_handler.hpp"
#include "config_parser.h"
#include "request_handler_registry.h"

namespace http {
namespace server {

class UploadHandler : public RequestHandler {
public:
    static RequestHandler* Init(const std::string& path_prefix, const NginxConfig* config);
    static bool Register();
    
    UploadHandler(const std::string& upload_dir, const std::string& path_prefix, size_t max_file_size = 10 * 1024 * 1024);
    
    std::unique_ptr<reply> handle_request(const request& request) override;

private:
    std::string upload_dir_;
    std::string path_prefix_;
    size_t max_file_size_;
    std::vector<std::string> allowed_extensions_;
    
    // Core upload logic
    bool validate_file(const std::string& filename, size_t size);
    std::string generate_file_id();
    std::string sanitize_filename(const std::string& filename);
    bool save_file_to_disk(const std::string& content, const std::string& file_path);
    
    // Form parsing
    struct FormData {
        std::string filename;
        std::string content;
        std::string course_code;
        std::string title;
        std::string content_type;
    };
    
    bool parse_multipart_form(const std::string& body, const std::string& boundary, FormData& form_data);
    std::string extract_boundary(const std::string& content_type);
    
    // Response helpers
    std::unique_ptr<reply> create_upload_form();
    std::unique_ptr<reply> create_success_response(const std::string& file_id, const std::string& filename);
    std::unique_ptr<reply> create_error_response(const std::string& error_message);
    
    // Helper method for URI validation
    bool is_valid_upload_path(const std::string& uri) const;
};

} // namespace server
} // namespace http

#endif // UPLOAD_HANDLER_H