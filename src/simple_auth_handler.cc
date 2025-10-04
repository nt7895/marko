#include "simple_auth_handler.h"
#include <iostream>
#include <sstream>
#include <random>
#include <algorithm>
#include <boost/regex.hpp>

namespace http {
namespace server {

// Static member definition
std::unordered_map<std::string, UserSession> SimpleAuthHandler::active_sessions_;

RequestHandler* SimpleAuthHandler::Init(const std::string& path_prefix, const NginxConfig* config) {
    // Optional: Read database path from config
    std::string db_path = "data/notes_app.db";
    if (config) {
        std::string config_db_path = config->FindConfigToken("db_path");
        if (!config_db_path.empty()) {
            db_path = config_db_path;
        }
    }
    
    return new SimpleAuthHandler(path_prefix, db_path);
}

SimpleAuthHandler::SimpleAuthHandler(const std::string& path_prefix, const std::string& db_path)
    : path_prefix_(path_prefix), db_manager_(std::make_unique<DatabaseManager>(db_path)) {
    std::cout << "SimpleAuthHandler initialized with path_prefix: " << path_prefix << std::endl;
}

std::unique_ptr<reply> SimpleAuthHandler::handle_request(const request& request) {
    // Clean up expired sessions periodically
    cleanupExpiredSessions();
    
    std::string decoded_uri = urlDecode(request.uri);
    
    // Handle login page
    if ((decoded_uri == "/login" || decoded_uri == path_prefix_) && request.method == "GET") {
        return serveLoginForm();
    }
    // Handle login form submission
    else if ((decoded_uri == "/login" || decoded_uri == path_prefix_) && request.method == "POST") {
        return handleLogin(request);
    }
    // Handle logout
    else if (decoded_uri == "/logout" && request.method == "POST") {
        return handleLogout(request);
    }
    
    return BuildResponse(reply::not_found, "Authentication endpoint not found");
}

std::unique_ptr<reply> SimpleAuthHandler::serveLoginForm() {
    std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>UCLA Notes - Login</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            margin: 0;
            padding: 0;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        
        .login-container {
            background: white;
            padding: 40px;
            border-radius: 12px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            max-width: 420px;
            width: 90%;
            text-align: center;
        }
        
        .logo {
            font-size: 3em;
            margin-bottom: 10px;
        }
        
        h1 {
            color: #333;
            margin-bottom: 10px;
            font-size: 2em;
            font-weight: 300;
        }
        
        .subtitle {
            color: #666;
            margin-bottom: 30px;
            font-size: 1.1em;
        }
        
        .info-box {
            background: linear-gradient(135deg, #e3f2fd 0%, #f3e5f5 100%);
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 30px;
            border-left: 4px solid #667eea;
        }
        
        .info-box h3 {
            color: #1565c0;
            margin-bottom: 8px;
            font-size: 1.1em;
        }
        
        .info-box p {
            color: #424242;
            font-size: 0.95em;
            line-height: 1.4;
        }
        
        .form-group {
            margin-bottom: 20px;
            text-align: left;
        }
        
        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            color: #333;
            font-size: 0.95em;
        }
        
        input[type="email"] {
            width: 100%;
            padding: 15px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-size: 16px;
            transition: border-color 0.3s ease, box-shadow 0.3s ease;
            outline: none;
        }
        
        input[type="email"]:focus {
            border-color: #667eea;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }
        
        button {
            width: 100%;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 15px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-size: 16px;
            font-weight: 600;
            transition: transform 0.2s ease, box-shadow 0.2s ease;
        }
        
        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
        }
        
        button:active {
            transform: translateY(0);
        }
        
        .note {
            text-align: center;
            margin-top: 20px;
            color: #666;
            font-size: 0.9em;
        }
        
        .error {
            background: #ffebee;
            color: #c62828;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            border-left: 4px solid #c62828;
            display: none;
        }
        
        .loading {
            display: none;
            text-align: center;
            color: #666;
            margin-top: 10px;
        }
        
        @media (max-width: 480px) {
            .login-container {
                padding: 30px 20px;
                margin: 20px;
            }
            
            h1 {
                font-size: 1.8em;
            }
        }
    </style>
</head>
<body>
    <div class="login-container">
        <div class="logo">🎓</div>
        <h1>UCLA Notes</h1>
        <div class="subtitle">Share and discover study materials</div>
        
        <div class="info-box">
            <h3>🚀 Simple Access</h3>
            <p>Enter your email address to access the notes sharing platform. No password required for this demo!</p>
        </div>
        
        <div id="errorMessage" class="error"></div>
        
        <form id="loginForm" action="/login" method="post">
            <div class="form-group">
                <label for="email">📧 Email Address:</label>
                <input type="email" id="email" name="email" placeholder="your.email@ucla.edu" required>
            </div>
            
            <button type="submit" id="loginButton">
                Access Notes Platform
            </button>
        </form>
        
        <div id="loading" class="loading">
            🔄 Signing you in...
        </div>
        
        <div class="note">
            <small>🔒 Your email is only used for identification</small>
        </div>
    </div>
    
    <script>
        document.getElementById('loginForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const email = document.getElementById('email').value.trim();
            const errorDiv = document.getElementById('errorMessage');
            const loadingDiv = document.getElementById('loading');
            const button = document.getElementById('loginButton');
            
            // Hide previous errors
            errorDiv.style.display = 'none';
            
            // Basic email validation
            if (!email) {
                showError('Please enter your email address');
                return;
            }
            
            if (!isValidEmail(email)) {
                showError('Please enter a valid email address');
                return;
            }
            
            // Show loading state
            button.style.display = 'none';
            loadingDiv.style.display = 'block';
            
            // Submit form
            fetch('/login', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: 'email=' + encodeURIComponent(email)
            })
            .then(response => {
                if (response.ok) {
                    // Check if it's a redirect
                    if (response.url !== window.location.href) {
                        window.location.href = response.url;
                    } else {
                        window.location.href = '/upload';
                    }
                } else {
                    return response.text().then(text => {
                        throw new Error(text || 'Login failed');
                    });
                }
            })
            .catch(error => {
                console.error('Login error:', error);
                showError('Login failed. Please try again.');
                button.style.display = 'block';
                loadingDiv.style.display = 'none';
            });
        });
        
