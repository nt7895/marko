#ifndef NGINX_CONFIG_PARSER_H
#define NGINX_CONFIG_PARSER_H

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>

// Forward declarations
class NginxConfig;
class NginxConfigStatement;

struct HandlerConfig {
  std::string type;
  std::unique_ptr<NginxConfig> config;
};

// The parsed representation of a single config statement.
class NginxConfigStatement {
 public:
  std::string ToString(int depth);
  std::vector<std::string> tokens_;
  std::unique_ptr<NginxConfig> child_block_;
};

// The parsed representation of the entire config.
class NginxConfig {
 public:
  std::string ToString(int depth = 0);
  
  // Extract the port number from config
  bool ExtractPort(std::string& port_num);
  
  // Extract handler configurations from the config file
  std::map<std::string, HandlerConfig> ExtractHandlerConfigs();
  
  // Find a specific token value in the configuration (helper method)
  std::string FindConfigToken(const std::string& token_name) const;
  
  std::vector<std::shared_ptr<NginxConfigStatement>> statements_;
};

// The driver that parses a config file and generates an NginxConfig.
class NginxConfigParser {
 public:
  NginxConfigParser() {}

  bool Parse(std::istream* config_file, NginxConfig* config);
  bool Parse(const char* file_name, NginxConfig* config);

  enum TokenType {
    TOKEN_TYPE_START = 0,
    TOKEN_TYPE_NORMAL = 1,
    TOKEN_TYPE_START_BLOCK = 2,
    TOKEN_TYPE_END_BLOCK = 3,
    TOKEN_TYPE_COMMENT = 4,
    TOKEN_TYPE_STATEMENT_END = 5,
    TOKEN_TYPE_EOF = 6,
    TOKEN_TYPE_ERROR = 7
  };
  // const char* TokenTypeAsString(TokenType type);

  enum TokenParserState {
    TOKEN_STATE_INITIAL_WHITESPACE = 0,
    TOKEN_STATE_SINGLE_QUOTE = 1,
    TOKEN_STATE_DOUBLE_QUOTE = 2,
    TOKEN_STATE_TOKEN_TYPE_COMMENT = 3,
    TOKEN_STATE_TOKEN_TYPE_NORMAL = 4
  };

  TokenType ParseToken(std::istream* input, std::string* value);
};

#endif // NGINX_CONFIG_PARSER_H