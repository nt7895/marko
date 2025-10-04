#ifndef ENTITY_PROCESSOR_H
#define ENTITY_PROCESSOR_H

#include <string>
#include <vector>
#include <memory>
#include <boost/filesystem.hpp>


namespace http {
namespace server {

// Processes db operations on entities
class EntityProcessor {
public:
  EntityProcessor(const std::string& data_path);
  virtual ~EntityProcessor() = default;
  
  // CRUD Functions that interact with our db
  virtual std::string CreateEntity(const std::string& entity_type, const std::string& json_data);
  virtual bool RetrieveEntity(const std::string& entity_type, const std::string& id, std::string& json_data);
  virtual bool UpdateEntity(const std::string& entity_type, const std::string& id, const std::string& json_data);
  virtual bool DeleteEntity(const std::string& entity_type, const std::string& id);
  virtual bool ListEntities(const std::string& entity_type, std::vector<std::string>& ids);

private:
  std::string data_path_;
  
  // Helper functions
  std::string GetNextId(const std::string& entity_type);
  std::string GetEntityPath(const std::string& entity_type);
  std::string GetEntityItemPath(const std::string& entity_type, const std::string& id);
  bool isValidEntityDirectory(const std::string& entity_type);
};

}
}

#endif // ENTITY_PROCESSOR_H