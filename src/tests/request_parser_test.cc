#include "gtest/gtest.h"
#include "request_parser.hpp"
#include "request.hpp"

namespace {

// Test fixture for request parser tests, providing common setup for all test cases
class RequestParserTest : public ::testing::Test {
protected:
  void SetUp() override {
    parser.reset();
    // Reset request object to clean state
    req = http::server::request();
  }

  http::server::request_parser parser;
  http::server::request req;
};

// Test handling of invalid method character
TEST_F(RequestParserTest, InvalidMethodCharacter) {
  const char* request_data = "@ET /index.html HTTP/1.1\r\n\r\n";
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::bad);
}

// Test handling of invalid character in method
TEST_F(RequestParserTest, InvalidCharacterInMethod) {
  const char* request_data = "G@T /index.html HTTP/1.1\r\n\r\n";
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::bad);
}

// Test handling of invalid URI character
TEST_F(RequestParserTest, InvalidUriCharacter) {
  // Control character in URI
  char request_data[] = {'G', 'E', 'T', ' ', '/', '\x1F', '.', 'h', 't', 'm', 'l', ' ', 
                         'H', 'T', 'T', 'P', '/', '1', '.', '1', '\r', '\n', '\r', '\n', '\0'};
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::bad);
}

// Test parsing HTTP version with multiple digits
TEST_F(RequestParserTest, MultiDigitHttpVersion) {
  const char* request_data = "GET /index.html HTTP/12.34\r\n\r\n";
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::good);
  EXPECT_EQ(req.http_version_major, 12);
  EXPECT_EQ(req.http_version_minor, 34);
}

// Test incorrect HTTP version - no major digit
TEST_F(RequestParserTest, NoMajorVersionDigit) {
  const char* request_data = "GET /index.html HTTP/A.1\r\n\r\n";
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::bad);
}

// Test incorrect HTTP version - no minor digit
TEST_F(RequestParserTest, NoMinorVersionDigit) {
  const char* request_data = "GET /index.html HTTP/1.B\r\n\r\n";
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::bad);
}

// Test header line with only carriage return (no newline)
TEST_F(RequestParserTest, InvalidHeaderLineEnding) {
  char request_data[] = {'G', 'E', 'T', ' ', '/', ' ', 'H', 'T', 'T', 'P', '/', '1', '.', '1', '\r', 'X', '\0'};
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::bad);
}

// Test header line with invalid characters
TEST_F(RequestParserTest, InvalidHeaderCharacter) {
  char request_data[] = {'G', 'E', 'T', ' ', '/', ' ', 'H', 'T', 'T', 'P', '/', '1', '.', '1', '\r', '\n', 
                         '\x1F', 'a', 'm', 'e', ':', ' ', 'v', 'a', 'l', 'u', 'e', '\r', '\n', '\r', '\n', '\0'};
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::bad);
}

// Test header with invalid character in name
TEST_F(RequestParserTest, InvalidHeaderNameCharacter) {
  const char* request_data = "GET / HTTP/1.1\r\nHe@der: value\r\n\r\n";
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::bad);
}

// Test header line continuation (header line word splitting)
TEST_F(RequestParserTest, HeaderLineContinuation) {
  const char* request_data = "GET / HTTP/1.1\r\nHeader: value1\r\n continued-value\r\n\r\n";
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::good);
  EXPECT_EQ(req.headers.size(), 1);
  EXPECT_EQ(req.headers[0].name, "Header");
  EXPECT_EQ(req.headers[0].value, "value1continued-value");
}

// Test header line continuation with control character
TEST_F(RequestParserTest, InvalidHeaderLineContinuation) {
  char request_data[] = {'G', 'E', 'T', ' ', '/', ' ', 'H', 'T', 'T', 'P', '/', '1', '.', '1', '\r', '\n',
                         'H', 'e', 'a', 'd', 'e', 'r', ':', ' ', 'v', 'a', 'l', 'u', 'e', '\r', '\n',
                         ' ', '\x1F', 'c', 'o', 'n', 't', 'i', 'n', 'u', 'e', 'd', '\r', '\n', '\r', '\n', '\0'};
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::bad);
}

