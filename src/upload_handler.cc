#include "upload_handler.h"
#include <iostream>
#include <algorithm>
#include <cctype>

namespace http {
namespace server {

RequestHandler* UploadHandler::Init(const std::string& path_prefix, const NginxConfig* config) {
    if (!config) {
        std::cerr << "Error: Upload handler requires configuration" << std::endl;
        return nullptr;
    }
    
    // Hard-coded defaults as per technical design doc
    std::string upload_dir = "./uploads";
    size_t max_file_size = 10 * 1024 * 1024; // 10MB
    
    // Optional: Check for config overrides if needed
    std::string config_upload_dir = config->FindConfigToken("upload_dir");
    if (!config_upload_dir.empty()) {
        upload_dir = config_upload_dir;
    }
    
    std::string max_size_str = config->FindConfigToken("max_file_size");
    if (!max_size_str.empty()) {
        max_file_size = std::stoull(max_size_str);
    }
    
    return new UploadHandler(upload_dir, path_prefix, max_file_size);
}

UploadHandler::UploadHandler(const std::string& upload_dir, const std::string& path_prefix, size_t max_file_size)
    : upload_dir_(upload_dir), path_prefix_(path_prefix), max_file_size_(max_file_size) {
    
    // Initialize allowed file extensions - includes PDF as per design doc
    allowed_extensions_ = {".txt", ".md", ".pdf"};
    
    // Create upload directory if it doesn't exist
    try {
        std::filesystem::create_directories(upload_dir_);
    } catch (const std::exception& e) {
        std::cerr << "Error creating upload directory: " << e.what() << std::endl;
    }
}

std::unique_ptr<reply> UploadHandler::handle_request(const request& request) {
    // Check if URI matches our path prefix exactly
    if (!is_valid_upload_path(request.uri)) {
        return BuildResponse(reply::not_found, "404 Not Found");
    }
    
    if (request.method == "GET") {
        // Serve upload form
        return create_upload_form();
    } else if (request.method == "POST") {
        // Handle file upload
        
        // Find Content-Type header
        std::string content_type;
        for (const auto& header : request.headers) {
            if (header.name == "Content-Type") {
                content_type = header.value;
                break;
            }
        }
        
        if (content_type.find("multipart/form-data") == std::string::npos) {
            return create_error_response("Invalid content type. Expected multipart/form-data.");
        }
        
        // Extract boundary
        std::string boundary = extract_boundary(content_type);
        if (boundary.empty()) {
            return create_error_response("Missing boundary in Content-Type header.");
        }
        
        // Parse form data
        FormData form_data;
        if (!parse_multipart_form(request.body, boundary, form_data)) {
            return create_error_response("Failed to parse form data.");
        }
        
        // Validate file
        if (!validate_file(form_data.filename, form_data.content.size())) {
            return create_error_response("File validation failed. Check file type and size.");
        }
        
        // Generate unique file ID and sanitize filename
        std::string file_id = generate_file_id();
        std::string sanitized_name = sanitize_filename(form_data.filename);
        
        // Create file path
        std::string file_path = upload_dir_ + "/" + file_id + "_" + sanitized_name;
        
        // Save file to disk
        if (!save_file_to_disk(form_data.content, file_path)) {
            return create_error_response("Failed to save file to disk.");
        }
        
        return create_success_response(file_id, form_data.filename);
    }
    
    return BuildResponse(reply::bad_request, "Method not allowed");
}

bool UploadHandler::is_valid_upload_path(const std::string& uri) const {
    // Check for exact match or subpath
    if (uri == path_prefix_) {
        return true;
    }
    
    // Check if it starts with path_prefix_ followed by '/'
    if (uri.length() > path_prefix_.length() && 
        uri.compare(0, path_prefix_.length(), path_prefix_) == 0 &&
        uri[path_prefix_.length()] == '/') {
        return true;
    }
    
    return false;
}

bool UploadHandler::validate_file(const std::string& filename, size_t size) {
    // Check file size
    if (size > max_file_size_) {
        return false;
    }
    
    // Check file extension
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return false;
    }
    
    std::string ext = filename.substr(dot_pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return std::find(allowed_extensions_.begin(), allowed_extensions_.end(), ext) != allowed_extensions_.end();
}

std::string UploadHandler::generate_file_id() {
    // Generate random file ID using timestamp and random number
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    return std::to_string(timestamp) + "_" + std::to_string(dis(gen));
}

std::string UploadHandler::sanitize_filename(const std::string& filename) {
    std::string sanitized = filename;
    
    // Replace spaces and special characters with underscores
    std::regex special_chars("[^a-zA-Z0-9._-]");
    sanitized = std::regex_replace(sanitized, special_chars, "_");
    
    // Remove multiple consecutive underscores
    std::regex multi_underscore("_{2,}");
    sanitized = std::regex_replace(sanitized, multi_underscore, "_");
    
    return sanitized;
}

bool UploadHandler::save_file_to_disk(const std::string& content, const std::string& file_path) {
    try {
        std::ofstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        file.write(content.data(), content.size());
        file.close();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving file: " << e.what() << std::endl;
        return false;
    }
}

std::string UploadHandler::extract_boundary(const std::string& content_type) {
    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string::npos) {
        return "";
    }
    
    std::string boundary = content_type.substr(boundary_pos + 9);
    
    // Remove quotes if present
    if (!boundary.empty() && boundary.front() == '"') {
        boundary = boundary.substr(1);
    }
    if (!boundary.empty() && boundary.back() == '"') {
        boundary.pop_back();
    }
    
