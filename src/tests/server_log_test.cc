#include "gtest/gtest.h"
#include "../src/server_log.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>

// Test Fixture class for multiple tests
class ServerLogTest : public testing::Test {
  protected:
  server_log log;
};

// Tests a file named test.log is created after starting a server log
TEST_F(ServerLogTest, FileCreated) {
    system("rm ../log_files/FileCreated_test.log");
    log.start_logging("../log_files/FileCreated_test.log");
    log.log_new_client_connection("1.1.1.1", "12345");

    std::ifstream file("../log_files/FileCreated_test.log");
    EXPECT_TRUE(file.is_open());
    file.close();
}

// Tests that server logs are written to file 
TEST_F(ServerLogTest, LoggingToFile) {
    system("rm ../log_files/LoggingToFile_test.log");
    log.start_logging("../log_files/LoggingToFile_test.log");
    log.log_config_parser_status(true);
    
    std::ifstream file("../log_files/LoggingToFile_test.log");
    std::string line;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            std::string expected_line = "Server config file successfully parsed";
            EXPECT_TRUE(line.substr(line.length() - expected_line.length(), line.length()) == expected_line);
        }
        file.close();
    }
}

// Tests that server logs are written to console 
TEST_F(ServerLogTest, LoggingToConsole) {
    system("rm ../log_files/LoggingToConsole_test.log");
    log.start_logging("../log_files/LoggingToConsole_test.log");

    // Save ptr to console output
    std::streambuf* old = std::cout.rdbuf();
    
    // Temporarily connect stdout to buffer stream
    std::stringstream buffer_stream;
    std::cout.rdbuf(buffer_stream.rdbuf());

    log.log_server_startup("80");
    
    // Set stdout ptr back to console output
    std::cout.rdbuf(old);

    // Store console output as string
    std::string message = buffer_stream.str();

    std::string expected_message = "[ServerStart] message:\"Server has started running\" port:80\n";
    std::string sub_message = message.substr(message.length() - expected_message.length(), message.length());

    EXPECT_EQ(expected_message, sub_message);
}