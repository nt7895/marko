#include "gtest/gtest.h"
#include "static_handler.h"
#include "request.hpp"
#include "reply.hpp"
#include <fstream>
#include <filesystem>

namespace http {
namespace server {

// Reusable fixture for StaticFileHandler unit tests
class StaticHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory structure
        std::filesystem::create_directory("./test_root");
        
        // Create a test HTML file
        std::ofstream html_file("./test_root/test.html");
        html_file << "<html><body><h1>Test HTML</h1></body></html>";
        html_file.close();
        
        // Create a test text file
        std::ofstream text_file("./test_root/test.txt");
        text_file << "This is a test text file.";
        text_file.close();
        
        // Initialize request with default values
        req.method = "GET";
        req.uri = "/static/test.html";
        req.http_version_major = 1;
        req.http_version_minor = 1;
        req.headers = {
            {"Host", "localhost"},
            {"User-Agent", "UnitTest/1.0"}
        };
        
        // Create handler with test root directory and path prefix
        handler = std::make_unique<StaticFileHandler>("/test_root", "/static");
    }
    
    void TearDown() override {
        // Clean up test files and directory
        std::filesystem::remove_all("./test_root");
    }
    
    request req;
    std::unique_ptr<StaticFileHandler> handler;
};

TEST_F(StaticHandlerTest, ServesHTMLFileWithCorrectMimeType) {
    // Use new API: handler returns reply directly
    std::unique_ptr<reply> rep = handler->handle_request(req);
    
    EXPECT_EQ(rep->status, reply::ok);
    EXPECT_EQ(rep->content, "<html><body><h1>Test HTML</h1></body></html>");
    
    // Check for Content-Length header
    bool has_content_length = false;
    for (const auto& header : rep->headers) {
        if (header.name == "Content-Length") {
            has_content_length = true;
            EXPECT_EQ(header.value, std::to_string(rep->content.size()));
            break;
        }
    }
    EXPECT_TRUE(has_content_length);
    
    // Check for Content-Type header
    bool has_content_type = false;
    for (const auto& header : rep->headers) {
        if (header.name == "Content-Type") {
            has_content_type = true;
            EXPECT_EQ(header.value, "text/html");
            break;
        }
    }
    EXPECT_TRUE(has_content_type);
}

TEST_F(StaticHandlerTest, ServesTextFileWithCorrectMimeType) {
    req.uri = "/static/test.txt";
    std::unique_ptr<reply> rep = handler->handle_request(req);
    
    EXPECT_EQ(rep->status, reply::ok);
    EXPECT_EQ(rep->content, "This is a test text file.");
    
    // Check for Content-Type header
    bool has_content_type = false;
    for (const auto& header : rep->headers) {
        if (header.name == "Content-Type") {
            has_content_type = true;
            EXPECT_EQ(header.value, "text/plain");
            break;
        }
    }
    EXPECT_TRUE(has_content_type);
}

TEST_F(StaticHandlerTest, Returns404ForNonexistentFile) {
    req.uri = "/static/nonexistent.html";
    std::unique_ptr<reply> rep = handler->handle_request(req);
    
    EXPECT_EQ(rep->status, reply::not_found);
    EXPECT_EQ(rep->content, "404 Not Found");
    
    // Check for Content-Type header
    bool has_content_type = false;
    for (const auto& header : rep->headers) {
        if (header.name == "Content-Type") {
            has_content_type = true;
            EXPECT_EQ(header.value, "text/plain");
            break;
        }
    }
    EXPECT_TRUE(has_content_type);
}

TEST_F(StaticHandlerTest, HandlesURIWithQueryParameters) {
    req.uri = "/static/test.html?param1=value1&param2=value2";
    std::unique_ptr<reply> rep = handler->handle_request(req);
    
    EXPECT_EQ(rep->status, reply::ok);
    EXPECT_EQ(rep->content, "<html><body><h1>Test HTML</h1></body></html>");
    
    // Check for Content-Type header
    bool has_content_type = false;
    for (const auto& header : rep->headers) {
        if (header.name == "Content-Type") {
            has_content_type = true;
            EXPECT_EQ(header.value, "text/html");
            break;
        }
    }
    EXPECT_TRUE(has_content_type);
}

