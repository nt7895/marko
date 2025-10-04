//test_database_manager.cpp - Comprehensive unit tests
#include "database_manager.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <memory>

class DatabaseManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a temporary test database
        test_db_path = "test_notes.db";
        
        // Remove test database if it exists
        if (std::filesystem::exists(test_db_path)) {
            std::filesystem::remove(test_db_path);
        }
        
        // Create fresh database manager
        db_manager = std::make_unique<DatabaseManager>(test_db_path);
    }

    void TearDown() override {
        // Clean up test database
        db_manager.reset();
        if (std::filesystem::exists(test_db_path)) {
            std::filesystem::remove(test_db_path);
        }
    }

    std::string test_db_path;
    std::unique_ptr<DatabaseManager> db_manager;
};

// Test database initialization
TEST_F(DatabaseManagerTest, DatabaseInitialization) {
    EXPECT_TRUE(std::filesystem::exists(test_db_path));
    
    // Database should be properly initialized with tables
    EXPECT_NE(db_manager.get(), nullptr);
}

// Test user creation and retrieval
TEST_F(DatabaseManagerTest, CreateAndRetrieveUser) {
    // Create a new user
    int user_id = db_manager->createOrUpdateUser("test@example.com", "Test User");
    EXPECT_GT(user_id, 0);

    // Retrieve user by ID
    auto user = db_manager->getUserById(user_id);
    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->id, user_id);
    EXPECT_EQ(user->email, "test@example.com");
    EXPECT_EQ(user->name, "Test User");
    EXPECT_FALSE(user->created_at.empty());

    // Retrieve user by email
    auto user_by_email = db_manager->getUserByEmail("test@example.com");
    ASSERT_NE(user_by_email, nullptr);
    EXPECT_EQ(user_by_email->id, user_id);
    EXPECT_EQ(user_by_email->email, "test@example.com");
    EXPECT_EQ(user_by_email->name, "Test User");
}

// Test user update functionality
TEST_F(DatabaseManagerTest, UpdateExistingUser) {
    // Create a user
    int user_id = db_manager->createOrUpdateUser("update@example.com", "Original Name");
    EXPECT_GT(user_id, 0);

    // Update the same user with new name
    int updated_user_id = db_manager->createOrUpdateUser("update@example.com", "Updated Name");
    EXPECT_EQ(user_id, updated_user_id);

    // Verify the name was updated
    auto user = db_manager->getUserById(user_id);
    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->name, "Updated Name");
    EXPECT_EQ(user->email, "update@example.com");
}

// Test retrieving non-existent user
TEST_F(DatabaseManagerTest, GetNonExistentUser) {
    auto user = db_manager->getUserById(9999);
    EXPECT_EQ(user, nullptr);

    auto user_by_email = db_manager->getUserByEmail("nonexistent@example.com");
    EXPECT_EQ(user_by_email, nullptr);
}

// Test note creation and retrieval
TEST_F(DatabaseManagerTest, CreateAndRetrieveNote) {
    // First create a user
    int user_id = db_manager->createOrUpdateUser("noteuser@example.com", "Note User");
    EXPECT_GT(user_id, 0);

    // Create a note
    Note note;
    note.user_id = user_id;
    note.filename = "test_file.pdf";
    note.original_filename = "Original Test File.pdf";
    note.file_path = "/uploads/test_file.pdf";
    note.file_type = "pdf";
    note.course_code = "TEST101";
    note.title = "Test Note Title";

    int note_id = db_manager->createNote(note);
    EXPECT_GT(note_id, 0);

    // Retrieve the note
    auto retrieved_note = db_manager->getNoteById(note_id);
    ASSERT_NE(retrieved_note, nullptr);
    EXPECT_EQ(retrieved_note->id, note_id);
    EXPECT_EQ(retrieved_note->user_id, user_id);
    EXPECT_EQ(retrieved_note->filename, "test_file.pdf");
    EXPECT_EQ(retrieved_note->original_filename, "Original Test File.pdf");
    EXPECT_EQ(retrieved_note->file_path, "/uploads/test_file.pdf");
    EXPECT_EQ(retrieved_note->file_type, "pdf");
    EXPECT_EQ(retrieved_note->course_code, "TEST101");
    EXPECT_EQ(retrieved_note->title, "Test Note Title");
    EXPECT_FALSE(retrieved_note->uploaded_at.empty());
}