        function showError(message) {
            const errorDiv = document.getElementById('errorMessage');
            errorDiv.textContent = message;
            errorDiv.style.display = 'block';
        }
        
        function isValidEmail(email) {
            const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
            return emailRegex.test(email);
        }
        
        // Auto-focus email input
        document.getElementById('email').focus();
    </script>
</body>
</html>
)";
    
    std::vector<header> headers;
    header content_type;
    content_type.name = "Content-Type";
    content_type.value = "text/html";
    headers.push_back(content_type);
    
    return BuildResponse(reply::ok, html, headers);
}

std::unique_ptr<reply> SimpleAuthHandler::handleLogin(const request& request) {
    std::string email = extractFormField(request.body, "email");
    
    if (email.empty()) {
        return BuildResponse(reply::bad_request, "Email required");
    }
    
    if (!isValidEmail(email)) {
        return BuildResponse(reply::bad_request, "Invalid email format");
    }
    
    try {
        // Create or get user in database
        int user_id = db_manager_->createOrUpdateUser(email, "Student");
        if (user_id <= 0) {
            return BuildResponse(reply::internal_server_error, "Failed to create user account");
        }
        
        // Create session
        std::string session_token = createSession(user_id, email);
        
        // Redirect to upload page with session cookie
        std::vector<header> headers;
        
        header location;
        location.name = "Location";
        location.value = "/upload";
        
        header cookie;
        cookie.name = "Set-Cookie";
        cookie.value = "session_token=" + session_token + "; Path=/; Max-Age=3600; HttpOnly";
        
        headers.push_back(location);
        headers.push_back(cookie);
        
        // return BuildResponse(reply::moved_temporarily, "", headers);
        // do a dummy return
        return BuildResponse(reply::ok, "Login successful. Redirecting...", headers);
        
    } catch (const std::exception& e) {
        std::cerr << "Login error: " << e.what() << std::endl;
        return BuildResponse(reply::internal_server_error, "Login failed. Please try again.");
    }
}

