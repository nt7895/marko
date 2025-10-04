#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "upload_handler.h"
#include "config_parser.h"

using namespace http::server;
using namespace testing;

class UploadHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test upload directory
        test_upload_dir_ = "./test_uploads";
        std::filesystem::create_directories(test_upload_dir_);
        
        // Create handler with test configuration
        handler_ = std::make_unique<UploadHandler>(test_upload_dir_, "/upload", 1024 * 1024); // 1MB limit for tests
    }
    
    void TearDown() override {
        // Clean up test directory
        if (std::filesystem::exists(test_upload_dir_)) {
            std::filesystem::remove_all(test_upload_dir_);
        }
    }
    
    // Helper to create multipart form data
    std::string create_multipart_form(const std::string& boundary, 
                                     const std::string& filename,
                                     const std::string& content,
                                     const std::string& course_code = "CS130",
                                     const std::string& title = "Test Notes") {
        std::ostringstream form;
        
        // File field
        form << "--" << boundary << "\r\n";
        form << "Content-Disposition: form-data; name=\"file\"; filename=\"" << filename << "\"\r\n";
        form << "Content-Type: text/plain\r\n";
        form << "\r\n";
        form << content << "\r\n";
        
        // Course code field
        form << "--" << boundary << "\r\n";
        form << "Content-Disposition: form-data; name=\"course_code\"\r\n";
        form << "\r\n";
        form << course_code << "\r\n";
        
        // Title field
        form << "--" << boundary << "\r\n";
        form << "Content-Disposition: form-data; name=\"title\"\r\n";
        form << "\r\n";
        form << title << "\r\n";
        
        // End boundary
        form << "--" << boundary << "--\r\n";
        
        return form.str();
    }
    
    request create_get_request() {
        request req;
        req.method = "GET";
        req.uri = "/upload";
        req.http_version_major = 1;
        req.http_version_minor = 1;
        return req;
    }
    
    request create_post_request(const std::string& body, const std::string& boundary) {
        request req;
        req.method = "POST";
        req.uri = "/upload";
        req.http_version_major = 1;
        req.http_version_minor = 1;
        req.body = body;
        
        header content_type;
        content_type.name = "Content-Type";
        content_type.value = "multipart/form-data; boundary=" + boundary;
        req.headers.push_back(content_type);
        
        return req;
    }
    
    std::string test_upload_dir_;
    std::unique_ptr<UploadHandler> handler_;
};

// Test GET request - should return upload form
TEST_F(UploadHandlerTest, HandleGetRequestReturnsUploadForm) {
    request req = create_get_request();
    
    auto response = handler_->handle_request(req);
    
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->status, reply::ok);
    EXPECT_THAT(response->content, HasSubstr("UCLA Notes Upload"));
    EXPECT_THAT(response->content, HasSubstr("<form"));
    EXPECT_THAT(response->content, HasSubstr("multipart/form-data"));
    
    // Check Content-Type header
    bool has_html_content_type = false;
    for (const auto& header : response->headers) {
        if (header.name == "Content-Type" && header.value == "text/html") {
            has_html_content_type = true;
            break;
        }
    }
    EXPECT_TRUE(has_html_content_type);
}

// Test successful file upload
TEST_F(UploadHandlerTest, HandleValidFileUploadSucceeds) {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string file_content = "This is test content for a text file.";
    std::string form_body = create_multipart_form(boundary, "test.txt", file_content);
    
    request req = create_post_request(form_body, boundary);
    
    auto response = handler_->handle_request(req);
    
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->status, reply::ok);
    EXPECT_THAT(response->content, HasSubstr("Upload Successful"));
    EXPECT_THAT(response->content, HasSubstr("test.txt"));
    
    // Verify file was saved to disk
    auto dir_iter = std::filesystem::directory_iterator(test_upload_dir_);
    bool file_found = false;
    for (auto const& dir_entry : dir_iter) {
        if (dir_entry.is_regular_file()) {
            std::ifstream file(dir_entry.path());
            std::string saved_content((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
            if (saved_content == file_content) {
                file_found = true;
                break;
            }
        }
    }
    EXPECT_TRUE(file_found);
}

// Test markdown file upload
TEST_F(UploadHandlerTest, HandleMarkdownFileUploadSucceeds) {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string md_content = "# Test Markdown\n\nThis is **bold** text.";
    std::string form_body = create_multipart_form(boundary, "notes.md", md_content);
    
    request req = create_post_request(form_body, boundary);
    
    auto response = handler_->handle_request(req);
    
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->status, reply::ok);
    EXPECT_THAT(response->content, HasSubstr("Upload Successful"));
}

// Test file size validation
TEST_F(UploadHandlerTest, RejectsFileTooLarge) {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string large_content(2 * 1024 * 1024, 'A'); // 2MB content, exceeds 1MB limit
    std::string form_body = create_multipart_form(boundary, "large.txt", large_content);
    
    request req = create_post_request(form_body, boundary);
    
    auto response = handler_->handle_request(req);
    
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->status, reply::bad_request);
    EXPECT_THAT(response->content, HasSubstr("File validation failed"));
}

// Test file extension validation
TEST_F(UploadHandlerTest, RejectsInvalidFileExtension) {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string content = "This is content";
    std::string form_body = create_multipart_form(boundary, "test.exe", content); // EXE not allowed
    
    request req = create_post_request(form_body, boundary);
    
    auto response = handler_->handle_request(req);
    
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->status, reply::bad_request);
    EXPECT_THAT(response->content, HasSubstr("File validation failed"));
}

