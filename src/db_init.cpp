//db_init.cpp - Database initialization utility
#include "database_manager.h"
#include <iostream>
#include <filesystem>
#include <string>

class DatabaseInitializer {
public:
    static bool initializeDatabase(const std::string& db_path, bool reset = false) {
        // Create directory if it doesn't exist
        std::filesystem::path path(db_path);
        std::filesystem::path dir = path.parent_path();
        
        if (!dir.empty() && !std::filesystem::exists(dir)) {
            try {
                std::filesystem::create_directories(dir);
                std::cout << "Created directory: " << dir << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to create directory: " << e.what() << std::endl;
                return false;
            }
        }

        // Remove existing database if reset is requested
        if (reset && std::filesystem::exists(db_path)) {
            try {
                std::filesystem::remove(db_path);
                std::cout << "Removed existing database: " << db_path << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to remove existing database: " << e.what() << std::endl;
                return false;
            }
        }

        // Initialize database manager
        DatabaseManager db_manager(db_path);
        
        std::cout << "Database initialized successfully at: " << db_path << std::endl;
        return true;
    }

    static bool seedTestData(const std::string& db_path) {
        DatabaseManager db_manager(db_path);
        
        // Create test users
        int user1_id = db_manager.createOrUpdateUser("john.doe@example.com", "John Doe");
        int user2_id = db_manager.createOrUpdateUser("jane.smith@example.com", "Jane Smith");
        
        if (user1_id == -1 || user2_id == -1) {
            std::cerr << "Failed to create test users" << std::endl;
            return false;
        }

        // Create test notes
        Note note1;
        note1.user_id = user1_id;
        note1.filename = "cs101_lecture1.pdf";
        note1.original_filename = "CS101 - Introduction to Programming.pdf";
        note1.file_path = "/uploads/cs101_lecture1.pdf";
        note1.file_type = "pdf";
        note1.course_code = "CS101";
        note1.title = "Introduction to Programming - Lecture 1";

        Note note2;
        note2.user_id = user1_id;
        note2.filename = "math201_notes.docx";
        note2.original_filename = "Calculus II Notes.docx";
        note2.file_path = "/uploads/math201_notes.docx";
        note2.file_type = "docx";
        note2.course_code = "MATH201";
        note2.title = "Calculus II - Chapter 1 Notes";

        Note note3;
        note3.user_id = user2_id;
        note3.filename = "cs101_assignment1.pdf";
        note3.original_filename = "Assignment 1 - Variables and Functions.pdf";
        note3.file_path = "/uploads/cs101_assignment1.pdf";
        note3.file_type = "pdf";
        note3.course_code = "CS101";
        note3.title = "Assignment 1 - Variables and Functions";

        int note1_id = db_manager.createNote(note1);
        int note2_id = db_manager.createNote(note2);
        int note3_id = db_manager.createNote(note3);

        if (note1_id == -1 || note2_id == -1 || note3_id == -1) {
            std::cerr << "Failed to create test notes" << std::endl;
            return false;
        }

        std::cout << "Test data seeded successfully:" << std::endl;
        std::cout << "- Created " << 2 << " test users" << std::endl;
        std::cout << "- Created " << 3 << " test notes" << std::endl;
        
        return true;
    }
};

// Main function for standalone database initialization
int main(int argc, char* argv[]) {
    std::string db_path = "data/notes_app.db";  // Default path
    bool reset = false;
    bool seed = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--reset" || arg == "-r") {
            reset = true;
        } else if (arg == "--seed" || arg == "-s") {
            seed = true;
        } else if (arg == "--path" || arg == "-p") {
            if (i + 1 < argc) {
                db_path = argv[++i];
            } else {
                std::cerr << "Error: --path requires a value" << std::endl;
                return 1;
            }
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -p, --path <path>    Database file path (default: data/notes_app.db)" << std::endl;
            std::cout << "  -r, --reset          Reset/recreate the database" << std::endl;
            std::cout << "  -s, --seed           Seed the database with test data" << std::endl;
            std::cout << "  -h, --help           Show this help message" << std::endl;
            return 0;
        }
    }

    std::cout << "Initializing database at: " << db_path << std::endl;
    
    if (!DatabaseInitializer::initializeDatabase(db_path, reset)) {
        std::cerr << "Database initialization failed!" << std::endl;
        return 1;
    }

    if (seed) {
        std::cout << "Seeding test data..." << std::endl;
        if (!DatabaseInitializer::seedTestData(db_path)) {
            std::cerr << "Failed to seed test data!" << std::endl;
            return 1;
        }
    }

    std::cout << "Database setup completed successfully!" << std::endl;
    return 0;
}