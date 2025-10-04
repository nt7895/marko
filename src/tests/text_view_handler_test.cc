#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "request.hpp"
#include "reply.hpp"
#include "text_view_handler.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>

namespace http {
namespace server {

// Reusable fixture for TextViewHandler unit tests
class TextViewHandlerTest : public ::testing::Test {
    protected:
        void SetUp() override {
            test_view_dir_ = "./test_uploads";

            req.method = "GET";
            req.uri = "/view";
            req.http_version_major = 1;
            req.http_version_minor = 1;
            
            if (!std::filesystem::exists(test_view_dir_)) {
                try {
                    std::filesystem::create_directories(test_view_dir_);
                } catch (const std::exception& e) {
                    std::cerr << "Error creating directory: " << e.what() << std::endl;
                }
            }

            handler = std::make_unique<TextViewHandler>(test_view_dir_);
        }

        void TearDown() override {
            if (std::filesystem::exists(test_view_dir_))
                std::filesystem::remove_all(test_view_dir_);
        }

        bool create_file(std::string file_name, std::string content) {
            std::ofstream file(test_view_dir_ + "/" + file_name);

            if (!file.is_open()) {
                std::cerr << "Error creating text file" << std::endl;
                return false; 
            }

            file << content;

            file.close(); 

            return true;
        }

        void check_headers(std::vector<header> headers, const std::string content_length, const std::string content_type,  bool& found_content_length, bool& found_content_type) {
            // Find content type and content length header
            for (const auto& header : headers) {
                if (header.name == "Content-Type" && header.value == content_type)
                    found_content_type = true;
                if (header.name == "Content-Length" && header.value == content_length)
                    found_content_length = true;
            }
        }
    

        request req;
        std::string test_view_dir_;
        std::unique_ptr<TextViewHandler> handler;
    };

    TEST_F(TextViewHandlerTest, TextViewRequest) {
        std::string text_content = "Line 1: This is a basic file test\nLine 2: ViewHandler should be able to handle it";

        EXPECT_TRUE(create_file("test.txt", text_content));

        req.uri += "/test.txt";
        auto rep = handler->handle_request(req);

        // check OK status
        EXPECT_EQ(rep->status, http::server::reply::ok);
        EXPECT_EQ(rep->content, text_content+"\r\n");

        // Find content type and content length header
        bool found_content_length = false;
        bool found_content_type = false;
        check_headers(rep->headers, std::to_string(text_content.size() + 2), "text/plain", found_content_length, found_content_type);

        EXPECT_TRUE(found_content_length);
        EXPECT_TRUE(found_content_type);

    }

    TEST_F(TextViewHandlerTest, MarkdownViewRequest) {
        std::string text_content = "# Heading 1\n" \
                                    "## Heading 2\n" \
                                    "This line is a paragraph. **Bolded** to *Italics*.\n" \
                                    "- Item 1\n" \
                                    "- Item 2\n" \
                                    "- Item 3\n" \
                                    "[UCLA](https://www.ucla.edu/)\n" \
                                    "1. Num 1\n" \
                                    "2. Num 2";

        EXPECT_TRUE(create_file("test.md", text_content));
        req.uri += "/test.md";
        auto rep = handler->handle_request(req);

        // check OK status
        EXPECT_EQ(rep->status, http::server::reply::ok);

        // expected rendered markdown output
        std::string rendered_markdown = "<!DOCTYPE html>\n<html>\n" \
                                        "<h1>Heading 1</h1>\n" \
                                        "<h2>Heading 2</h2>\n" \
                                        "<p>This line is a paragraph. <strong>Bolded</strong> to <em>Italics</em>.</p>\n" \
                                        "<ul>\n" \
                                        "<li>Item 1</li>\n" \
                                        "<li>Item 2</li>\n" \
                                        "<li>Item 3</li>\n" \
                                        "</ul>\n" \
                                        "<p><a href=\"https://www.ucla.edu/\">UCLA</a></p>\n" \
                                        "<ol>\n" \
                                        "<li>Num 1</li>\n" \
                                        "<li>Num 2</li>\n" \
                                        "</ol>\n" \
                                        "</html>";
        EXPECT_EQ(rep->content, rendered_markdown + "\r\n");

        // Find content type and content length header
        bool found_content_length = false;
        bool found_content_type = false;
        check_headers(rep->headers, std::to_string(rendered_markdown.size() + 2), "text/html", found_content_length, found_content_type);

        EXPECT_TRUE(found_content_length);
        EXPECT_TRUE(found_content_type);

    }

    TEST_F(TextViewHandlerTest, PDFViewRequest) {
        // not sure about paths on google cloud server
        std::string command = "cp ../test_pdf/test.pdf " + test_view_dir_ + "/test.pdf";
        int failure = system(command.c_str()); // returns 0 on success
        EXPECT_EQ(failure, 0);

        req.uri += "/test.pdf";
        auto rep = handler->handle_request(req);

        std::string expected_content = "This is a test pdf.\r\n";
        // check OK status
        EXPECT_EQ(rep->status, http::server::reply::ok);
        EXPECT_EQ(rep->content, expected_content);

        // Find content type and content length header
        bool found_content_length = false;
        bool found_content_type = false;
        check_headers(rep->headers, std::to_string(expected_content.size()), "text/plain", found_content_length, found_content_type);

        EXPECT_TRUE(found_content_length);
        EXPECT_TRUE(found_content_type);
    }

    TEST_F(TextViewHandlerTest, NoFileFound) {
        req.uri += "/unknown.txt";
        auto rep = handler->handle_request(req);

        // check 404 status
        EXPECT_EQ(rep->status, http::server::reply::not_found);
    }

    TEST_F(TextViewHandlerTest, UnsupportedFileType) {
        EXPECT_TRUE(create_file("unsupported.doc", "content doesn't matter"));
        req.uri += "/unsupported.doc";

        auto rep = handler->handle_request(req);

        // check not implemented status
        EXPECT_EQ(rep->status, http::server::reply::not_implemented);
    }

    TEST_F(TextViewHandlerTest, SpecialCharFileName) {
        std::string text_content = "TextViewHandler can handle special characters";

        EXPECT_TRUE(create_file("space between.txt", text_content));
        req.uri += "/space%20between.txt";
        auto rep = handler->handle_request(req);

        // check ok status
        EXPECT_EQ(rep->status, http::server::reply::ok);
        EXPECT_EQ(rep->content, text_content+"\r\n");

        // Find content type and content length header
        bool found_content_length = false;
        bool found_content_type = false;
        check_headers(rep->headers, std::to_string(text_content.size() + 2), "text/plain", found_content_length, found_content_type);
        EXPECT_TRUE(found_content_length);
        EXPECT_TRUE(found_content_type);
    }

    TEST_F(TextViewHandlerTest, PathAttack) {
        // trying to read passwords
        req.uri += "/../etc/passwd";
        auto rep = handler->handle_request(req);

        // check bad request status
        EXPECT_EQ(rep->status, http::server::reply::bad_request);
    }
}
}
