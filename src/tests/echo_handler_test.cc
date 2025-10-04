#include "gtest/gtest.h"
#include "echo_handler.hpp"
#include "request.hpp"
#include "reply.hpp"

namespace http {
namespace server {

// Reusable fixture for EchoHandler unit tests
class EchoHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        req.method = "GET";
        req.uri = "/echo/test";
        req.http_version_major = 1;
        req.http_version_minor = 1;
        req.headers = {
            {"Host", "localhost"},
            {"User-Agent", "UnitTest/1.0"}
        };
    }

    request req;
    EchoHandler handler;
};

TEST_F(EchoHandlerTest, BasicGETRequestProducesEchoedContent) {
    // Use new API: handler returns reply directly
    std::unique_ptr<reply> rep = handler.handle_request(req);
    
    std::string expected_status_line = "GET /echo/test HTTP/1.1\r\n";
    std::string expected_host_header = "Host: localhost\r\n";
    std::string expected_user_agent = "User-Agent: UnitTest/1.0\r\n";
    
    EXPECT_EQ(rep->status, reply::ok);
    EXPECT_NE(rep->content.find(expected_status_line), std::string::npos);
    EXPECT_NE(rep->content.find(expected_host_header), std::string::npos);
    EXPECT_NE(rep->content.find(expected_user_agent), std::string::npos);
    
    // Find content type header
    bool found_content_type = false;
    for (const auto& header : rep->headers) {
        if (header.name == "Content-Type" && header.value == "text/plain") {
            found_content_type = true;
            break;
        }
    }
    EXPECT_TRUE(found_content_type);
}
    
TEST_F(EchoHandlerTest, HandlesEmptyMethod) {
    req.method = "";
    std::unique_ptr<reply> rep = handler.handle_request(req);
    
    EXPECT_EQ(rep->status, reply::ok);
    EXPECT_TRUE(rep->content.find(" /echo/test HTTP/1.1") != std::string::npos);
}
    
TEST_F(EchoHandlerTest, HandlesEmptyHeaders) {
    req.headers.clear();
    std::unique_ptr<reply> rep = handler.handle_request(req);
    
    EXPECT_EQ(rep->status, reply::ok);
    EXPECT_TRUE(rep->content.find("GET /echo/test HTTP/1.1\r\n") != std::string::npos);
    EXPECT_TRUE(rep->content.find("Host:") == std::string::npos);
}
    
TEST_F(EchoHandlerTest, HandlesDifferentMethodAndUri) {
    req.method = "POST";
    req.uri = "/echo/hello/world";
    std::unique_ptr<reply> rep = handler.handle_request(req);
    
    EXPECT_EQ(rep->status, reply::ok);
    EXPECT_TRUE(rep->content.find("POST /echo/hello/world HTTP/1.1\r\n") != std::string::npos);
}

}  // namespace server
}  // namespace http