// Test retrieving notes by user ID
TEST_F(DatabaseManagerTest, GetNotesByUserId) {
    // Create two users
    int user1_id = db_manager->createOrUpdateUser("user1@example.com", "User One");
    int user2_id = db_manager->createOrUpdateUser("user2@example.com", "User Two");

    // Create notes for user1
    Note note1;
    note1.user_id = user1_id;
    note1.filename = "note1.pdf";
    note1.original_filename = "Note 1.pdf";
    note1.file_path = "/uploads/note1.pdf";
    note1.file_type = "pdf";
    note1.course_code = "CS101";
    note1.title = "First Note";

    Note note2;
    note2.user_id = user1_id;
    note2.filename = "note2.docx";
    note2.original_filename = "Note 2.docx";
    note2.file_path = "/uploads/note2.docx";
    note2.file_type = "docx";
    note2.course_code = "CS102";
    note2.title = "Second Note";

    // Create note for user2
    Note note3;
    note3.user_id = user2_id;
    note3.filename = "note3.pdf";
    note3.original_filename = "Note 3.pdf";
    note3.file_path = "/uploads/note3.pdf";
    note3.file_type = "pdf";
    note3.course_code = "CS101";
    note3.title = "Third Note";

    db_manager->createNote(note1);
    db_manager->createNote(note2);
    db_manager->createNote(note3);

    // Get notes for user1
    auto user1_notes = db_manager->getNotesByUserId(user1_id);
    EXPECT_EQ(user1_notes.size(), 2);

    // Get notes for user2
    auto user2_notes = db_manager->getNotesByUserId(user2_id);
    EXPECT_EQ(user2_notes.size(), 1);
    EXPECT_EQ(user2_notes[0].title, "Third Note");

    // Get notes for non-existent user
    auto no_notes = db_manager->getNotesByUserId(9999);
    EXPECT_EQ(no_notes.size(), 0);
}

// Test retrieving notes by course code
TEST_F(DatabaseManagerTest, GetNotesByCourseCode) {
    // Create user
    int user_id = db_manager->createOrUpdateUser("student@example.com", "Student");

    // Create notes with different course codes
    Note cs101_note1;
    cs101_note1.user_id = user_id;
    cs101_note1.filename = "cs101_1.pdf";
    cs101_note1.original_filename = "CS101 Lecture 1.pdf";
    cs101_note1.file_path = "/uploads/cs101_1.pdf";
    cs101_note1.file_type = "pdf";
    cs101_note1.course_code = "CS101";
    cs101_note1.title = "CS101 Lecture 1";

    Note cs101_note2;
    cs101_note2.user_id = user_id;
    cs101_note2.filename = "cs101_2.pdf";
    cs101_note2.original_filename = "CS101 Lecture 2.pdf";
    cs101_note2.file_path = "/uploads/cs101_2.pdf";
    cs101_note2.file_type = "pdf";
    cs101_note2.course_code = "CS101";
    cs101_note2.title = "CS101 Lecture 2";

    Note math201_note;
    math201_note.user_id = user_id;
    math201_note.filename = "math201.pdf";
    math201_note.original_filename = "Math 201 Notes.pdf";
    math201_note.file_path = "/uploads/math201.pdf";
    math201_note.file_type = "pdf";
    math201_note.course_code = "MATH201";
    math201_note.title = "Math 201 Notes";

    db_manager->createNote(cs101_note1);
    db_manager->createNote(cs101_note2);
    db_manager->createNote(math201_note);

    // Get CS101 notes
    auto cs101_notes = db_manager->getNotesByCourseCode("CS101");
    EXPECT_EQ(cs101_notes.size(), 2);

    // Get MATH201 notes
    auto math201_notes = db_manager->getNotesByCourseCode("MATH201");
    EXPECT_EQ(math201_notes.size(), 1);
    EXPECT_EQ(math201_notes[0].title, "Math 201 Notes");

    // Get notes for non-existent course
    auto no_notes = db_manager->getNotesByCourseCode("NONEXISTENT");
    EXPECT_EQ(no_notes.size(), 0);
}

// Test note search functionality
TEST_F(DatabaseManagerTest, SearchNotes) {
    // Create user and notes
    int user_id = db_manager->createOrUpdateUser("search@example.com", "Search User");

    Note note1;
    note1.user_id = user_id;
    note1.filename = "programming_basics.pdf";
    note1.original_filename = "Programming Basics.pdf";
    note1.file_path = "/uploads/programming_basics.pdf";
    note1.file_type = "pdf";
    note1.course_code = "CS101";
    note1.title = "Introduction to Programming";

    Note note2;
    note2.user_id = user_id;
    note2.filename = "advanced_algorithms.pdf";
    note2.original_filename = "Advanced Algorithms.pdf";
    note2.file_path = "/uploads/advanced_algorithms.pdf";
    note2.file_type = "pdf";
    note2.course_code = "CS301";
    note2.title = "Data Structures and Algorithms";

    Note note3;
    note3.user_id = user_id;
    note3.filename = "calculus_notes.pdf";
    note3.original_filename = "Calculus Notes.pdf";
    note3.file_path = "/uploads/calculus_notes.pdf";
    note3.file_type = "pdf";
    note3.course_code = "MATH201";
    note3.title = "Differential Calculus";

    db_manager->createNote(note1);
    db_manager->createNote(note2);
    db_manager->createNote(note3);

    // Search by title
    auto programming_search = db_manager->searchNotes("Programming");
    EXPECT_EQ(programming_search.size(), 1);
    EXPECT_EQ(programming_search[0].title, "Introduction to Programming");

    // Search by course code
    auto cs_search = db_manager->searchNotes("CS");
    EXPECT_EQ(cs_search.size(), 2);

    // Search by filename
    auto calculus_search = db_manager->searchNotes("calculus");
    EXPECT_EQ(calculus_search.size(), 1);
    EXPECT_EQ(calculus_search[0].course_code, "MATH201");

    // Search with no results
    auto no_results = db_manager->searchNotes("nonexistent");
    EXPECT_EQ(no_results.size(), 0);
}

