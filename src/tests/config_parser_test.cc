#include "gtest/gtest.h"
#include "../src/config_parser.h"
#include <sstream>
#include <fstream>

// Enhanced test fixture that provides helpers for creating configs
class ConfigParserExtendedTest : public testing::Test {
protected:
  NginxConfigParser parser;
  NginxConfig out_config;
  std::string port;

  // Helper to create a config from a string
  bool ParseString(const std::string& config_string) {
    std::stringstream config_stream(config_string);
    return parser.Parse(&config_stream, &out_config);
  }
  bool ParseFile(const std::string& file_name) {
    return parser.Parse(file_name.c_str(), &out_config);
  }
};

// TEST: Extract handler configurations from a valid config
TEST_F(ConfigParserExtendedTest, ExtractHandlerConfigs_Valid) {
  const std::string config_string = 
    "port 8080;\n"
    "location /echo EchoHandler {}\n"
    "location /static StaticHandler {\n"
    "  root /var/www/html;\n"
    "}\n"
    "# this is a \"comment\"\n"
    "foo \"bar\";\n";
  
  ASSERT_TRUE(ParseString(config_string));

  std::string port_num;
  ASSERT_TRUE(out_config.ExtractPort(port_num));
  EXPECT_EQ(port_num, "8080");
  
  auto handler_configs = out_config.ExtractHandlerConfigs();
  EXPECT_EQ(handler_configs.size(), 2);
  
  // Check first handler
  EXPECT_TRUE(handler_configs.find("/echo") != handler_configs.end());
  EXPECT_EQ(handler_configs["/echo"].type, "EchoHandler");
  EXPECT_NE(handler_configs["/echo"].config.get(), nullptr);
  
  // Check second handler
  EXPECT_TRUE(handler_configs.find("/static") != handler_configs.end());
  EXPECT_EQ(handler_configs["/static"].type, "StaticHandler");
  EXPECT_NE(handler_configs["/static"].config.get(), nullptr);
  
  // Check config value for static handler
  std::string root_value = handler_configs["/static"].config->FindConfigToken("root");
  EXPECT_EQ(root_value, "/var/www/html");
}

// TEST: Handler with trailing slash should be skipped
TEST_F(ConfigParserExtendedTest, ExtractHandlerConfigs_TrailingSlash) {
  const std::string config_string = 
    "port 8080;\n"
    "location /echo/ EchoHandler {}\n"
    "location /static StaticHandler {}\n";
  
  ASSERT_TRUE(ParseString(config_string));
  
  auto handler_configs = out_config.ExtractHandlerConfigs();
  EXPECT_EQ(handler_configs.size(), 1);
  EXPECT_TRUE(handler_configs.find("/echo/") == handler_configs.end());
  EXPECT_TRUE(handler_configs.find("/static") != handler_configs.end());
}

TEST_F(ConfigParserExtendedTest, ParseFromFile) {
  const std::string config_string = 
    "port 8080;\n"
    "location /echo EchoHandler {}\n"
    "location /static StaticHandler {\n"
    "  root /var/www/html;\n"
    "}\n";
  
  // temp file creation
  std::ofstream config_file("test_config.conf");
  ASSERT_TRUE(config_file.is_open());
  // Write the config string to the file
  config_file << config_string;
  config_file.close();

  ASSERT_FALSE(ParseFile("non_existent_file.conf"));
  
  ASSERT_TRUE(ParseFile("test_config.conf"));
  
  auto handler_configs = out_config.ExtractHandlerConfigs();
  EXPECT_EQ(handler_configs.size(), 2);
  
  // Check first handler
  EXPECT_TRUE(handler_configs.find("/echo") != handler_configs.end());
  EXPECT_EQ(handler_configs["/echo"].type, "EchoHandler");
  
  // Check second handler
  EXPECT_TRUE(handler_configs.find("/static") != handler_configs.end());
  EXPECT_EQ(handler_configs["/static"].type, "StaticHandler");
}

// TEST: Duplicate handler paths should be skipped
TEST_F(ConfigParserExtendedTest, ExtractHandlerConfigs_DuplicatePaths) {
  const std::string config_string = 
    "port 8080;\n"
    "location /echo EchoHandler {}\n"
    "location /echo StaticHandler {}\n";
  
  ASSERT_TRUE(ParseString(config_string));
  
  auto handler_configs = out_config.ExtractHandlerConfigs();
  EXPECT_EQ(handler_configs.size(), 1);
  EXPECT_EQ(handler_configs["/echo"].type, "EchoHandler");
}

// TEST: Empty configuration should return empty map
TEST_F(ConfigParserExtendedTest, ExtractHandlerConfigs_EmptyConfig) {
  const std::string config_string = "port 8080;";
  
  ASSERT_TRUE(ParseString(config_string));
  
  auto handler_configs = out_config.ExtractHandlerConfigs();
  EXPECT_TRUE(handler_configs.empty());
}