std::unique_ptr<reply> SimpleAuthHandler::handleLogout(const request& request) {
    std::string session_token = extractSessionToken(request);
    
    if (!session_token.empty()) {
        clearSession(session_token);
    }
    
    // Redirect to login page with cleared cookie
    std::vector<header> headers;
    
    header location;
    location.name = "Location";
    location.value = "/login";
    
    header cookie;
    cookie.name = "Set-Cookie";
    cookie.value = "session_token=; Path=/; Max-Age=0; HttpOnly";
    
    headers.push_back(location);
    headers.push_back(cookie);
    
    return BuildResponse(reply::moved_temporarily, "", headers);
}

std::string SimpleAuthHandler::createSession(int user_id, const std::string& email) {
    std::string token = generateSessionToken();
    UserSession session;
    session.user_id = user_id;
    session.email = email;
    session.expires_at = std::time(nullptr) + 3600; // 1 hour expiry
    
    active_sessions_[token] = session;
    return token;
}

std::string SimpleAuthHandler::generateSessionToken() {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    
    std::string token;
    token.reserve(32);
    for (int i = 0; i < 32; ++i) {
        token += chars[dis(gen)];
    }
    return token;
}

void SimpleAuthHandler::cleanupExpiredSessions() {
    std::time_t now = std::time(nullptr);
    auto it = active_sessions_.begin();
    while (it != active_sessions_.end()) {
        if (it->second.expires_at <= now) {
            it = active_sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

// Static methods for use by other handlers
int SimpleAuthHandler::validateSession(const std::string& session_token) {
    auto it = active_sessions_.find(session_token);
    if (it != active_sessions_.end() && it->second.expires_at > std::time(nullptr)) {
        return it->second.user_id;
    }
    return -1; // Invalid session
}

std::string SimpleAuthHandler::extractSessionToken(const request& request) {
    // Extract from cookie header
    for (const auto& header : request.headers) {
        if (header.name == "Cookie") {
            std::string cookie = header.value;
            boost::smatch match;
            boost::regex session_regex(R"((?:^|;\s*)session_token=([^;]+))");
            if (boost::regex_search(cookie, match, session_regex)) {
                return match[1].str();
            }
        }
    }
    return "";
}

std::string SimpleAuthHandler::getUserEmail(const std::string& session_token) {
    auto it = active_sessions_.find(session_token);
    if (it != active_sessions_.end() && it->second.expires_at > std::time(nullptr)) {
        return it->second.email;
    }
    return "";
}

void SimpleAuthHandler::clearSession(const std::string& session_token) {
    active_sessions_.erase(session_token);
}

std::string SimpleAuthHandler::extractFormField(const std::string& body, const std::string& field_name) {
    std::string search = field_name + "=";
    size_t pos = body.find(search);
    if (pos == std::string::npos) return "";
    
    pos += search.length();
    size_t end = body.find("&", pos);
    if (end == std::string::npos) end = body.length();
    
    std::string value = body.substr(pos, end - pos);
    return urlDecode(value);
}

std::string SimpleAuthHandler::urlDecode(const std::string& encoded) {
    std::string decoded;
    decoded.reserve(encoded.length());
    
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            std::string hex = encoded.substr(i + 1, 2);
            int value;
            std::stringstream ss;
            ss << std::hex << hex;
            ss >> value;
            decoded += static_cast<char>(value);
            i += 2;
        } else if (encoded[i] == '+') {
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }
    
    return decoded;
}

bool SimpleAuthHandler::isValidEmail(const std::string& email) {
    boost::regex email_regex(R"(^[^\s@]+@[^\s@]+\.[^\s@]+$)");
    return boost::regex_match(email, email_regex);
}

bool SimpleAuthHandler::Register() {
    std::cout << "Registering SimpleAuthHandler" << std::endl;
    return RequestHandlerRegistry::RegisterHandler("SimpleAuthHandler", SimpleAuthHandler::Init);
}

static struct SimpleAuthHandlerRegistrar {
    SimpleAuthHandlerRegistrar() {
        std::cout << "SimpleAuthHandlerRegistrar constructor called" << std::endl;
        SimpleAuthHandler::Register();
    }
} simpleAuthHandlerRegistrar;

} // namespace server
} // namespace http