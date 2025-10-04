#include "gtest/gtest.h"
#include "server.h"
#include <boost/asio.hpp>
#include <map>
#include <string>

using boost::asio::ip::tcp;

class ServerTest : public ::testing::Test {
protected:
    boost::asio::io_service io_service_;
    const short test_port = 8080;
    // Test handler configs
    std::map<std::string, HandlerConfig> handler_configs_;
    
    void SetUp() override {
        // Create a test handler config
        HandlerConfig echo_config;
        echo_config.type = "EchoHandler";
        handler_configs_["/echo"] = std::move(echo_config);
        
        HandlerConfig static_config;
        static_config.type = "StaticHandler";
        handler_configs_["/static"] = std::move(static_config);
    }
};

// Test server initialization
TEST_F(ServerTest, ServerInitializes) {
    // Simply test that the server constructs without throwing exceptions
    ASSERT_NO_THROW({
        server test_server(io_service_, test_port, handler_configs_);
    });
}

// Test server port binding
TEST_F(ServerTest, ServerBindsToPort) {
    // Create first server
    server test_server(io_service_, test_port, handler_configs_);
    
    // Try to create another server on the same port
    boost::asio::io_service another_io_service;
    bool caught_exception = false;
    
    try {
        server another_server(another_io_service, test_port, handler_configs_);
    } catch (const boost::system::system_error& e) {
        caught_exception = true;
    }
    
    EXPECT_TRUE(caught_exception) << "Expected exception when binding to same port";
}

// Test server with different ports
TEST_F(ServerTest, ServerUsesSpecifiedPort) {
    const short port1 = 8081;
    const short port2 = 8082;
    
    // Should be able to create two servers with different ports
    ASSERT_NO_THROW({
        server server1(io_service_, port1, handler_configs_);
        server server2(io_service_, port2, handler_configs_);
    });
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}