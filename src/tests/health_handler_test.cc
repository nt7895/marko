#include "gtest/gtest.h"
#include "health_handler.h"
#include "request.hpp"
#include "reply.hpp"

namespace http {
namespace server {
// Reusable fixture for HealthHandler unit tests
class HealthHandlerTest : public ::testing::Test {
    protected:
        void SetUp() override {
            req.method = "GET";
            req.uri = "/health";
            req.http_version_major = 1;
            req.http_version_minor = 1;
        }

        request req;
        HealthHandler handler;
    };

    TEST_F(HealthHandlerTest, BasicGoodHealthRequest) {
        auto rep = handler.handle_request(req);
        
        // check OK status
        EXPECT_EQ(rep->status, http::server::reply::ok);
        EXPECT_EQ(rep->content, "OK\r\n");

        // Find content type and content length header
        bool found_content_length = false;
        bool found_content_type = false;
        for (const auto& header : rep->headers) {
            if (header.name == "Content-Type" && header.value == "text/plain")
                found_content_type = true;
            if (header.name == "Content-Length" && header.value == "4")
                found_content_length = true;
        }
        EXPECT_TRUE(found_content_length);
        EXPECT_TRUE(found_content_type);
    }
}
}