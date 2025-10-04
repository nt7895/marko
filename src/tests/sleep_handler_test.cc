#include "gtest/gtest.h"
#include "sleep_handler.h"
#include "request.hpp"
#include "reply.hpp"
#include <ctime>

namespace http {
namespace server {

// Reusable fixture for SleepHandler unit tests
class SleepHandlerTest : public ::testing::Test {
    protected:
        void SetUp() override {
            req.method = "GET";
            req.uri = "/sleep";
            req.http_version_major = 1;
            req.http_version_minor = 1;
        }

        request req;
        SleepHandler handler;
    };

    TEST_F(SleepHandlerTest, BasicSleepRequest) {
        std::time_t start = std::time(nullptr);
        auto rep = handler.handle_request(req);
        std::time_t end = std::time(nullptr);

        // check OK status
        EXPECT_EQ(rep->status, http::server::reply::ok);
        EXPECT_EQ(rep->content, "The request slept for 5 seconds\r\n");

        // Find content type and content length header
        bool found_content_length = false;
        bool found_content_type = false;
        for (const auto& header : rep->headers) {
            if (header.name == "Content-Type" && header.value == "text/plain")
                found_content_type = true;
            if (header.name == "Content-Length" && header.value == "33")
                found_content_length = true;
        }
        EXPECT_TRUE(found_content_length);
        EXPECT_TRUE(found_content_type);
        
        // Handling request should take 5 seconds
        EXPECT_GE(std::difftime(end, start), 5);
    }
}
}