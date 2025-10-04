
#ifndef HTTP_TEXT_VIEW_HANDLER_HPP
#define HTTP_TEXT_VIEW_HANDLER_HPP
#include "request_handler.hpp"
#include "config_parser.h"
#include "request_handler_registry.h"
#include <filesystem>

namespace http {
namespace server {
  class TextViewHandler : public RequestHandler {
  public:
    static RequestHandler* Init(const std::string& path_prefix, const NginxConfig* config) {
      if (!config) {
        std::cerr << "Error: TextView handler requires configuration" << std::endl;
        return nullptr;
      }
      
      std::string view_dir = config->FindConfigToken("view_dir");
      if (view_dir.empty()) {
        std::cerr << "Error: TextView handler requires 'view_dir' parameter" << std::endl;
        return nullptr;
      }
      if (view_dir.find("..") != std::string::npos || view_dir.front() == '/'){
        std::cerr << "Error: view_dir is invalid" << std::endl;
        return nullptr;
      }

      if (!std::filesystem::exists(view_dir)) {
        try {
          std::filesystem::create_directories(view_dir);
        } catch (const std::exception& e) {
          std::cerr << "Error creating directory: " << e.what() << std::endl;
        }
      }

      return new TextViewHandler(view_dir);
    }
    
    // Register handler with static initializer function
    static bool Register();
    TextViewHandler(const std::string& view_dir) : view_dir_(view_dir) {}
    
    std::unique_ptr<reply> handle_request(const request& request) override;
    // converts markdown file to an html file
    std::string render_markdown(const std::string& content);
    // reads pdf file by converting to txt file
    bool read_pdf(std::string& content, const std::string &id);
  private:
    std::string view_dir_;
    // request path and file parsing
    std::string urlDecode(const std::string& encoded);
    bool parse_uri(const std::string& uri, std::string& id);
    bool read_file(const std::string& id, std::string& file_content);
    bool parse_file_extension(const std::string& id, std::string& file_extension);
    // converts each markdown line to html
    std::string convert_to_html(const std::string& line, bool& parsing_status, std::string& tag);
    
    // counts number of # in headers
    int find_header_level(const std::string& header);
    // adds open/close tags for lists
    std::string add_open_tag(std::string& tag, const std::string& tag_name, bool& status);
    std::string add_close_tag(const std::string& tag, bool& status);
    // handles bold, italics, links in paragraphs
    std::string handle_paragraph(const std::string& line);
  };
} // namespace server
} // namespace http
#endif // HTTP_TEXT_VIEW_HANDLER_HPP