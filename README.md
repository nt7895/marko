# Source Code Layout

1. config_parser.h: parses the config file
    - Parse(): checks correctness in the syntax of the config file
    - FindConfigToken(): finds the input token and returns the parameters
    - ExtractPort(): extracts the port using FindConfigToken(config, "listen")

***Gives port and url + handler map to server***

2. server_main.cc: main program for running the server

***Starts server setup***

3. server.h: starts and handles new client connections
    - start_accept(): creates new client session
    - handle_accept(): monitors client connection

***Creates client session***

4. session.h: reads requests and sends replies
    - start(): starts reading/writing 
    - handle_read(): operations for reading incoming requests
    - handle_write(): operations for sending replies to clients

***Gives stdin buffer to request parser***

5. request.hpp, request_parser.hpp: builds and checks syntax of request
    - parse(): reads read buffer and returns a well formatted request object

***Gives request to handler***

6. request_handler.h, static handler.h, echo_handler.h, not found handler.hp
    - RequestHandler(): constructor
    - handle_request(): performs handler specific operation
    - BuildResponse(): creates and formats a reply

***Sends reply***

7. reply.hpp:
    - stock_reply(): creates a reply object


# Building, Testing, and Running the code
## Build
In the root directory ```/marko```, run the following commands to create a ```/marko/build``` directory and perform an out of source build.
```
mkdir build
cd build
cmake ..
make
```

## Test
After completing all the steps in Build, run ```make test``` in ```/marko/build``` to run all the tests.
```
make test
```

## Run
After completing all the steps in Build, start the webserver by running ```./bin/server my_config``` in ```/marko/build```.
```
./bin/server my_config
```
By default:
 - **locally**, CMake has copied `my_config_local` → `my_config` (so you get `data_path ./database;`)
 - **in Docker**, the Dockerfile has copied `my_config_prod` → `my_config` (so you get `data_path /mnt/storage/crud;`)

This repository provides an example config file in ```/marko/my_config_local```. The config file currently listens on port 80. 


### Run in Docker with persistent storage

First, SSH into your GCE VM (replace `<INSTANCE_NAME>` and `<ZONE>` with your own):
```bash
# gcloud compute ssh <INSTANCE_NAME> --zone <ZONE>
# Example:
gcloud compute ssh web-server-new --zone us-west1-a
```

Then, on your **GCE VM host**, create and (once) format your disk directory:

```bash
# on your GCE VM host
sudo mkdir -p /mnt/disks/gce-containers-mounts/gce-persistent-disks/webserver-storage/crud
sudo chmod a+rwx /mnt/disks/gce-containers-mounts/gce-persistent-disks/webserver-storage/crud
```

Then launch your container (this maps that host‐disk into /mnt/storage inside the container):

```bash
docker run -d \
  --name web-server \
  --restart always \
  -v /mnt/disks/gce-containers-mounts/gce-persistent-disks/webserver-storage:/mnt/storage \
  -p 80:80 \
  gcr.io/marko-456521/marko:latest \
  my_config
```

# Verify your mount
Once your container is running, you can confirm the disk both on the host and inside the container:

```bash
# 1) On the VM host:
ls /mnt/disks/gce-containers-mounts/gce-persistent-disks/webserver-storage
# → you should see: crud

# 2) Inside the container:
docker ps                              # find your container name (here: web-server)
docker exec -it web-server /bin/sh     # open a shell
ls /mnt/storage                        # should list "crud" here too
```

The APIHandler (per my_config_prod) writes into /mnt/storage/crud, which persists across container (and VM) restarts.


### How to add custom files
If you have a custom file ```[FILE_NAME]``` you'd like to use during runtime (Ex: a custom config file), you can do so by following these steps.

1. In the root directory ```/marko```, add ```[FILE_NAME]```.

2. In ```/marko/CMakeLists.txt```, add these 2 lines to copy the file and give rwx permissions when performing out of source builds.
```
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/[FILE_NAME]
               ${CMAKE_CURRENT_BINARY_DIR}/[FILE_NAME]
               COPYONLY)
file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/[FILE_NAME]
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
```

