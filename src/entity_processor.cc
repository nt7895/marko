#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include "entity_processor.h"

namespace fs = boost::filesystem;

namespace http {
namespace server {

EntityProcessor::EntityProcessor(const std::string& data_path) 
  : data_path_(data_path) {
}


// Handles POST
std::string EntityProcessor::CreateEntity(const std::string& entity_type, const std::string& json_data) {
  std::string id = GetNextId(entity_type);
  
  if (!isValidEntityDirectory(entity_type)) {
    return "";
  }
  
  std::string file_path = GetEntityItemPath(entity_type, id);
  std::ofstream file(file_path);
  if (!file.is_open()) {
    return "";
  }
  
  file << json_data;
  file.close();
  
  return id;
}

// Handles GET by ID
bool EntityProcessor::RetrieveEntity(const std::string& entity_type, const std::string& id, std::string& json_data) {
  std::string file_path = GetEntityItemPath(entity_type, id);
  if (!fs::exists(file_path)) {
    return false;
  }
  
  std::ifstream file(file_path);
  if (!file.is_open()) {
    return false;
  }
  
  std::stringstream buffer;
  buffer << file.rdbuf();
  json_data = buffer.str();
  
  return true;
}

// Handles PUT
bool EntityProcessor::UpdateEntity(const std::string& entity_type, const std::string& id, const std::string& json_data) {
  std::string file_path = GetEntityItemPath(entity_type, id);
  
  // Check if entity exists
  if (!fs::exists(file_path)) {
    return false;
  }
  
  // Write the updated data
  std::ofstream file(file_path);
  if (!file.is_open()) {
    return false;
  }
  
  file << json_data;
  file.close();
  
  return true;
}

// Handles DELETE
bool EntityProcessor::DeleteEntity(const std::string& entity_type, const std::string& id) {
  std::string file_path = GetEntityItemPath(entity_type, id);
  
  // Check if entity exists
  if (!fs::exists(file_path)) {
    return false;
  }
  
  try {
    return fs::remove(file_path);
  } catch (const std::exception& e) {
    std::cerr << "Error deleting file: " << e.what() << std::endl;
    return false;
  }
}

// Handles GET all
bool EntityProcessor::ListEntities(const std::string& entity_type, std::vector<std::string>& ids) {
  std::string entity_path = GetEntityPath(entity_type);
  
  // Check if entity directory exists
  if (!fs::exists(entity_path)) {
    return false;
  }
  
  try {
    for (fs::directory_iterator it(entity_path); it != fs::directory_iterator(); ++it) {
      if (fs::is_regular_file(it->path())) {
        ids.push_back(it->path().filename().string());
      }
    }
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Error listing entities: " << e.what() << std::endl;
    return false;
  }
}

/*---------- Helper Functions ----------*/

std::string EntityProcessor::GetNextId(const std::string& entity_type) {
  std::string entity_path = GetEntityPath(entity_type);
  
  if (!fs::exists(entity_path)) {
    return "1";
  }
  
  int max_id = 0;
  
  try {
    for (fs::directory_iterator it(entity_path); it != fs::directory_iterator(); ++it) {
      try {
        int current_id = std::stoi(it->path().filename().string());
        max_id = std::max(max_id, current_id);
      } catch (const std::exception& e) {
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error finding next ID: " << e.what() << std::endl;
  }
  
  return std::to_string(max_id + 1);
}

std::string EntityProcessor::GetEntityPath(const std::string& entity_type) {
  return data_path_ + "/" + entity_type;
}

std::string EntityProcessor::GetEntityItemPath(const std::string& entity_type, const std::string& id) {
  return GetEntityPath(entity_type) + "/" + id;
}

bool EntityProcessor::isValidEntityDirectory(const std::string& entity_type) {
  std::string entity_path = GetEntityPath(entity_type);
  if (fs::exists(entity_path)) {
    return true;
  }
  
  try {
    return fs::create_directories(entity_path);
  } catch (const std::exception& e) {
    std::cerr << "Error creating directory: " << e.what() << std::endl;
    return false;
  }
}

}
} 