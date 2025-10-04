#include "gtest/gtest.h"
#include "session.h"
#include <boost/system/error_code.hpp>
#include <boost/asio/error.hpp>
#include <vector>
#include "request_handler_registry.h"

// using namespace boost::system;
// using namespace session;

// Helper to turn string into buffer pointers
static std::pair<const char*, const char*>
  str_span(const std::string& s) {
    return { s.data(), s.data() + s.size() };
}

class server_session_logic_test : public ::testing::Test {
 protected:
  void SetUp() override {
    io_service_ = std::make_shared<boost::asio::io_service>();
    handler_registry_ = std::make_shared<http::server::RequestHandlerRegistry>();
        
    // Initialize the handler registry with some test handlers
    std::map<std::string, HandlerConfig> handler_configs;
    HandlerConfig echo_config;
    echo_config.type = "EchoHandler";
    handler_configs["/echo"] = std::move(echo_config);
        
    handler_registry_->Init(handler_configs);
    
    s = std::make_unique<session>(*io_service_, *handler_registry_);
  }
  
  std::shared_ptr<boost::asio::io_service> io_service_;
  std::shared_ptr<http::server::RequestHandlerRegistry> handler_registry_;
  std::unique_ptr<session> s;
};

// 1) GET request should yield WriteResponse with nonempty buffers
TEST_F(server_session_logic_test, ProcessRead_GetRequest) {
  std::string req = "GET / HTTP/1.1\r\nHost: example\r\n\r\n";
  std::vector<boost::asio::const_buffer> out;
  auto span = str_span(req);
  auto action = s->process_read(boost::system::error_code{}, span.first, span.second, out);
  EXPECT_EQ(action, session::SessionAction::WriteResponse);
  EXPECT_FALSE(out.empty());
}

// 2) Non-GET still yields WriteResponse but empty buffers
TEST_F(server_session_logic_test, ProcessRead_NonGetRequest) {
  std::string req = "POST / HTTP/1.1\r\n\r\n";
  std::vector<boost::asio::const_buffer> out{{}};
  auto span = str_span(req);
  auto action = s->process_read(boost::system::error_code{}, span.first, span.second, out);
  EXPECT_EQ(action, session::SessionAction::WriteResponse);
  // buffers cleared
  EXPECT_TRUE(out.empty());
}

// 3) Error on read => Close
TEST_F(server_session_logic_test, ProcessRead_ErrorCloses) {
    std::vector<boost::asio::const_buffer> out;
    boost::system::error_code ec(
        boost::asio::error::connection_reset,
        boost::asio::error::get_system_category());
    auto action = s->process_read(ec, nullptr, nullptr, out);
    EXPECT_EQ(action, session::SessionAction::Close);
  }

// 4) Successful write => ReadAgain
TEST_F(server_session_logic_test, ProcessWrite_Success) {
  auto action = s->process_write(boost::system::error_code{});
  EXPECT_EQ(action, session::SessionAction::ReadAgain);
}

// 5) Write error => Close
TEST_F(server_session_logic_test, ProcessWrite_ErrorCloses) {
    boost::system::error_code ec(
        boost::asio::error::operation_aborted,
        boost::asio::error::get_system_category());
    auto action = s->process_write(ec);
    EXPECT_EQ(action, session::SessionAction::Close);
  }