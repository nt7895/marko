#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <fstream>     // for std::ofstream
#include <algorithm>   // for std::sort

#include "../src/entity_processor.h"

using http::server::EntityProcessor;
namespace fs = boost::filesystem;

/* ------------------------------------------------------------------ */
/*  One shared fixture creates a throw-away data directory for each run */
class EntityProcessorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    root_ = (fs::temp_directory_path() / fs::unique_path("crud_%%%%")).string();
    fs::create_directory(root_);
    ep_ = std::make_unique<EntityProcessor>(root_);
  }
  void TearDown() override { fs::remove_all(root_); }

  std::string root_;
  std::unique_ptr<EntityProcessor> ep_;
};

TEST_F(EntityProcessorTest, CreateRetrieveUpdateDeleteCycle) {
  const std::string init = R"({"k":1})";
  std::string id = ep_->CreateEntity("Shoes", init);
  ASSERT_EQ(id, "1");

  std::string out;
  ASSERT_TRUE(ep_->RetrieveEntity("Shoes", id, out));
  EXPECT_EQ(out, init);

  /* update */
  const std::string updated = R"({"k":2})";
  ASSERT_TRUE(ep_->UpdateEntity("Shoes", id, updated));
  ASSERT_TRUE(ep_->RetrieveEntity("Shoes", id, out));
  EXPECT_EQ(out, updated);

  /* delete */
  ASSERT_TRUE(ep_->DeleteEntity("Shoes", id));
  EXPECT_FALSE(ep_->RetrieveEntity("Shoes", id, out));
}

TEST_F(EntityProcessorTest, SequentialIds) {
  EXPECT_EQ(ep_->CreateEntity("Books", "{}"), "1");
  EXPECT_EQ(ep_->CreateEntity("Books", "{}"), "2");
}

TEST_F(EntityProcessorTest, ListReturnsAllIds) {
  ep_->CreateEntity("Games", "{}");
  ep_->CreateEntity("Games", "{}");

  std::vector<std::string> ids;
  ASSERT_TRUE(ep_->ListEntities("Games", ids));
  std::sort(ids.begin(), ids.end());
  EXPECT_EQ(ids, (std::vector<std::string>{"1","2"}));
}

TEST_F(EntityProcessorTest, UpdateFailIfIdMissing) {
  EXPECT_FALSE(ep_->UpdateEntity("Shoes", "999", "{}"));
}

TEST_F(EntityProcessorTest, DeleteFailIfIdMissing) {
  EXPECT_FALSE(ep_->DeleteEntity("Shoes", "42"));
}

TEST_F(EntityProcessorTest, ListFailIfEntityDirMissing) {
  std::vector<std::string> ids;
  EXPECT_FALSE(ep_->ListEntities("Nonexistent", ids));
}


/* Non-numeric filenames in the entity directory must be ignored
 *     when selecting the next available ID.                            */
TEST_F(EntityProcessorTest, NonNumericFilenamesAreIgnored) {
  const std::string ent = "Weird";
  fs::create_directories(root_ + "/" + ent);
  std::ofstream(root_ + "/" + ent + "/abc").close();   // nuisance file

  std::string id = ep_->CreateEntity(ent, "{}");
  EXPECT_EQ(id, "1");                                  // should still start at 1
}

/*   ListEntities succeeds on an empty (but existing) directory
 *     and returns an empty vector.                                     */
TEST_F(EntityProcessorTest, ListOnEmptyDirectory) {
  const std::string ent = "Empty";
  fs::create_directories(root_ + "/" + ent);

  std::vector<std::string> ids;
  ASSERT_TRUE(ep_->ListEntities(ent, ids));
  EXPECT_TRUE(ids.empty());
}

/*  If a large ID already exists on disk, the next ID is largest+1. */
TEST_F(EntityProcessorTest, NextIdAfterManualFileGap) {
  const std::string ent = "Shoes";
  ASSERT_EQ(ep_->CreateEntity(ent, "{}"), "1");
  ASSERT_EQ(ep_->CreateEntity(ent, "{}"), "2");

  /* inject an out-of-band file “10” */
  std::ofstream(root_ + "/" + ent + "/10").close();

  std::string next = ep_->CreateEntity(ent, "{}");
  EXPECT_EQ(next, "11");
}

/*  Creation fails (returns "") if a regular file blocks the
 *     directory name intended for the entity type.                     */
TEST_F(EntityProcessorTest, CreateFailsWhenPathIsAFile) {
  const std::string ent = "Blocked";
  std::ofstream(root_ + "/" + ent).close();           // path already a file

  std::string id = ep_->CreateEntity(ent, "{}");
  EXPECT_EQ(id, "");                                  // failure indicated
}

/*  Retrieving from a missing entity directory returns false.       */
TEST_F(EntityProcessorTest, RetrieveNonexistentEntityType) {
  std::string out;
  EXPECT_FALSE(ep_->RetrieveEntity("Ghost", "1", out));
}
