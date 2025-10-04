#include <iostream>
#include <boost/asio.hpp>
#include "server.h"
#include "config_parser.h"
#include "server_log.h"
#include <signal.h>
#include "request_handler_registry.h" // Add this include
#include <thread>
#include <vector>

// Initialize handlers to ensure they're registered
void init_handlers() {
    std::cout << "Initializing handlers..." << std::endl;
    // This will access the factory map and ensure it's created
    http::server::RequestHandlerRegistry::GetFactoryMap();
    
    // Print available handlers
    std::cout << "Available handlers: ";
    for (const auto& [name, _] : http::server::RequestHandlerRegistry::GetFactoryMap()) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
}

// Close server after Ctrl + C input
void interrupt_handler(const boost::system::error_code& error, int signal_number) {
  server_log log;
  log.log_server_close();
  exit(signal_number);
}

void thread_handler(boost::asio::io_service& io_service, std::exception_ptr& thread_exception_ptr) {
  try {
    io_service.run();
  } catch (std::exception& e) {
    thread_exception_ptr = std::current_exception();
  }
}

int main(int argc, char* argv[])
{
  std::exception_ptr thread_exception_ptr = nullptr;

  // Initialize the handler registry first
  init_handlers();
  
  server_log log;
  log.start_logging("../log_files/server_log_%Y-%m-%d_%N.log");
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: server <config_file>" << std::endl;
      return 1;
    }

    boost::asio::io_service io_service;

    NginxConfigParser config_parser;
    NginxConfig config;
    
    // Parse config file
    bool config_parse_status = config_parser.Parse(argv[1], &config);
    log.log_config_parser_status(config_parse_status);
    
    if (!config_parse_status) {
      std::cerr << "Failed to parse config file" << std::endl;
      return 1;
    }
    
    // Extract port number
    std::string port_num;
    if (!config.ExtractPort(port_num)) {
      std::cerr << "No port specified in config file" << std::endl;
      return 1;
    }
    
    // Extract handler configurations
    auto handler_configs = config.ExtractHandlerConfigs();
    if (handler_configs.empty()) {
      std::cerr << "No valid handlers specified in config file" << std::endl;
      return 1;
    }
    
    // Create and run the server
    server s(io_service, std::stoi(port_num), handler_configs);
    log.log_server_startup(port_num);

    // Run interrupt_handler() when Ctrl + C is entered
    boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
    signals.async_wait(interrupt_handler);

    // Multithreading
    std::vector<std::thread> threads_container;
    int thread_count = 4;

    // Runs request handlers in multiple threads
    for (int i = 0; i < thread_count; i++) {
        threads_container.emplace_back([&]() {
          thread_handler(io_service, thread_exception_ptr);
        });
    }

    // Rethrow exceptions caught during threading 
    if (thread_exception_ptr) {
      try {
        std::rethrow_exception(thread_exception_ptr);
      } catch(const std::exception &e) {
        std::cerr << "Thread Exception: " << e.what() << "\n";
      }
    }

    // Check if all threads have completed their operations
    for (std::thread& t : threads_container)
      t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  log.log_server_close();
  return 0;
}