// Test header without space after colon
TEST_F(RequestParserTest, NoSpaceAfterHeaderColon) {
  const char* request_data = "GET / HTTP/1.1\r\nHeader:value\r\n\r\n";
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::bad);
}

// Test header with control character in value
TEST_F(RequestParserTest, ControlCharInHeaderValue) {
  char request_data[] = {'G', 'E', 'T', ' ', '/', ' ', 'H', 'T', 'T', 'P', '/', '1', '.', '1', '\r', '\n',
                         'H', 'e', 'a', 'd', 'e', 'r', ':', ' ', 'v', 'a', 'l', '\x1F', 'u', 'e', '\r', '\n',
                         '\r', '\n', '\0'};
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::bad);
}

// Test header with second line but no continuation
TEST_F(RequestParserTest, InvalidSecondHeaderLine) {
  const char* request_data = "GET / HTTP/1.1\r\nHeader: value\r\nX\r\n\r\n";
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::bad);
  // The test expects this to fail because "X" in the header line is missing a colon
  // which violates the HTTP specification for header format
}

// Test incremental parsing (feeding one character at a time)
TEST_F(RequestParserTest, IncrementalParsing) {
  const char* request_data = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";
  size_t length = strlen(request_data);
  
  for (size_t i = 0; i < length - 1; ++i) {
    auto result = parser.parse(req, request_data + i, request_data + i + 1);
    EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::indeterminate);
  }
  
  // Last character should complete the request
  auto result = parser.parse(req, request_data + length - 1, request_data + length);
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::good);
  EXPECT_EQ(req.method, "GET");
  EXPECT_EQ(req.uri, "/");
  EXPECT_EQ(req.headers.size(), 1);
  EXPECT_EQ(req.headers[0].name, "Host");
  EXPECT_EQ(req.headers[0].value, "example.com");
}

// Test final newline without preceding header
TEST_F(RequestParserTest, MissingFinalNewline) {
  const char* request_data = "GET / HTTP/1.1\r\n\r";
  auto result = parser.parse(req, request_data, request_data + strlen(request_data));
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::indeterminate);
}

// Test parser reset functionality
TEST_F(RequestParserTest, ParserReset) {
  // First parse a partial request
  const char* request_data1 = "GET /index.html HTTP/1.1\r\n";
  auto result1 = parser.parse(req, request_data1, request_data1 + strlen(request_data1));
  EXPECT_EQ(std::get<0>(result1), http::server::request_parser::result_type::indeterminate);
  
  // Reset the parser and the request object
  parser.reset();
  req = http::server::request(); // Reset the request object too
  
  // Parse a different complete request
  const char* request_data2 = "POST /submit HTTP/1.0\r\n\r\n";
  auto result2 = parser.parse(req, request_data2, request_data2 + strlen(request_data2));
  
  // Verify the parser started from scratch
  EXPECT_EQ(std::get<0>(result2), http::server::request_parser::result_type::good);
  EXPECT_EQ(req.method, "POST");
  EXPECT_EQ(req.uri, "/submit");
  EXPECT_EQ(req.http_version_major, 1);
  EXPECT_EQ(req.http_version_minor, 0);
}

// Test maximum method length handling
TEST_F(RequestParserTest, LongMethodHandling) {
  // Create a very long method name (50 characters)
  std::string long_method(50, 'X');
  std::string request_str = long_method + " / HTTP/1.1\r\n\r\n";
  auto result = parser.parse(req, request_str.c_str(), request_str.c_str() + request_str.length());
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::good);
  EXPECT_EQ(req.method, long_method);
}

// Test maximum URI length handling
TEST_F(RequestParserTest, LongUriHandling) {
  // Create a very long URI (1000 characters)
  std::string long_uri(1000, 'x');
  std::string request_str = "GET /" + long_uri + " HTTP/1.1\r\n\r\n";
  auto result = parser.parse(req, request_str.c_str(), request_str.c_str() + request_str.length());
  
  EXPECT_EQ(std::get<0>(result), http::server::request_parser::result_type::good);
  EXPECT_EQ(req.uri, "/" + long_uri);
}

}