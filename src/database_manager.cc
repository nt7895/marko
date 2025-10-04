#include "database_manager.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>

const char* DatabaseManager::CREATE_USERS_TABLE = R"(
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        email TEXT UNIQUE NOT NULL,
        name TEXT NOT NULL,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP
    );
)";

const char* DatabaseManager::CREATE_NOTES_TABLE = R"(
    CREATE TABLE IF NOT EXISTS notes (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id INTEGER REFERENCES users(id),
        filename TEXT NOT NULL,
        original_filename TEXT NOT NULL,
        file_path TEXT NOT NULL,
        file_type TEXT NOT NULL,
        course_code TEXT NOT NULL,
        title TEXT NOT NULL,
        uploaded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
    );
)";

const char* DatabaseManager::CREATE_INDEXS = R"(
    CREATE INDEX IF NOT EXISTS idx_notes_user_id ON notes(user_id);
    CREATE INDEX IF NOT EXISTS idx_notes_course_code ON notes(course_code);
    CREATE INDEX IF NOT EXISTS idx_notes_title ON notes(title);
)";

DatabaseManager::DatabaseManager(const std::string& db_path) : db_path(db_path), db(nullptr) {
    std::cout << "[DEBUG] Constructor called with path: " << db_path << std::endl;
    std::cout << "[DEBUG] Calling initialize()..." << std::endl;
    if (!initialize()) {
        std::cerr << "Failed to initialize database." << std::endl;
    }
    // int rc = sqlite3_open(db_path.c_str(), &db);
    // if (rc != SQLITE_OK) {
    //     std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
    //     sqlite3_close(db);
    //     db = nullptr;
    // }
    // executeQuery("PRAGMA foreign_keys = ON;");
}

DatabaseManager::~DatabaseManager() {
    if (db) {
        sqlite3_close(db);
    }
}

bool DatabaseManager::initialize() {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (!db) {
        int rc = sqlite3_open(db_path.c_str(), &db);
        if (rc != SQLITE_OK) {
            logError("Can't open database: " + std::string(sqlite3_errmsg(db)));
            return false;
        }
    }

    // Enable foreign keys FIRST
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        logError("Failed to enable foreign keys: " + std::string(errMsg));
        sqlite3_free(errMsg);
        return false;
    }
    if (!executeQuery(CREATE_USERS_TABLE) || !executeQuery(CREATE_NOTES_TABLE) || !executeQuery(CREATE_INDEXS)) {
        logError("Failed to create tables or indexes.");
        return false;
    }
    return true;
}

bool DatabaseManager::executeQuery(const std::string& query) {
    // std::lock_guard<std::mutex> lock(db_mutex);
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        logError("SQL error: " + std::string(errMsg));
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

void DatabaseManager::logError(const std::string& message) {
    std::cerr << "Database error: " << message << std::endl << sqlite3_errmsg(db) << std::endl;
}

sqlite3_stmt* DatabaseManager::prepareStatement(const std::string& query) {
    // std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        logError("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
        return nullptr;
    }
    return stmt;
}

int DatabaseManager::createOrUpdateUser(const std::string& email, const std::string& name) {
    std::lock_guard<std::mutex> lock(db_mutex);

    // check if user exists
    const char* checkUserQuery = "SELECT id FROM users WHERE email = ?;";
    sqlite3_stmt* checkStmt = prepareStatement(checkUserQuery);
    if (!checkStmt) return -1;
    
    sqlite3_bind_text(checkStmt, 1, email.c_str(), -1, SQLITE_STATIC);
    int userId = -1;
    if (sqlite3_step(checkStmt) == SQLITE_ROW) {
        userId = sqlite3_column_int(checkStmt, 0);
        sqlite3_finalize(checkStmt);
        // User exists, update their name
        const char* updateUserQuery = "UPDATE users SET name = ? WHERE id = ?;";
        sqlite3_stmt* updateStmt = prepareStatement(updateUserQuery);
        if (!updateStmt) return -1;

        sqlite3_bind_text(updateStmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(updateStmt, 2, userId);
        sqlite3_step(updateStmt);
        sqlite3_finalize(updateStmt);
    }
    else {
        sqlite3_finalize(checkStmt);
        // User does not exist, create a new user
        const char* insertUserQuery = "INSERT INTO users (email, name) VALUES (?, ?);";
        sqlite3_stmt* insertStmt = prepareStatement(insertUserQuery);
        if (!insertStmt) return -1;

        sqlite3_bind_text(insertStmt, 1, email.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insertStmt, 2, name.c_str(), -1, SQLITE_STATIC);
        
        if (sqlite3_step(insertStmt) == SQLITE_DONE) {
            userId = sqlite3_last_insert_rowid(db);
        } else {
            logError("Failed to insert user: " + std::string(sqlite3_errmsg(db)));
        }
        sqlite3_finalize(insertStmt);
    }
    return userId;
}

std::shared_ptr<User> DatabaseManager::getUserById(int user_id) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const char* query = "SELECT id, email, name, created_at FROM users WHERE id = ?;";
    sqlite3_stmt* stmt = prepareStatement(query);
    if (!stmt) return nullptr;

    sqlite3_bind_int(stmt, 1, user_id);
    std::shared_ptr<User> user = nullptr;
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user = std::make_shared<User>();
        user->id = sqlite3_column_int(stmt, 0);
        user->email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user->name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user->created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    }
    
    sqlite3_finalize(stmt);
    return user;
}

