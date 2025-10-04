#include "gtest/gtest.h"
#include "session.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include "request_handler_registry.h"
#include "echo_handler.hpp"  // Include all handler headers we'll test
#include "not_found_handler.hpp"
#include "static_handler.h"

// Define boost placeholders to avoid deprecated warning
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

class HandlerRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure factory map is initialized
        auto& factory_map = http::server::RequestHandlerRegistry::GetFactoryMap();
        
        // Make sure our handlers are registered
        http::server::EchoHandler::Register();
        http::server::NotFoundHandler::Register();
        http::server::StaticFileHandler::Register();
    }
};

TEST_F(HandlerRegistryTest, HandlersAreRegistered) {
    auto& factory_map = http::server::RequestHandlerRegistry::GetFactoryMap();
    
    // Check that our handlers are registered
    EXPECT_TRUE(factory_map.find("EchoHandler") != factory_map.end());
    EXPECT_TRUE(factory_map.find("NotFoundHandler") != factory_map.end());
    EXPECT_TRUE(factory_map.find("StaticHandler") != factory_map.end());
}

// Make this test class a friend of session
class server_session_test : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure handlers are registered first
        http::server::EchoHandler::Register();
        http::server::NotFoundHandler::Register();
        http::server::StaticFileHandler::Register();
        
        io_service_ = std::make_shared<boost::asio::io_service>();
        handler_registry_ = std::make_shared<http::server::RequestHandlerRegistry>();
        
        // Initialize the handler registry with some test handlers
        std::map<std::string, HandlerConfig> handler_configs;
        
        // Create the EchoHandler config using move semantics
        HandlerConfig echo_config;
        echo_config.type = "EchoHandler";
        // Use move semantics to transfer ownership
        handler_configs["/echo"] = std::move(echo_config);
        
        // Create the StaticHandler config for testing
        HandlerConfig static_config;
        static_config.type = "StaticHandler";
        static_config.config = std::make_unique<NginxConfig>();
        // Add root config token
        auto statement = std::make_shared<NginxConfigStatement>();
        statement->tokens_.push_back("root");
        statement->tokens_.push_back("/usr/src/static");
        static_config.config->statements_.push_back(statement);
        handler_configs["/static"] = std::move(static_config);
        
        // Initialize the registry
        ASSERT_TRUE(handler_registry_->Init(handler_configs));
        
        session_ = std::make_shared<session>(*io_service_, *handler_registry_);
    }
    
    void TearDown() override {
        // Make sure socket is closed
        if (session_->socket().is_open()) {
            session_->socket().close();
        }
    }
    
    std::shared_ptr<boost::asio::io_service> io_service_;
    std::shared_ptr<http::server::RequestHandlerRegistry> handler_registry_;
    std::shared_ptr<session> session_;
};

// Basic test to verify that a session can be constructed
TEST_F(server_session_test, ConstructorDoesNotThrow) {
    EXPECT_NO_THROW({
        session s(*io_service_, *handler_registry_);
    });
}

// Test to verify socket accessor works
TEST_F(server_session_test, SocketAccessorReturnsSocket) {
    boost::asio::ip::tcp::socket& socket = session_->socket();
    
    // Verify socket is associated with our io_service
    EXPECT_EQ(&socket.get_executor().context(), io_service_.get());
    
    // Verify socket is initially closed
    EXPECT_FALSE(socket.is_open());
}

// Test session deletion cleanup
TEST_F(server_session_test, DestructorDoesNotThrow) {
    session* s = new session(*io_service_, *handler_registry_);
    EXPECT_NO_THROW({
        delete s;
    });
}

// Test that socket options can be set
TEST_F(server_session_test, SocketOptionsCanBeSet) {
    boost::asio::ip::tcp::socket& socket = session_->socket();
    
    // Test socket options can be set
    boost::asio::socket_base::keep_alive option(true);
    EXPECT_NO_THROW({
        socket.open(boost::asio::ip::tcp::v4());
        socket.set_option(option);
    });
    
    // Verify option was set
    boost::asio::socket_base::keep_alive actual_option;
    socket.get_option(actual_option);
    EXPECT_TRUE(actual_option.value());
    
    // Clean up
    socket.close();
}

// Create a pair of connected endpoints for testing
class SessionWithConnectedSocketsTest : public server_session_test {
protected:
    void SetUp() override {
        server_session_test::SetUp();
        
        // Create a pair of connected endpoints
        boost::asio::ip::tcp::acceptor acceptor(*io_service_, boost::asio::ip::tcp::endpoint(
            boost::asio::ip::address_v4::loopback(), 0));
            
        // Start accept
        acceptor.listen();
        boost::asio::ip::tcp::endpoint server_endpoint = acceptor.local_endpoint();
        
        // Create client socket and connect
        client_socket_ = std::make_shared<boost::asio::ip::tcp::socket>(*io_service_);
        client_socket_->open(boost::asio::ip::tcp::v4());
        client_socket_->connect(server_endpoint);
        
        // Accept the connection on server side
        acceptor.accept(session_->socket());
    }
    
    void TearDown() override {
        if (client_socket_->is_open()) {
            client_socket_->close();
        }
        server_session_test::TearDown();
    }
    
