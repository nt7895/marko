#include "gtest/gtest.h"
#include "reply.hpp"
#include <boost/asio.hpp>

TEST(ReplyTest, StockReply) {
    std::unique_ptr<http::server::reply> rep = http::server::reply::stock_reply(http::server::reply::status_type::ok, "Hello, World!");
    
    EXPECT_EQ(rep->status, http::server::reply::status_type::ok);
    EXPECT_EQ(rep->content, "Hello, World!");
    EXPECT_EQ(rep->headers.size(), 2); // Expecting Content-Length and Content-Type
    EXPECT_EQ(rep->headers[0].name, "Content-Length");
    EXPECT_EQ(rep->headers[0].value, std::to_string(rep->content.size()));
    EXPECT_EQ(rep->headers[1].name, "Content-Type");
    EXPECT_EQ(rep->headers[1].value, "text/plain");
}

TEST(ReplyTest, OutputFormatting) {
    std::unique_ptr<http::server::reply> rep = http::server::reply::stock_reply(http::server::reply::status_type::bad_request, "Bad Request");
    
    auto buffers = rep->to_buffers();
    
    std::string expected_response = "HTTP/1.1 400 Bad Request\r\nContent-Length: " + std::to_string(rep->content.size()) + "\r\nContent-Type: text/plain\r\n\r\nBad Request";
    
    // Here we would need to concatenate all buffers to check the full output
    std::string actual_response;
    for (const auto& buffer : buffers) {
        actual_response.append(boost::asio::buffer_cast<const char*>(buffer), boost::asio::buffer_size(buffer));
    }

    EXPECT_EQ(actual_response, expected_response);
}

// Test all status codes via to_buffers() output
TEST(ReplyTest, AllStatusCodes) {
    // Helper function to extract status line from buffers
    auto get_status_line = [](const std::vector<boost::asio::const_buffer>& buffers) -> std::string {
        // Status line is always the first buffer
        return std::string(
            boost::asio::buffer_cast<const char*>(buffers[0]), 
            boost::asio::buffer_size(buffers[0])
        );
    };
    
    // Test each status code
    std::unique_ptr<http::server::reply> ok_reply = http::server::reply::stock_reply(http::server::reply::ok, "");
    EXPECT_EQ(get_status_line(ok_reply->to_buffers()), "HTTP/1.1 200 OK\r\n");
    
    std::unique_ptr<http::server::reply> created_reply = http::server::reply::stock_reply(http::server::reply::created, "");
    EXPECT_EQ(get_status_line(created_reply->to_buffers()), "HTTP/1.1 201 Created\r\n");
    
    std::unique_ptr<http::server::reply> accepted_reply = http::server::reply::stock_reply(http::server::reply::accepted, "");
    EXPECT_EQ(get_status_line(accepted_reply->to_buffers()), "HTTP/1.1 202 Accepted\r\n");
    
    std::unique_ptr<http::server::reply> no_content_reply = http::server::reply::stock_reply(http::server::reply::no_content, "");
    EXPECT_EQ(get_status_line(no_content_reply->to_buffers()), "HTTP/1.1 204 No Content\r\n");
    
    std::unique_ptr<http::server::reply> multiple_choices_reply = http::server::reply::stock_reply(http::server::reply::multiple_choices, "");
    EXPECT_EQ(get_status_line(multiple_choices_reply->to_buffers()), "HTTP/1.1 300 Multiple Choices\r\n");
    
    std::unique_ptr<http::server::reply> moved_permanently_reply = http::server::reply::stock_reply(http::server::reply::moved_permanently, "");
    EXPECT_EQ(get_status_line(moved_permanently_reply->to_buffers()), "HTTP/1.1 301 Moved Permanently\r\n");
    
    std::unique_ptr<http::server::reply> moved_temporarily_reply = http::server::reply::stock_reply(http::server::reply::moved_temporarily, "");
    EXPECT_EQ(get_status_line(moved_temporarily_reply->to_buffers()), "HTTP/1.1 302 Moved Temporarily\r\n");
    
    std::unique_ptr<http::server::reply> not_modified_reply = http::server::reply::stock_reply(http::server::reply::not_modified, "");
    EXPECT_EQ(get_status_line(not_modified_reply->to_buffers()), "HTTP/1.1 304 Not Modified\r\n");
    
    std::unique_ptr<http::server::reply> bad_request_reply = http::server::reply::stock_reply(http::server::reply::bad_request, "");
    EXPECT_EQ(get_status_line(bad_request_reply->to_buffers()), "HTTP/1.1 400 Bad Request\r\n");
    
    std::unique_ptr<http::server::reply> unauthorized_reply = http::server::reply::stock_reply(http::server::reply::unauthorized, "");
    EXPECT_EQ(get_status_line(unauthorized_reply->to_buffers()), "HTTP/1.1 401 Unauthorized\r\n");
    
    std::unique_ptr<http::server::reply> forbidden_reply = http::server::reply::stock_reply(http::server::reply::forbidden, "");
    EXPECT_EQ(get_status_line(forbidden_reply->to_buffers()), "HTTP/1.1 403 Forbidden\r\n");
    
    std::unique_ptr<http::server::reply> not_found_reply = http::server::reply::stock_reply(http::server::reply::not_found, "");
    EXPECT_EQ(get_status_line(not_found_reply->to_buffers()), "HTTP/1.1 404 Not Found\r\n");
    
    std::unique_ptr<http::server::reply> internal_server_error_reply = http::server::reply::stock_reply(http::server::reply::internal_server_error, "");
    EXPECT_EQ(get_status_line(internal_server_error_reply->to_buffers()), "HTTP/1.1 500 Internal Server Error\r\n");
    
    std::unique_ptr<http::server::reply> not_implemented_reply = http::server::reply::stock_reply(http::server::reply::not_implemented, "");
    EXPECT_EQ(get_status_line(not_implemented_reply->to_buffers()), "HTTP/1.1 501 Not Implemented\r\n");
    
    std::unique_ptr<http::server::reply> bad_gateway_reply = http::server::reply::stock_reply(http::server::reply::bad_gateway, "");
    EXPECT_EQ(get_status_line(bad_gateway_reply->to_buffers()), "HTTP/1.1 502 Bad Gateway\r\n");
    
    std::unique_ptr<http::server::reply> service_unavailable_reply = http::server::reply::stock_reply(http::server::reply::service_unavailable, "");
    EXPECT_EQ(get_status_line(service_unavailable_reply->to_buffers()), "HTTP/1.1 503 Service Unavailable\r\n");
    
    // Test invalid status (should default to internal_server_error)
    auto invalid_reply = std::make_unique<http::server::reply>();
    invalid_reply->status = static_cast<http::server::reply::status_type>(999);
    invalid_reply->content = "";
    EXPECT_EQ(get_status_line(invalid_reply->to_buffers()), "HTTP/1.1 500 Internal Server Error\r\n");
}