TEST_F(StaticHandlerTest, Returns404ForInvalidPathPrefix) {
    req.uri = "/invalid/test.html";
    std::unique_ptr<reply> rep = handler->handle_request(req);
    
    EXPECT_EQ(rep->status, reply::not_found);
    EXPECT_EQ(rep->content, "404 Not Found");
}

TEST_F(StaticHandlerTest, DetectsCorrectMimeTypeForVariousExtensions) {
    // Test various file extensions by creating temporary files
    struct TestFile {
        std::string filename;
        std::string content;
        std::string expected_mime;
    };
    
    std::vector<TestFile> test_files = {
        {"test.css", "body { color: red; }", "text/css"},
        {"test.js", "function test() {}", "application/javascript"},
        {"test.json", "{\"test\": true}", "application/json"},
        {"test.xml", "<root><test>true</test></root>", "application/xml"},
        {"test.pdf", "%PDF-1.0 test", "application/pdf"}
    };
    
    for (const auto& tf : test_files) {
        // Create the test file
        std::ofstream file("./test_root/" + tf.filename);
        file << tf.content;
        file.close();
        
        // Test the handler
        req.uri = "/static/" + tf.filename;
        std::unique_ptr<reply> rep = handler->handle_request(req);
        
        EXPECT_EQ(rep->status, reply::ok) << "Failed for " << tf.filename;
        EXPECT_EQ(rep->content, tf.content) << "Failed for " << tf.filename;
        
        // Check for Content-Type header
        bool has_correct_mime = false;
        for (const auto& header : rep->headers) {
            if (header.name == "Content-Type" && header.value == tf.expected_mime) {
                has_correct_mime = true;
                break;
            }
        }
        EXPECT_TRUE(has_correct_mime) << "Failed for " << tf.filename;
    }
}

TEST_F(StaticHandlerTest, HandlesUnknownFileExtensionWithDefaultMimeType) {
    // Create a file with an unknown extension
    std::ofstream unknown_file("./test_root/test.unknown");
    unknown_file << "Content with unknown type";
    unknown_file.close();
    
    req.uri = "/static/test.unknown";
    std::unique_ptr<reply> rep = handler->handle_request(req);
    
    EXPECT_EQ(rep->status, reply::ok);
    EXPECT_EQ(rep->content, "Content with unknown type");
    
    // Check for Content-Type header
    bool has_content_type = false;
    for (const auto& header : rep->headers) {
        if (header.name == "Content-Type") {
            has_content_type = true;
            EXPECT_EQ(header.value, "application/octet-stream"); // Default MIME type
            break;
        }
    }
    EXPECT_TRUE(has_content_type);
}

TEST_F(StaticHandlerTest, HandlesRequestWithMultiplePathSegments) {
    // Create a nested directory structure
    std::filesystem::create_directory("./test_root/nested");
    std::ofstream nested_file("./test_root/nested/test.html");
    nested_file << "<html><body><h1>Nested HTML</h1></body></html>";
    nested_file.close();
    
    req.uri = "/static/nested/test.html";
    std::unique_ptr<reply> rep = handler->handle_request(req);
    
    EXPECT_EQ(rep->status, reply::ok);
    EXPECT_EQ(rep->content, "<html><body><h1>Nested HTML</h1></body></html>");
    
    // Check for Content-Type header
    bool has_content_type = false;
    for (const auto& header : rep->headers) {
        if (header.name == "Content-Type") {
            has_content_type = true;
            EXPECT_EQ(header.value, "text/html");
            break;
        }
    }
    EXPECT_TRUE(has_content_type);
}

}  // namespace server
}  // namespace http