#include <sstream>
#include <iostream>
#include "api_handler.h"


namespace http {
namespace server {

APIHandler::APIHandler(const std::string& path_prefix, std::unique_ptr<EntityProcessor> entity_processor)
  : path_prefix_(path_prefix), entity_processor_(std::move(entity_processor)) {
}

bool APIHandler::Register() {
  std::cout << "Registering APIHandler" << std::endl;
  return RequestHandlerRegistry::RegisterHandler("APIHandler", APIHandler::Init);
}

static struct APIHandlerRegistrar {
  APIHandlerRegistrar() {
    std::cout << "APIHandlerRegistrar constructor called" << std::endl;
    APIHandler::Register();
  }
} APIHandlerRegistrar;


std::unique_ptr<reply> APIHandler::handle_request(const request& request) {
  // Parse the URI to get the entity type and ID if it's a part of the request
  std::string entity_type, id;
  if (!ParseUri(request.uri, entity_type, id)) {
    return BuildResponse(reply::bad_request, "Invalid API request URI");
  }
  
  // Routes the HTTP method in request
  if (request.method == "POST" && !entity_type.empty() && id.empty()) {
    return HandleCreate(entity_type, request.body);
  } 
  else if (request.method == "GET" && !entity_type.empty() && !id.empty()) {
    return HandleRetrieve(entity_type, id);
  }
  else if (request.method == "PUT" && !entity_type.empty() && !id.empty()) {
    return HandleUpdate(entity_type, id, request.body);
  }
  else if (request.method == "DELETE" && !entity_type.empty() && !id.empty()) {
    return HandleDelete(entity_type, id);
  }
  else if (request.method == "GET" && !entity_type.empty() && id.empty()) {
    return HandleList(entity_type);
  }
  
  // Invalid endpoint
  return BuildResponse(reply::not_found, "404 Not Found");
}

bool APIHandler::ParseUri(const std::string& uri, std::string& entity_type, std::string& id) {
  if (uri.compare(0, path_prefix_.length(), path_prefix_) != 0) {
    return false;
  }
  
  std::string path = uri.substr(path_prefix_.length());
  if (!path.empty() && path[0] == '/') {
    path = path.substr(1);
  }
  
  // Gets entity type and id if it exists
  size_t slash_pos = path.find('/');
  if (slash_pos == std::string::npos) {
    entity_type = path;
    id = "";
  } else {
    entity_type = path.substr(0, slash_pos);
    id = path.substr(slash_pos + 1);
  }
  
  return !entity_type.empty();
}

std::unique_ptr<reply> APIHandler::HandleCreate(const std::string& entity_type, const std::string& json_data) {
  // Validate input
  if (json_data.empty()) {  // Validation not in requirements, but could be added later on
    return BuildResponse(reply::bad_request, "Invalid JSON data");
  }
  
  std::string id = entity_processor_->CreateEntity(entity_type, json_data);
  if (id.empty()) {
    return BuildResponse(reply::internal_server_error, "Failed to create entity");
  }
  
  std::string response = "{\"id\": " + id + "}";
  
  std::vector<header> headers;
  headers.push_back({"Content-Type", "application/json"});
  
  return BuildResponse(reply::ok, response, headers);
}

std::unique_ptr<reply> APIHandler::HandleRetrieve(const std::string& entity_type, const std::string& id) {
  std::string json_data;
  if (!entity_processor_->RetrieveEntity(entity_type, id, json_data)) {
    return BuildResponse(reply::not_found, "404 Not Found");
  }
  
  std::vector<header> headers;
  headers.push_back({"Content-Type", "application/json"});
  
  return BuildResponse(reply::ok, json_data, headers);
}

std::unique_ptr<reply> APIHandler::HandleUpdate(const std::string& entity_type, const std::string& id, const std::string& json_data) {
  // Validate input
  if (json_data.empty()) {
    return BuildResponse(reply::bad_request, "Invalid JSON data");
  }
  
  if (!entity_processor_->UpdateEntity(entity_type, id, json_data)) {
    return BuildResponse(reply::not_found, "404 Not Found");
  }
  
  std::vector<header> headers;
  headers.push_back({"Content-Type", "application/json"});
  
  return BuildResponse(reply::ok, "{\"status\": \"success\"}", headers);
}

std::unique_ptr<reply> APIHandler::HandleDelete(const std::string& entity_type, const std::string& id) {
  if (!entity_processor_->DeleteEntity(entity_type, id)) {
    return BuildResponse(reply::not_found, "404 Not Found");
  }
  
  std::vector<header> headers;
  headers.push_back({"Content-Type", "application/json"});
  
  return BuildResponse(reply::ok, "{\"status\": \"success\"}", headers);
}

std::unique_ptr<reply> APIHandler::HandleList(const std::string& entity_type) {
  std::vector<std::string> ids;
  if (!entity_processor_->ListEntities(entity_type, ids)) {
    return BuildResponse(reply::not_found, "404 Not Found");
  }
  
  // Convert vector of IDs to JSON array
  std::string response = "[";
  for (size_t i = 0; i < ids.size(); ++i) {
    response += ids[i];
    if (i < ids.size() - 1) {
      response += ",";
    }
  }
  response += "]";
  
  std::vector<header> headers;
  headers.push_back({"Content-Type", "application/json"});
  
  return BuildResponse(reply::ok, response, headers);
}

}
}