// Test reply with multiple headers
TEST(ReplyTest, MultipleHeaders) {
    auto rep = std::make_unique<http::server::reply>();
    rep->status = http::server::reply::ok;
    rep->content = "Test Content";
    
    // Add headers
    http::server::header h1 = {"Content-Length", std::to_string(rep->content.size())};
    http::server::header h2 = {"Content-Type", "text/plain"};
    http::server::header h3 = {"Connection", "close"};
    http::server::header h4 = {"Server", "Boost-Asio-HTTP-Server"};
    
    rep->headers.push_back(h1);
    rep->headers.push_back(h2);
    rep->headers.push_back(h3);
    rep->headers.push_back(h4);
    
    auto buffers = rep->to_buffers();
    
    // We expect status line + 4 headers (name, separator, value, CRLF for each) + CRLF + content
    EXPECT_EQ(buffers.size(), 1 + 4*4 + 1 + 1);
    
    // Verify the content is the last buffer
    EXPECT_EQ(std::string(boost::asio::buffer_cast<const char*>(buffers.back()), 
                         boost::asio::buffer_size(buffers.back())), 
             "Test Content");
}

// Test empty reply
TEST(ReplyTest, EmptyReply) {
    auto rep = std::make_unique<http::server::reply>();
    rep->status = http::server::reply::no_content;
    rep->content = ""; // Empty content
    
    auto buffers = rep->to_buffers();
    
    // Even with empty content, we should have a buffer for it
    EXPECT_GE(buffers.size(), 1);
    
    // Last buffer should be empty but present
    EXPECT_EQ(boost::asio::buffer_size(buffers.back()), 0);
}

// Test malformed req reply
TEST(ReplyTest, MalformedReqReply) {
    // helper function for converting buffer to string
    auto to_str = [](const std::vector<boost::asio::const_buffer>& buffers) -> std::string {
        std::string response = "";
        for (int i = 0; i < buffers.size(); i++) 
            response += std::string(boost::asio::buffer_cast<const char*>(buffers[i]), boost::asio::buffer_size(buffers[i]));
        return response;
    };

    auto rep = std::make_unique<http::server::reply>();
    rep = rep->build_malformed_req_response();

    // buffer contains status line (1), 2 headers (2 * 4), CRLF (1), and content (1)
    EXPECT_EQ(rep->to_buffers().size(), 1 + 2*4 + 1 + 1);

    // check reply's content is same as expected content
    EXPECT_EQ(to_str(rep->to_buffers()), "HTTP/1.1 400 Bad Request\r\nContent-Length: 46\r\nContent-Type: text/plain\r\n\r\nRequest is malformed and cannot be processed\r\n");
} 
// Test different content types
TEST(ReplyTest, DifferentContentTypes) {
    // HTML content
    std::unique_ptr<http::server::reply> html_rep = http::server::reply::stock_reply(http::server::reply::ok, "<html><body>Test</body></html>");
    html_rep->headers[1].value = "text/html";
    
    // JSON content
    std::unique_ptr<http::server::reply> json_rep = http::server::reply::stock_reply(http::server::reply::ok, "{\"message\":\"Test\"}");
    json_rep->headers[1].value = "application/json";
    
    // Check that the content type is set correctly
    EXPECT_EQ(html_rep->headers[1].name, "Content-Type");
    EXPECT_EQ(html_rep->headers[1].value, "text/html");
    
    EXPECT_EQ(json_rep->headers[1].name, "Content-Type");
    EXPECT_EQ(json_rep->headers[1].value, "application/json");
    
    // Check buffers contain correct content types
    auto html_buffers = html_rep->to_buffers();
    auto json_buffers = json_rep->to_buffers();
    
    std::string html_response;
    for (const auto& buffer : html_buffers) {
        html_response.append(boost::asio::buffer_cast<const char*>(buffer), boost::asio::buffer_size(buffer));
    }
    
    std::string json_response;
    for (const auto& buffer : json_buffers) {
        json_response.append(boost::asio::buffer_cast<const char*>(buffer), boost::asio::buffer_size(buffer));
    }
    
    EXPECT_TRUE(html_response.find("Content-Type: text/html") != std::string::npos);
    EXPECT_TRUE(json_response.find("Content-Type: application/json") != std::string::npos);
}