// Test invalid content type
TEST_F(UploadHandlerTest, RejectsInvalidContentType) {
    request req;
    req.method = "POST";
    req.uri = "/upload";
    req.body = "not multipart data";
    
    header content_type;
    content_type.name = "Content-Type";
    content_type.value = "application/json"; // Wrong content type
    req.headers.push_back(content_type);
    
    auto response = handler_->handle_request(req);
    
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->status, reply::bad_request);
    EXPECT_THAT(response->content, HasSubstr("Invalid content type"));
}

// Test missing boundary
TEST_F(UploadHandlerTest, RejectsMissingBoundary) {
    request req;
    req.method = "POST";
    req.uri = "/upload";
    req.body = "multipart data without boundary";
    
    header content_type;
    content_type.name = "Content-Type";
    content_type.value = "multipart/form-data"; // Missing boundary
    req.headers.push_back(content_type);
    
    auto response = handler_->handle_request(req);
    
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->status, reply::bad_request);
    EXPECT_THAT(response->content, HasSubstr("Missing boundary"));
}

// Test malformed multipart data
TEST_F(UploadHandlerTest, RejectsMalformedMultipartData) {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string malformed_body = "--" + boundary + "\r\nmalformed data\r\n";
    
    request req = create_post_request(malformed_body, boundary);
    
    auto response = handler_->handle_request(req);
    
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->status, reply::bad_request);
    EXPECT_THAT(response->content, HasSubstr("Failed to parse form data"));
}

// Test unsupported HTTP method
TEST_F(UploadHandlerTest, RejectsUnsupportedMethod) {
    request req;
    req.method = "DELETE";
    req.uri = "/upload";
    
    auto response = handler_->handle_request(req);
    
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->status, reply::bad_request);
}

// Test wrong URI path
TEST_F(UploadHandlerTest, RejectsWrongPath) {
    request req;
    req.method = "GET";
    req.uri = "/wrong-path";
    
    auto response = handler_->handle_request(req);
    
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->status, reply::not_found);
}

// Test filename sanitization (integration with file saving)
TEST_F(UploadHandlerTest, SanitizesFilenamesCorrectly) {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string content = "Test content";
    std::string problematic_filename = "test file@#$%.txt"; // Contains special characters
    std::string form_body = create_multipart_form(boundary, problematic_filename, content);
    
    request req = create_post_request(form_body, boundary);
    
    auto response = handler_->handle_request(req);
    
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->status, reply::ok);
    
    // Check that a file was created (filename should be sanitized)
    auto dir_iter = std::filesystem::directory_iterator(test_upload_dir_);
    bool file_found = false;
    for (auto const& dir_entry : dir_iter) {
        if (dir_entry.is_regular_file()) {
            file_found = true;
            break;
        }
    }
    EXPECT_TRUE(file_found);
}

// Test PDF file upload
TEST_F(UploadHandlerTest, HandlePdfFileUploadSucceeds) {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string pdf_content = "%PDF-1.4\n1 0 obj\n<<\n/Type /Catalog\n>>\nendobj\nxref\n%%EOF";
    std::string form_body = create_multipart_form(boundary, "notes.pdf", pdf_content);
    
    request req = create_post_request(form_body, boundary);
    
    auto response = handler_->handle_request(req);
    
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->status, reply::ok);
    EXPECT_THAT(response->content, HasSubstr("Upload Successful"));
    EXPECT_THAT(response->content, HasSubstr("notes.pdf"));
}

// Test case-insensitive file extension validation
TEST_F(UploadHandlerTest, HandlesCaseInsensitiveExtensions) {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string content = "Test content";
    
    // Test uppercase extensions
    std::vector<std::string> test_files = {"test.TXT", "notes.MD", "doc.PDF"};
    
    for (const auto& filename : test_files) {
        std::string form_body = create_multipart_form(boundary, filename, content);
        request req = create_post_request(form_body, boundary);
        
        auto response = handler_->handle_request(req);
        
        ASSERT_NE(response, nullptr);
        EXPECT_EQ(response->status, reply::ok) << "Failed for file: " << filename;
        EXPECT_THAT(response->content, HasSubstr("Upload Successful"));
    }
}

// Test exact path matching
TEST_F(UploadHandlerTest, RejectsInvalidPaths) {
    request req = create_get_request();
    
    // Test various invalid paths
    std::vector<std::string> invalid_paths = {
        "/uploadXYZ",           // Prefix match but not exact
        "/upload123",           // Prefix match but not exact  
        "/uploading",           // Prefix match but not exact
        "/wrong-path",          // Completely wrong
        "/upload_handler"       // Similar but not exact
    };
    
    for (const auto& path : invalid_paths) {
        req.uri = path;
        auto response = handler_->handle_request(req);
        
        ASSERT_NE(response, nullptr);
        EXPECT_EQ(response->status, reply::not_found) << "Should reject path: " << path;
    }
}

// Test valid subpaths
TEST_F(UploadHandlerTest, AcceptsValidSubpaths) {
    request req = create_get_request();
    
    // Test valid paths (exact match and subpaths)
    std::vector<std::string> valid_paths = {
        "/upload",              // Exact match
        "/upload/",             // With trailing slash
        "/upload/subpath"       // Subpath
    };
    
    for (const auto& path : valid_paths) {
        req.uri = path;
        auto response = handler_->handle_request(req);
        
        ASSERT_NE(response, nullptr);
        EXPECT_NE(response->status, reply::not_found) << "Should accept path: " << path;
    }
}