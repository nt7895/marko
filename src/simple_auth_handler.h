#ifndef HTTP_SIMPLE_AUTH_HANDLER_HPP
#define HTTP_SIMPLE_AUTH_HANDLER_HPP

#include "request_handler.hpp"
#include "config_parser.h"
#include "request_handler_registry.h"
#include "database_manager.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <ctime>

namespace http {
namespace server {

struct UserSession {
    int user_id;
    std::string email;
    std::time_t expires_at;
};

class SimpleAuthHandler : public RequestHandler {
public:
    static RequestHandler* Init(const std::string& path_prefix, const NginxConfig* config);
    static bool Register();
    
    SimpleAuthHandler(const std::string& path_prefix, const std::string& db_path = "data/notes_app.db");
    
    std::unique_ptr<reply> handle_request(const request& request) override;
    
    // Static methods for use by other handlers
    static int validateSession(const std::string& session_token);
    static std::string extractSessionToken(const request& request);
    static std::string getUserEmail(const std::string& session_token);
    static void clearSession(const std::string& session_token);

private:
    std::string path_prefix_;
    std::unique_ptr<DatabaseManager> db_manager_;
    static std::unordered_map<std::string, UserSession> active_sessions_;
    
    // Request handling methods
    std::unique_ptr<reply> serveLoginForm();
    std::unique_ptr<reply> handleLogin(const request& request);
    std::unique_ptr<reply> handleLogout(const request& request);
    
    // Session management
    std::string createSession(int user_id, const std::string& email);
    std::string generateSessionToken();
    void cleanupExpiredSessions();
    
    // Helper methods
    std::string extractFormField(const std::string& body, const std::string& field_name);
    std::string urlDecode(const std::string& encoded);
    bool isValidEmail(const std::string& email);
};

} // namespace server
} // namespace http

#endif // HTTP_SIMPLE_AUTH_HANDLER_HPP