    // Helper to send data from client to server
    void SendFromClient(const std::string& data) {
        boost::asio::write(*client_socket_, boost::asio::buffer(data));
    }
    
    // Helper to receive data from server to client
    std::string ReceiveFromClient(size_t max_size = 1024) {
        std::vector<char> buffer(max_size);
        size_t bytes_read = client_socket_->read_some(boost::asio::buffer(buffer));
        return std::string(buffer.data(), bytes_read);
    }
    
    std::shared_ptr<boost::asio::ip::tcp::socket> client_socket_;
};

// Test the session.start() method initiates reading
TEST_F(SessionWithConnectedSocketsTest, StartMethodInitiatesReading) {
    // Start the session
    session_->start();
    
    // Send a valid HTTP/1.1 GET request to our echo handler - this will be processed by handle_read
    std::string valid_get = "GET /echo HTTP/1.1\r\nHost: localhost\r\n\r\n";
    SendFromClient(valid_get);
    
    // Instead of running io_service synchronously which might block,
    // we'll run it in a separate thread with a timeout
    bool completed = false;
    std::thread io_thread([&]() {
        try {
            // Run until there are no more handlers or timeout
            boost::asio::deadline_timer timer(*io_service_);
            timer.expires_from_now(boost::posix_time::seconds(1));
            timer.async_wait([&](const boost::system::error_code&) {
                io_service_->stop();
            });
            
            io_service_->run();
            completed = true;
        } catch (const std::exception& e) {
            ADD_FAILURE() << "Exception in io_service thread: " << e.what();
        }
    });
    
    // Set a timeout for the test
    auto timeout = std::chrono::system_clock::now() + std::chrono::seconds(2);
    
    // Wait for io_service to complete or timeout
    while (!completed && std::chrono::system_clock::now() < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Clean up the thread
    if (io_thread.joinable()) {
        io_thread.join();
    }
    
    // Ensure the operation completed
    EXPECT_TRUE(completed) << "IO service operations did not complete within timeout";
    
    // Verify we received a response
    std::string response;
    try {
        response = ReceiveFromClient();
        EXPECT_FALSE(response.empty()) << "Received empty response";
        EXPECT_TRUE(response.find("200 OK") != std::string::npos) 
            << "Response doesn't contain '200 OK': " << response;
    } catch (const std::exception& e) {
        FAIL() << "Exception receiving response: " << e.what();
    }
}

// Test GET request functionality
TEST_F(SessionWithConnectedSocketsTest, HandlesValidGetRequest) {
    // Start the session
    session_->start();
    
    // Send a valid HTTP/1.1 GET request to our echo handler
    std::string valid_get = "GET /echo HTTP/1.1\r\nHost: localhost\r\n\r\n";
    SendFromClient(valid_get);
    
    // Run the io_service to process the request
    io_service_->run_one();  // Process read
    io_service_->run_one();  // Process write
    
    // Receive the response
    std::string response = ReceiveFromClient();
    
    // Verify the response contains HTTP/1.1 200 OK
    EXPECT_TRUE(response.find("200 OK") != std::string::npos);
}

// Test a GET request to a path not handled by EchoHandler
TEST_F(SessionWithConnectedSocketsTest, HandlesNotFoundPath) {
    session_->start();

    // Send a GET request to a path not handled by EchoHandler
    std::string bad_uri = "GET /nonexistent HTTP/1.1\r\nHost: localhost\r\n\r\n";
    SendFromClient(bad_uri);

    io_service_->run_one();  // handle_read
    io_service_->run_one();  // handle_write

    std::string response = ReceiveFromClient();
    EXPECT_TRUE(response.find("404 Not Found") != std::string::npos);
}

// Test static handler file serving
TEST_F(SessionWithConnectedSocketsTest, HandlesStaticFileRequest) {
    session_->start();

    // Send a GET request to the static handler path
    std::string static_uri = "GET /static/index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
    SendFromClient(static_uri);

    io_service_->run_one();  // handle_read
    io_service_->run_one();  // handle_write

    std::string response = ReceiveFromClient();
    EXPECT_TRUE(response.find("404 Not Found") != std::string::npos || 
                response.find("200 OK") != std::string::npos);
}

// New test for handler registration mechanism
TEST_F(HandlerRegistryTest, CanCreateHandlerFromRegistry) {
    http::server::RequestHandlerRegistry registry;
    
    // Create test config
    std::map<std::string, HandlerConfig> handler_configs;
    HandlerConfig echo_config;
    echo_config.type = "EchoHandler";
    handler_configs["/echo"] = std::move(echo_config);
    
    // Initialize the registry
    ASSERT_TRUE(registry.Init(handler_configs));
    
    std::string handler_name;

    // Create a handler for a URI
    auto handler = registry.CreateHandler("/echo/test", handler_name);
    EXPECT_NE(handler, nullptr);
    
    // Create a handler for an unknown URI
    auto not_found_handler = registry.CreateHandler("/unknown",  handler_name);
    EXPECT_NE(not_found_handler, nullptr);
}

int main(int argc, char** argv) {
    // Explicitly register all handlers at program start
    http::server::EchoHandler::Register();
    http::server::NotFoundHandler::Register();
    http::server::StaticFileHandler::Register();
    
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}