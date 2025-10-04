#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/api_handler.h"

using http::server::APIHandler;
using http::server::EntityProcessor;
using http::server::request;
using http::server::reply;
using ::testing::_; using ::testing::Return;
using ::testing::DoAll; using ::testing::SetArgReferee;

/* ---------------- mock ---------------- */
class MockEP : public EntityProcessor {
 public:
  MockEP() : EntityProcessor("/tmp") {}
  MOCK_METHOD(std::string, CreateEntity,
              (const std::string&, const std::string&), (override));
  MOCK_METHOD(bool, RetrieveEntity,
              (const std::string&, const std::string&, std::string&), (override));
  MOCK_METHOD(bool, UpdateEntity,
              (const std::string&, const std::string&, const std::string&), (override));
  MOCK_METHOD(bool, DeleteEntity,
              (const std::string&, const std::string&), (override));
  MOCK_METHOD(bool, ListEntities,
              (const std::string&, std::vector<std::string>&), (override));
};

/* ---------------- fixture ------------- */
class APIHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto mock = std::make_unique<MockEP>();
    mock_ = mock.get();
    handler_ = std::make_unique<APIHandler>("/api", std::move(mock));
  }
  MockEP* mock_;
  std::unique_ptr<APIHandler> handler_;
};

/* ----- POST create ----- */
TEST_F(APIHandlerTest, CreateSuccess) {
  EXPECT_CALL(*mock_, CreateEntity("Shoes", R"({"a":1})"))
      .WillOnce(Return("3"));

  request req; req.method="POST"; req.uri="/api/Shoes"; req.body=R"({"a":1})";
  auto rep = handler_->handle_request(req);

  EXPECT_EQ(rep->status, reply::ok);
  EXPECT_EQ(rep->content, R"({"id": 3})");
}

/* POST empty body -> 400 */
TEST_F(APIHandlerTest, CreateBadRequest) {
  request req; req.method="POST"; req.uri="/api/Shoes";
  auto rep = handler_->handle_request(req);
  EXPECT_EQ(rep->status, reply::bad_request);
}

/* ----- GET id ----- */
TEST_F(APIHandlerTest, RetrieveSuccess) {
  const std::string payload = R"({"x":9})";
  EXPECT_CALL(*mock_, RetrieveEntity("Shoes", "1", _))
      .WillOnce(DoAll(SetArgReferee<2>(payload), Return(true)));

  request req; req.method="GET"; req.uri="/api/Shoes/1";
  auto rep = handler_->handle_request(req);

  EXPECT_EQ(rep->status, reply::ok);
  EXPECT_EQ(rep->content, payload);
}

TEST_F(APIHandlerTest, RetrieveNotFound) {
  EXPECT_CALL(*mock_, RetrieveEntity("Shoes", "99", _))
      .WillOnce(Return(false));

  request req; req.method="GET"; req.uri="/api/Shoes/99";
  auto rep = handler_->handle_request(req);
  EXPECT_EQ(rep->status, reply::not_found);
}

/* ----- PUT update ----- */
TEST_F(APIHandlerTest, UpdateSuccess) {
  EXPECT_CALL(*mock_, UpdateEntity("Shoes","2",R"({"x":2})"))
      .WillOnce(Return(true));

  request req; req.method="PUT"; req.uri="/api/Shoes/2"; req.body=R"({"x":2})";
  auto rep = handler_->handle_request(req);

  EXPECT_EQ(rep->status, reply::ok);
  EXPECT_EQ(rep->content, R"({"status": "success"})");
}

TEST_F(APIHandlerTest, UpdateMissingId) {
  EXPECT_CALL(*mock_, UpdateEntity("Shoes","5", _))
      .WillOnce(Return(false));

  request req; req.method="PUT"; req.uri="/api/Shoes/5"; req.body="{}";
  auto rep = handler_->handle_request(req);
  EXPECT_EQ(rep->status, reply::not_found);
}

/* ----- DELETE ----- */
TEST_F(APIHandlerTest, DeleteSuccess) {
  EXPECT_CALL(*mock_, DeleteEntity("Shoes","4")).WillOnce(Return(true));

  request req; req.method="DELETE"; req.uri="/api/Shoes/4";
  auto rep = handler_->handle_request(req);
  EXPECT_EQ(rep->status, reply::ok);
}

TEST_F(APIHandlerTest, DeleteMissing) {
  EXPECT_CALL(*mock_, DeleteEntity("Shoes","404")).WillOnce(Return(false));

  request req; req.method="DELETE"; req.uri="/api/Shoes/404";
  auto rep = handler_->handle_request(req);
  EXPECT_EQ(rep->status, reply::not_found);
}

/* ----- GET list ----- */
TEST_F(APIHandlerTest, ListSuccess) {
  std::vector<std::string> ids{"1","2"};
  EXPECT_CALL(*mock_, ListEntities("Shoes", _))
      .WillOnce(DoAll(SetArgReferee<1>(ids), Return(true)));

  request req; req.method="GET"; req.uri="/api/Shoes";
  auto rep = handler_->handle_request(req);

  EXPECT_EQ(rep->status, reply::ok);
  EXPECT_EQ(rep->content, "[1,2]");
}

TEST_F(APIHandlerTest, ListNotFound) {
  EXPECT_CALL(*mock_, ListEntities("Unknown", _)).WillOnce(Return(false));
  request req; req.method="GET"; req.uri="/api/Unknown";
  auto rep = handler_->handle_request(req);
  EXPECT_EQ(rep->status, reply::not_found);
}