# Adding a request handler
## 1. Add handler to the config file
In a config file, add a ```location``` block under the following format.
```
location [URL_PATH] [HANDLER_NAME] {
    [ARG1] [ARG1_PARAM];
    [ARG2] [ARG2_PARAM];
    ...
}
```
A handler's ```location``` block must contain:
- [URL_PATH]: path served by the request handler
- [HANDLER_NAME]: name of the request handler

If any arguments are required, then add arguments inside the ```location``` block:
- [ARG]: request handler specfic arguments
- [ARG_PARAM]: parameters of argument

### Config File Example (StaticFileHandler):
```
location /static StaticHandler {
  root ./usr;
}
```

## 2. Create handler header files
In the directory ```./marko/src```, add a header file with ```class [HANDLER_NAME]``` that extends ```class RequestHandler```. 

The new handler's header file should inherit the constructor and ```handle_request()``` using the following format.

###  Header File Format
```
#include "request_handler.hpp"

namespace http {
    namespace server {
        class [HANDLER_NAME]: public RequestHandler {
        public:
            [HANDLER_NAME]() {} // constructor
            std::unique_ptr<reply> handle_request(const request& request) override; // main method for request handling
            
            // additional functions + variables

        private:
            // additional functions + variables
        };

    } 
}
```
- [HANDLER_NAME]();
    - Constructor for handler
    - Use to initialize tables, lists, etc.

- std::unique_ptr<reply> handle_request(const request& request) override;
    - Inherited method from RequestHandler to perform operation 
    - Use BuildResponse() to format your reply (see more about BuildResponse() below)


The ```RequestHandler``` class also provides a helper function called ```BuildResponse()``` that formats the status, content, and headers into a proper reply. Use ```BuildResponse()``` to create your handler's reply.

```
BuildResponse()
ARGS:
reply::status_type status               // status of the request; see reply.h enum status_type
const std::string& content              // content of the reply
const std::vector<header>& headers      // headers of the reply
RETURN:
std::unique_ptr<reply>                  // formatted reply object
```

### Header File Example (StaticFileHandler):
More detailed example of the header file format using Static Handler
```
#include <string>
#include <map>
#include "request_handler.hpp"

namespace http {
    namespace server {

        class StaticFileHandler: public RequestHandler {
        public:
            StaticFileHandler(const std::string& root_dir, const std::string& path_prefix) 
            : root_dir_(root_dir), path_prefix_(path_prefix) {
                InitMimeTypeMap();
            }

            std::unique_ptr<reply> handle_request(const request& request) override; 

        private:
            std::string root_dir_;              
            std::string path_prefix_;
            std::map<std::string, std::string> mime_type_map_; 
            
            void InitMimeTypeMap();
            std::string GetMimeType(const std::string& file_path);
            bool ReadFile(const std::string& file_path, std::string& content);
        };

    } 
} 
```

- StaticFileHandler(const std::string& root_dir, const std::string& path_prefix);
    - Constructor for static handler that receives directory for reading files and path url for static file requests

- std::unique_ptr<reply> handle_request(const request& request) override;
    - Inherited method from RequestHandler to perform operation 

- void InitMimeTypeMap();
    - Initializes the file extension to MIME type table

- std::string GetMimeType(const std::string& file_path);
    - Finds MIME type based on file extension

- bool ReadFile(const std::string& file_path, std::string& content)
    - Finds and reads a file in a directory based on path: root_dir + request path 


## 3. Update CMakeLists.txt with header files
There are **4 modifications** that need to be made in CMakeLists.txt.

Find the comment ```# Define libraries for server submodules``` and make 2 changes:
1. Create handler library using ```add_library()```. 
2. Link the handler library to the server's session library by adding ```[HANDLER_LIB]``` as an argument to ```target_link_libraries()```.

Find the comment ```# Define main server executable and link necessary libraries``` and make 2 changes:
3. Add handler implementation to the server executable by adding ```src/[HANDLER_NAME].cc``` as an argument to ```add_executable()```. 
4. Link the handler library to the server by adding ```[HANDLER_LIB]``` as an argument to ```target_link_libraries()```.

