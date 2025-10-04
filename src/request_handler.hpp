#ifndef HTTP_REQUEST_HANDLER_HPP
#define HTTP_REQUEST_HANDLER_HPP

#include "request.hpp"
#include "reply.hpp"
#include <memory>
#include <iostream> 

namespace http {
namespace server {

class RequestHandler {
public:
    virtual ~RequestHandler() {}
    
    virtual std::unique_ptr<reply> handle_request(const request& request) = 0;
    
protected:
    // Helper method to build complete response
    std::unique_ptr<reply> BuildResponse(reply::status_type status,
                       const std::string& content,
                       const std::vector<header>& headers = {}) {
        try {
            auto rep = std::make_unique<reply>();
            rep->status = status;
            rep->content = content;
            
            rep->headers = headers;
            
            bool has_content_length = false;
            for (const auto& h : rep->headers) {
                if (h.name == "Content-Length") {
                    has_content_length = true;
                    break;
                }
            }
            
            if (!has_content_length) {
                header content_length;
                content_length.name = "Content-Length";
                content_length.value = std::to_string(content.size());
                rep->headers.push_back(content_length);
            }
            
            bool has_content_type = false;
            for (const auto& h : rep->headers) {
                if (h.name == "Content-Type") {
                    has_content_type = true;
                    break;
                }
            }
            
            if (!has_content_type) {
                header content_type;
                content_type.name = "Content-Type";
                content_type.value = "text/plain";
                rep->headers.push_back(content_type);
            }
            return rep;
        } catch (std::exception& e) {
            std::cerr << "BuildResponse Exception: " << e.what() << "\n";

            auto rep = std::make_unique<reply>();
            rep->status = reply::internal_server_error;
            rep->content = "BuildResponse() Failure: " + std::string(e.what());
            rep->headers = {{"Content-Length", std::to_string(rep->content.size())}, {"Content-Type", "text/plain"}};

            return rep;
        }
    }
};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HANDLER_HPP