// TEST: Multiple handlers with different configurations
TEST_F(ConfigParserExtendedTest, ExtractHandlerConfigs_MultipleHandlers) {
  const std::string config_string = 
    "port 8080;\n"
    "location /echo EchoHandler {}\n"
    "location /static StaticHandler {\n"
    "  root /var/www/html;\n"
    "}\n"
    "location /api ApiHandler {\n"
    "  auth_token abc123;\n"
    "  timeout 30;\n"
    "}\n";
  
  ASSERT_TRUE(ParseString(config_string));
  
  auto handler_configs = out_config.ExtractHandlerConfigs();
  EXPECT_EQ(handler_configs.size(), 3);
  
  // Check API handler configuration
  EXPECT_TRUE(handler_configs.find("/api") != handler_configs.end());
  EXPECT_EQ(handler_configs["/api"].type, "ApiHandler");
  EXPECT_NE(handler_configs["/api"].config.get(), nullptr);
  
  std::string auth_token = handler_configs["/api"].config->FindConfigToken("auth_token");
  EXPECT_EQ(auth_token, "abc123");
  
  std::string timeout = handler_configs["/api"].config->FindConfigToken("timeout");
  EXPECT_EQ(timeout, "30");
}

// TEST: Root path handler configuration
TEST_F(ConfigParserExtendedTest, ExtractHandlerConfigs_RootPath) {
  const std::string config_string = 
    "port 8080;\n"
    "location / DefaultHandler {}\n";
  
  ASSERT_TRUE(ParseString(config_string));
  
  auto handler_configs = out_config.ExtractHandlerConfigs();
  EXPECT_EQ(handler_configs.size(), 1);
  EXPECT_TRUE(handler_configs.find("/") != handler_configs.end());
  EXPECT_EQ(handler_configs["/"].type, "DefaultHandler");
}

// TEST: Single character path handler configuration
TEST_F(ConfigParserExtendedTest, ExtractHandlerConfigs_SingleCharPath) {
  const std::string config_string = 
    "port 8080;\n"
    "location /a ShortPathHandler {}\n";
  
  ASSERT_TRUE(ParseString(config_string));
  
  auto handler_configs = out_config.ExtractHandlerConfigs();
  EXPECT_EQ(handler_configs.size(), 1);
  EXPECT_TRUE(handler_configs.find("/a") != handler_configs.end());
  EXPECT_EQ(handler_configs["/a"].type, "ShortPathHandler");
}

// TEST: Nested configuration blocks
TEST_F(ConfigParserExtendedTest, ExtractHandlerConfigs_NestedBlocks) {
  const std::string config_string = 
    "port 8080;\n"
    "location /complex ComplexHandler {\n"
    "  outer_param value1;\n"
    "  nested_block {\n"
    "    inner_param value2;\n"
    "  }\n"
    "}\n";
  
  ASSERT_TRUE(ParseString(config_string));
  
  auto handler_configs = out_config.ExtractHandlerConfigs();
  EXPECT_EQ(handler_configs.size(), 1);
  EXPECT_TRUE(handler_configs.find("/complex") != handler_configs.end());
  
  // Check outer param
  std::string outer_param = handler_configs["/complex"].config->FindConfigToken("outer_param");
  EXPECT_EQ(outer_param, "value1");
  
  // Check that the nested block structure is preserved
  bool has_nested_block = false;
  for (const auto& stmt : handler_configs["/complex"].config->statements_) {
    if (stmt->tokens_.size() > 0 && stmt->tokens_[0] == "nested_block" && stmt->child_block_) {
      has_nested_block = true;
      std::string inner_param = stmt->child_block_->FindConfigToken("inner_param");
      EXPECT_EQ(inner_param, "value2");
    }
  }
  EXPECT_TRUE(has_nested_block);
}

// TEST: FindConfigToken in nested blocks
TEST_F(ConfigParserExtendedTest, FindConfigToken_NestedBlocks) {
  const std::string config_string = 
    "outer1 value1;\n"
    "block1 {\n"
    "  inner1 value2;\n"
    "  block2 {\n"
    "    inner2 value3;\n"
    "  }\n"
    "}\n"
    "outer2 value4;\n";
  
  ASSERT_TRUE(ParseString(config_string));
  
  // Test direct token access
  EXPECT_EQ(out_config.FindConfigToken("outer1"), "value1");
  EXPECT_EQ(out_config.FindConfigToken("outer2"), "value4");
  
  // Test nested token access
  EXPECT_EQ(out_config.FindConfigToken("inner1"), "value2");
  EXPECT_EQ(out_config.FindConfigToken("inner2"), "value3");
  
  // Test non-existent token
  EXPECT_EQ(out_config.FindConfigToken("nonexistent"), "");
}