### CMakeLists.txt with handler libraries Example:
See TODOs to see the **4 modifications** that need to be made.
```
# Define libraries for server submodules
add_library(config_parser_lib src/config_parser.cc)
add_library(reply_lib src/reply.cc)
...
...
add_library([HANDLER_LIB] src/[HANDLER_NAME].cc) # TODO: create handler library
target_link_libraries(session_lib reply_lib ... [HANDLER_LIB]) # TODO: add handler library to session

# Define main server executable and link necessary libraries
 add_executable(server
    src/server_main.cc
    src/server.cc
    ...
    ...
    src/[HANDLER_NAME].cc # TODO: add handler file to server
)
target_link_libraries(server Boost::system config_parser_lib)
target_link_libraries(server Boost::system reply_lib)
...
...
target_link_libraries(server Boost::system [HANDLER_LIB]) # TODO: add handler library to server
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/include)
```

## 4. Writing handler unit tests
In the directory ```./marko/tests```, create a new file called ```[HANDLER_NAME]_test.cc``` and write the handler unit tests in the new file. 

Use ```void SetUp()``` and ```void TearDown()``` to create request objects and delete reply ptrs.

### Unit Tests File Format
The handler unit tests file should use the following format.
```
#include "gtest/gtest.h"
#include "request.hpp"
#include "reply.hpp"
#include "../src/[HANDLER_NAME].h"

// Test Fixture class
class [HANDLER_TEXT_FIXTURE] : public testing::Test {
  protected:
    void SetUp() override {
        // create resources

        // create request object
        req.method = "[METHOD]";      
        req.uri = "[URL]"; 
        req.http_version_major = 1;
        req.http_version_minor = 1;
        req.headers = {};
    }
    void TearDown() override {
        // delete resources
    }

    request req;
};

// Test #1: some test
TEST_F([HANDLER_TEXT_FIXTURE], [TEST_NAME]) {
    // test a function
}
```

## 5. Update CMakeLists.txt with unit tests
After writing unit tests, build the unit tests by creating a handler test executable, importing the necessary libraries, and generating a list of tests. 

There are **3 modifications** that need to be made in CMakeLists.txt. Find the comment ```# Add EchoHandler unit test``` in CMakeLists.txt and add these 3 lines:
```
add_executable([HANDLER_TEST] tests/[HANDLER_NAME]_test.cc)
target_link_libraries([HANDLER_TEST] [HANDLER_LIB] reply_lib request_parser_lib gtest_main)
gtest_discover_tests(static_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
```

### CMakeLists.txt with unit tests Example:
See TODOs to see the **3 modifications** that need to be made.
```
# Add EchoHandler unit test
add_executable(echo_handler_test tests/echo_handler_test.cc)
target_link_libraries(echo_handler_test echo_handler_lib reply_lib request_parser_lib gtest_main)
gtest_discover_tests(echo_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
...
...
add_executable([HANDLER_TEST] tests/[HANDLER_NAME]_test.cc) # TODO: create handler tests executable
target_link_libraries([HANDLER_TEST] [HANDLER_LIB] reply_lib request_parser_lib gtest_main) # TODO: add libraries for handler test
gtest_discover_tests(static_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests) # TODO: build to a google test
```

## 6. Add handler to code coverage report
Once handler unit tests are created and added, you can generate a code coverage report including the new handler implementation. 

There are **2 modifications** that need to be made in CMakeLists.txt. Find the comment ```# Enable code coverage report generation``` and make 2 changes inside ```generate_coverage_report()```:
1. After parameter ```TARGETS```, add ```[HANDLER_LIB]```
2. After parameter ```TESTS```, add ```[HANDLER_TEST]```

### CMakeLists.txt with code coverage Example:
See TODOs to see the **2 modifications** that need to be made.
```
# Enable code coverage report generation
include(cmake/CodeCoverageReportConfig.cmake)
generate_coverage_report(
  TARGETS
    server
    config_parser_lib
    ...
    ...
    [HANDLER_LIB]   # TODO: add handler library here
  TESTS
    server_config_test
    server_request_parser_test
    ...
    ...
    [HANDLER_TEST]  # TODO: add handler tests here
)
```

### Run code coverage
To create a code coverage report, perform an out-of-source build by running the following 4 commands in the root directory ```/marko```.
```
mkdir build_coverage
cd build_coverage
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make coverage
```

After running the above commands, in the directory ```./marko/build_coverage/report```, a file called ```index.html``` will be created. Opening ```index.html``` using a browser allows you to view the code coverage report of the handler's unit tests.