    return boundary;
}

bool UploadHandler::parse_multipart_form(const std::string& body, const std::string& boundary, FormData& form_data) {
    std::string delimiter = "--" + boundary;
    std::string end_delimiter = "--" + boundary + "--";
    
    size_t pos = 0;
    while (pos < body.length()) {
        // Find next boundary
        size_t boundary_start = body.find(delimiter, pos);
        if (boundary_start == std::string::npos) {
            break;
        }
        
        // Skip boundary
        pos = boundary_start + delimiter.length();
        
        // Skip CRLF after boundary
        if (pos + 1 < body.length() && body.substr(pos, 2) == "\r\n") {
            pos += 2;
        }
        
        // Find end of headers
        size_t headers_end = body.find("\r\n\r\n", pos);
        if (headers_end == std::string::npos) {
            break;
        }
        
        // Parse headers
        std::string headers = body.substr(pos, headers_end - pos);
        pos = headers_end + 4;
        
        // Find next boundary to get content
        size_t next_boundary = body.find(delimiter, pos);
        if (next_boundary == std::string::npos) {
            break;
        }
        
        // Extract content (remove trailing CRLF)
        std::string content = body.substr(pos, next_boundary - pos);
        if (content.length() >= 2 && content.substr(content.length() - 2) == "\r\n") {
            content = content.substr(0, content.length() - 2);
        }
        
        // Parse field name and filename from headers
        if (headers.find("name=\"file\"") != std::string::npos) {
            // This is the file field
            size_t filename_pos = headers.find("filename=\"");
            if (filename_pos != std::string::npos) {
                filename_pos += 10; // length of "filename=\""
                size_t filename_end = headers.find("\"", filename_pos);
                if (filename_end != std::string::npos) {
                    form_data.filename = headers.substr(filename_pos, filename_end - filename_pos);
                }
            }
            
            // Extract content type
            size_t content_type_pos = headers.find("Content-Type: ");
            if (content_type_pos != std::string::npos) {
                content_type_pos += 14; // length of "Content-Type: "
                size_t content_type_end = headers.find("\r\n", content_type_pos);
                if (content_type_end != std::string::npos) {
                    form_data.content_type = headers.substr(content_type_pos, content_type_end - content_type_pos);
                }
            }
            
            form_data.content = content;
        } else if (headers.find("name=\"course_code\"") != std::string::npos) {
            form_data.course_code = content;
        } else if (headers.find("name=\"title\"") != std::string::npos) {
            form_data.title = content;
        }
        
        pos = next_boundary;
    }
    
    return !form_data.filename.empty() && !form_data.content.empty();
}

std::unique_ptr<reply> UploadHandler::create_upload_form() {
    std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>UCLA Notes Upload</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input, textarea { width: 300px; padding: 8px; border: 1px solid #ccc; border-radius: 4px; }
        button { background-color: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; }
        button:hover { background-color: #0056b3; }
        .info { background-color: #f8f9fa; padding: 15px; border-radius: 4px; margin-bottom: 20px; }
    </style>
</head>
<body>
    <h1>UCLA Notes Upload</h1>
    <div class="info">
        <p><strong>Supported file types:</strong> .txt, .md, .pdf</p>
        <p><strong>Maximum file size:</strong> 10MB</p>
    </div>
    
    <form method="post" enctype="multipart/form-data">
        <div class="form-group">
            <label for="file">Select File:</label>
            <input type="file" id="file" name="file" accept=".txt,.md,.pdf" required>
        </div>
        
        <div class="form-group">
            <label for="course_code">Course Code:</label>
            <input type="text" id="course_code" name="course_code" placeholder="e.g., CS130" required>
        </div>
        
        <div class="form-group">
            <label for="title">Title:</label>
            <input type="text" id="title" name="title" placeholder="e.g., Lecture 5 Notes" required>
        </div>
        
        <button type="submit">Upload</button>
    </form>
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

std::unique_ptr<reply> UploadHandler::create_success_response(const std::string& file_id, const std::string& filename) {
    std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Upload Success</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .success { background-color: #d4edda; color: #155724; padding: 15px; border-radius: 4px; margin-bottom: 20px; }
        .file-info { background-color: #f8f9fa; padding: 15px; border-radius: 4px; }
        a { color: #007bff; text-decoration: none; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <h1>Upload Successful!</h1>
    <div class="success">
        <p>Your file has been uploaded successfully.</p>
    </div>
    
    <div class="file-info">
        <p><strong>File ID:</strong> )" + file_id + R"(</p>
        <p><strong>Filename:</strong> )" + filename + R"(</p>
    </div>
    
    <p><a href=")" + path_prefix_ + R"(">Upload another file</a></p>
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

std::unique_ptr<reply> UploadHandler::create_error_response(const std::string& error_message) {
    std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Upload Error</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .error { background-color: #f8d7da; color: #721c24; padding: 15px; border-radius: 4px; margin-bottom: 20px; }
        a { color: #007bff; text-decoration: none; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <h1>Upload Failed</h1>
    <div class="error">
        <p>Error: )" + error_message + R"(</p>
    </div>
    
    <p><a href=")" + path_prefix_ + R"(">Try again</a></p>
</body>
</html>
)";
    
    std::vector<header> headers;
    header content_type;
    content_type.name = "Content-Type";
    content_type.value = "text/html";
    headers.push_back(content_type);
    
    return BuildResponse(reply::bad_request, html, headers);
}

bool UploadHandler::Register() {
    std::cout << "Registering UploadHandler" << std::endl;
    return RequestHandlerRegistry::RegisterHandler("UploadHandler", UploadHandler::Init);
}

static struct UploadHandlerRegistrar {
    UploadHandlerRegistrar() {
        std::cout << "UploadHandlerRegistrar constructor called" << std::endl;
        UploadHandler::Register();
    }
} uploadHandlerRegistrar;

} // namespace server
} // namespace http