std::shared_ptr<User> DatabaseManager::getUserByEmail(const std::string& email) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const char* query = "SELECT id, email, name, created_at FROM users WHERE email = ?;";
    sqlite3_stmt* stmt = prepareStatement(query);
    if (!stmt) return nullptr;

    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_STATIC);
    std::shared_ptr<User> user = nullptr;
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user = std::make_shared<User>();
        user->id = sqlite3_column_int(stmt, 0);
        user->email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user->name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user->created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    }
    
    sqlite3_finalize(stmt);
    return user;
}

int DatabaseManager::createNote(const Note& note) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const char* query = R"(
            INSERT INTO notes (user_id, filename, original_filename, file_path, file_type, course_code, title) 
            VALUES (?, ?, ?, ?, ?, ?, ?);
        )";
    sqlite3_stmt* stmt = prepareStatement(query);
    if (!stmt) return -1;

    sqlite3_bind_int(stmt, 1, note.user_id);
    sqlite3_bind_text(stmt, 2, note.filename.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, note.original_filename.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, note.file_path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, note.file_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, note.course_code.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, note.title.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        int note_id = sqlite3_last_insert_rowid(db);
        sqlite3_finalize(stmt);
        return note_id;
    } else {
        logError("Failed to create note: " + std::string(sqlite3_errmsg(db)));
        sqlite3_finalize(stmt);
        return -1;
    }
}

std::unique_ptr<Note> DatabaseManager::getNoteById(int note_id) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const char* query = R"(
        SELECT id, user_id, filename, original_filename, file_path, file_type, course_code, title, uploaded_at 
        FROM notes WHERE id = ?;
    )";
    sqlite3_stmt* stmt = prepareStatement(query);
    if (!stmt) return nullptr;

    sqlite3_bind_int(stmt, 1, note_id);
    std::unique_ptr<Note> note = nullptr;
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        note = std::make_unique<Note>();
        note->id = sqlite3_column_int(stmt, 0);
        note->user_id = sqlite3_column_int(stmt, 1);
        note->filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        note->original_filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        note->file_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        note->file_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        note->course_code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        note->title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        note->uploaded_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
    }
    
    sqlite3_finalize(stmt);
    return note;
}

std::vector<Note> DatabaseManager::getNotesByUserId(int user_id) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const char* query = R"(
        SELECT id, user_id, filename, original_filename, file_path, file_type, course_code, title, uploaded_at 
        FROM notes WHERE user_id = ? ORDER BY uploaded_at DESC;
    )";
    sqlite3_stmt* stmt = prepareStatement(query);
    if (!stmt) return {};

    sqlite3_bind_int(stmt, 1, user_id);
    std::vector<Note> notes;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Note note;
        note.id = sqlite3_column_int(stmt, 0);
        note.user_id = sqlite3_column_int(stmt, 1);
        note.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        note.original_filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        note.file_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        note.file_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        note.course_code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        note.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        note.uploaded_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        notes.push_back(note);
    }

    sqlite3_finalize(stmt);
    return notes;
}

std::vector<Note> DatabaseManager::getNotesByCourseCode(const std::string& course_code) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const char* query = R"(
        SELECT id, user_id, filename, original_filename, file_path, file_type, title, uploaded_at 
        FROM notes WHERE course_code = ? ORDER BY uploaded_at DESC;
    )";
    sqlite3_stmt* stmt = prepareStatement(query);
    if (!stmt) return {};

    sqlite3_bind_text(stmt, 1, course_code.c_str(), -1, SQLITE_STATIC);
    std::vector<Note> notes;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Note note;
        note.id = sqlite3_column_int(stmt, 0);
        note.user_id = sqlite3_column_int(stmt, 1);
        note.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        note.original_filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        note.file_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        note.file_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        note.course_code = course_code; // already bound
        note.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        note.uploaded_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        notes.push_back(note);
    }

    sqlite3_finalize(stmt);
    return notes;
}

std::vector<Note> DatabaseManager::searchNotes(const std::string& query) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::string sql = R"(
        SELECT id, user_id, filename, original_filename, file_path, file_type, course_code, title, uploaded_at 
        FROM notes 
        WHERE title LIKE ? OR course_code LIKE ? OR original_filename LIKE ?
        ORDER BY uploaded_at DESC;
    )";
    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return {};

    std::string searchPattern = "%" + query + "%";
    sqlite3_bind_text(stmt, 1, searchPattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, searchPattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, searchPattern.c_str(), -1, SQLITE_STATIC);

    std::vector<Note> notes;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Note note;
        note.id = sqlite3_column_int(stmt, 0);
        note.user_id = sqlite3_column_int(stmt, 1);
        note.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        note.original_filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        note.file_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        note.file_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        note.course_code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        note.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        note.uploaded_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        notes.push_back(note);
    }

    sqlite3_finalize(stmt);
    return notes;
}

bool DatabaseManager::deleteNote(int id, int user_id) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const char* query = "DELETE FROM notes WHERE id = ? AND user_id = ?;";
    sqlite3_stmt* stmt = prepareStatement(query);
    if (!stmt) return false;

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, user_id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success && sqlite3_changes(db) > 0;
}

std::vector<std::string> DatabaseManager::getAllCourseCodes() {
    std::lock_guard<std::mutex> lock(db_mutex);
    const char* query = "SELECT DISTINCT course_code FROM notes ORDER BY course_code;";
    sqlite3_stmt* stmt = prepareStatement(query);
    if (!stmt) return {};

    std::vector<std::string> course_codes;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* course_code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (course_code) {
            course_codes.push_back(course_code);
        }
    }

    sqlite3_finalize(stmt);
    return course_codes;
}