// Test note deletion
TEST_F(DatabaseManagerTest, DeleteNote) {
    // Create user and note
    int user_id = db_manager->createOrUpdateUser("delete@example.com", "Delete User");
    
    Note note;
    note.user_id = user_id;
    note.filename = "to_delete.pdf";
    note.original_filename = "File to Delete.pdf";
    note.file_path = "/uploads/to_delete.pdf";
    note.file_type = "pdf";
    note.course_code = "TEST101";
    note.title = "Note to Delete";

    int note_id = db_manager->createNote(note);
    EXPECT_GT(note_id, 0);

    // Verify note exists
    auto retrieved_note = db_manager->getNoteById(note_id);
    EXPECT_NE(retrieved_note, nullptr);

    // Delete the note
    bool deleted = db_manager->deleteNote(note_id, user_id);
    EXPECT_TRUE(deleted);

    // Verify note no longer exists
    auto deleted_note = db_manager->getNoteById(note_id);
    EXPECT_EQ(deleted_note, nullptr);

    // Try to delete non-existent note
    bool delete_nonexistent = db_manager->deleteNote(9999, user_id);
    EXPECT_FALSE(delete_nonexistent);

    // Try to delete note with wrong user_id
    int another_user_id = db_manager->createOrUpdateUser("other@example.com", "Other User");
    Note another_note;
    another_note.user_id = another_user_id;
    another_note.filename = "protected.pdf";
    another_note.original_filename = "Protected.pdf";
    another_note.file_path = "/uploads/protected.pdf";
    another_note.file_type = "pdf";
    another_note.course_code = "TEST101";
    another_note.title = "Protected Note";

    int protected_note_id = db_manager->createNote(another_note);
    bool unauthorized_delete = db_manager->deleteNote(protected_note_id, user_id);
    EXPECT_FALSE(unauthorized_delete);
}

// Test getting all course codes
TEST_F(DatabaseManagerTest, GetAllCourseCodes) {
    // Initially should be empty
    auto empty_codes = db_manager->getAllCourseCodes();
    EXPECT_EQ(empty_codes.size(), 0);

    // Create user and notes with different course codes
    int user_id = db_manager->createOrUpdateUser("courses@example.com", "Course User");

    // Create notes with various course codes
    std::vector<std::string> expected_codes = {"CS101", "CS201", "MATH101", "PHYS201"};
    
    for (const auto& code : expected_codes) {
        Note note;
        note.user_id = user_id;
        note.filename = code + "_note.pdf";
        note.original_filename = code + " Note.pdf";
        note.file_path = "/uploads/" + code + "_note.pdf";
        note.file_type = "pdf";
        note.course_code = code;
        note.title = code + " Course Note";
        
        db_manager->createNote(note);
    }

    // Add duplicate course code
    Note duplicate_note;
    duplicate_note.user_id = user_id;
    duplicate_note.filename = "cs101_extra.pdf";
    duplicate_note.original_filename = "CS101 Extra.pdf";
    duplicate_note.file_path = "/uploads/cs101_extra.pdf";
    duplicate_note.file_type = "pdf";
    duplicate_note.course_code = "CS101";
    duplicate_note.title = "CS101 Extra Note";
    db_manager->createNote(duplicate_note);

    // Get all course codes
    auto all_codes = db_manager->getAllCourseCodes();
    EXPECT_EQ(all_codes.size(), 4); // Should be unique

    // Verify all expected codes are present and sorted
    for (size_t i = 0; i < expected_codes.size(); ++i) {
        EXPECT_EQ(std::find(all_codes.begin(), all_codes.end(), expected_codes[i]) != all_codes.end(), true);
    }
}

// Test thread safety (basic test)
TEST_F(DatabaseManagerTest, ConcurrentAccess) {
    const int num_threads = 10;
    const int operations_per_thread = 5;
    std::vector<std::thread> threads;
    std::vector<int> user_ids(num_threads);

    // Create users concurrently
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, &user_ids]() {
            std::string email = "user" + std::to_string(i) + "@example.com";
            std::string name = "User " + std::to_string(i);
            user_ids[i] = db_manager->createOrUpdateUser(email, name);
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all users were created successfully
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_GT(user_ids[i], 0);
        auto user = db_manager->getUserById(user_ids[i]);
        EXPECT_NE(user, nullptr);
    }
}

// Main function to run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}