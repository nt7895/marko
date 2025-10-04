#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <string>
#include <memory>
#include <vector>
#include <sqlite3.h>
#include <mutex>

struct User {
    int id;
    std::string email;
    std::string name;
    std::string created_at;
};

struct Note {
    int id;
    int user_id;
    std::string filename;
    std::string original_filename;
    std::string file_path;
    std::string file_type;
    std::string course_code;
    std::string title;
    std::string uploaded_at;
};

class DatabaseManager {
    public:
        DatabaseManager(const std::string& db_path);
        ~DatabaseManager();

        // initialize the database and create necessary tables
        bool initialize();

        // User operations
        int createOrUpdateUser(const std::string& email, const std::string& name);
        std::shared_ptr<User> getUserById(int user_id);
        std::shared_ptr<User> getUserByEmail(const std::string& email);

        // Note operations
        int createNote(const Note& note);
        std::unique_ptr<Note> getNoteById(int note_id);
        std::vector<Note> getNotesByUserId(int user_id);
        std::vector<Note> getNotesByCourseCode(const std::string& course_code);
        std::vector<Note> searchNotes(const std::string& query);
        bool deleteNote(int id, int user_id);

        // Get all course codes
        std::vector<std::string> getAllCourseCodes();

    private:
        sqlite3* db;
        std::string db_path;
        std::mutex db_mutex;

        // Helper functions
        bool executeQuery(const std::string& query);
        sqlite3_stmt* prepareStatement(const std::string& query);
        void logError(const std::string& message);

        static const char* CREATE_USERS_TABLE;
        static const char* CREATE_NOTES_TABLE;
        static const char* CREATE_INDEXS;
};

#endif // DATABASE_MANAGER_H