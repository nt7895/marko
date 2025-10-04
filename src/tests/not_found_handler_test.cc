#include "gtest/gtest.h"
#include "../src/not_found_handler.hpp"
#include "../src/request.hpp"
#include "../src/reply.hpp"

namespace http {
namespace server {

// Reusable fixture for NotFoundHandler tests 
class NotFoundHandlerTest : public ::testing::Test {
protected:
  void SetUp() override {
    req.method = "GET";
    req.uri = "/some/missing/resource";
    req.http_version_major = 1;
    req.http_version_minor = 1;
  }
  request req;
  NotFoundHandler handler;
};

TEST_F(NotFoundHandlerTest, Returns404Status) {
  auto rep = handler.handle_request(req);
  EXPECT_EQ(rep->status, reply::not_found);
}

TEST_F(NotFoundHandlerTest, SetsContentTypePlain) {
  auto rep = handler.handle_request(req);
  bool found=false;
  for (auto& h : rep->headers) {
    if (h.name=="Content-Type" && h.value=="text/plain") {
      found=true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(NotFoundHandlerTest, IncludesRequestedUriInBody) {
  auto rep = handler.handle_request(req);
  EXPECT_NE(rep->content.find(req.uri), std::string::npos);
}

}  // namespace server
}  // namespace http