// TEST: ToString method for configs with nested blocks
TEST_F(ConfigParserExtendedTest, ToString_NestedBlocks) {
  const std::string config_string = 
    "server {\n"
    "  listen 80;\n"
    "  location / {\n"
    "    root /var/www/html;\n"
    "  }\n"
    "}\n";
  
  ASSERT_TRUE(ParseString(config_string));
  
  std::string serialized = out_config.ToString();
  
  // Check that the serialized string contains all the key tokens
  EXPECT_NE(serialized.find("server"), std::string::npos);
  EXPECT_NE(serialized.find("listen 80"), std::string::npos);
  EXPECT_NE(serialized.find("location /"), std::string::npos);
  EXPECT_NE(serialized.find("root /var/www/html"), std::string::npos);
  
  // Check that braces are properly balanced
  int open_braces = 0, close_braces = 0;
  for (char c : serialized) {
    if (c == '{') open_braces++;
    if (c == '}') close_braces++;
  }
  EXPECT_EQ(open_braces, close_braces);
}

// TEST: Invalid location specification (missing path or handler type)
TEST_F(ConfigParserExtendedTest, ExtractHandlerConfigs_InvalidFormat) {
  const std::string config_string = 
    "port 8080;\n"
    "location EchoHandler {}\n"; // Missing path
  
  ASSERT_TRUE(ParseString(config_string));
  
  auto handler_configs = out_config.ExtractHandlerConfigs();
  EXPECT_TRUE(handler_configs.empty());
}

// TEST: Multiple handlers with same prefix but different length
TEST_F(ConfigParserExtendedTest, ExtractHandlerConfigs_SimilarPaths) {
  const std::string config_string = 
    "port 8080;\n"
    "location /api ApiHandler {}\n"
    "location /api/v1 ApiV1Handler {}\n"
    "location /api/v2 ApiV2Handler {}\n";
  
  ASSERT_TRUE(ParseString(config_string));
  
  auto handler_configs = out_config.ExtractHandlerConfigs();
  EXPECT_EQ(handler_configs.size(), 3);
  
  EXPECT_TRUE(handler_configs.find("/api") != handler_configs.end());
  EXPECT_TRUE(handler_configs.find("/api/v1") != handler_configs.end());
  EXPECT_TRUE(handler_configs.find("/api/v2") != handler_configs.end());
  
  EXPECT_EQ(handler_configs["/api"].type, "ApiHandler");
  EXPECT_EQ(handler_configs["/api/v1"].type, "ApiV1Handler");
  EXPECT_EQ(handler_configs["/api/v2"].type, "ApiV2Handler");
}

// TEST: Special characters in paths
TEST_F(ConfigParserExtendedTest, ExtractHandlerConfigs_SpecialCharPaths) {
  const std::string config_string = 
    "port 8080;\n"
    "location /path-with-dash DashHandler {}\n"
    "location /path_with_underscore UnderscoreHandler {}\n"
    "location /path.with.dots DotsHandler {}\n";
  
  ASSERT_TRUE(ParseString(config_string));
  
  auto handler_configs = out_config.ExtractHandlerConfigs();
  EXPECT_EQ(handler_configs.size(), 3);
  
  EXPECT_TRUE(handler_configs.find("/path-with-dash") != handler_configs.end());
  EXPECT_TRUE(handler_configs.find("/path_with_underscore") != handler_configs.end());
  EXPECT_TRUE(handler_configs.find("/path.with.dots") != handler_configs.end());
}

// TEST: APIHandler's local data_path parameter
TEST_F(ConfigParserExtendedTest, ExtractDataPathLocal) {
  const std::string config_string =
    "location /api APIHandler {\n"
    "  data_path ./database;\n"
    "}\n";

  ASSERT_TRUE(ParseString(config_string));
  auto handler_configs = out_config.ExtractHandlerConfigs();
  ASSERT_EQ(handler_configs.count("/api"), 1);
  std::string path = handler_configs["/api"].config->FindConfigToken("data_path");
  EXPECT_EQ(path, "./database");
}

// TEST: APIHandler's prod (docker) data_path parameter
TEST_F(ConfigParserExtendedTest, ExtractDataPathProd) {
  const std::string config_string =
    "location /api APIHandler {\n"
    "  data_path /mnt/storage/crud;\n"
    "}\n";

  ASSERT_TRUE(ParseString(config_string));
  auto handler_configs = out_config.ExtractHandlerConfigs();
  ASSERT_EQ(handler_configs.count("/api"), 1);
  std::string path = handler_configs["/api"].config->FindConfigToken("data_path");
  EXPECT_EQ(path, "/mnt/storage/crud");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}