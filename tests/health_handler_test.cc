#include "gtest/gtest.h"
#include "handlers/health_handler.h"
#include "request.h"
#include "response.h"

TEST(HealthHandlerTest, Returns200OK) {
    HealthHandler handler;

    std::string raw_request = "GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n";
    Request req(raw_request);

    std::unique_ptr<Response> res = handler.handle_request(req);

    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status(), Response::StatusCode::OK);
    EXPECT_NE(res->to_string().find("OK"), std::string::npos);
}
TEST(HealthHandlerTest, InvalidMethodReturns400) {
    HealthHandler handler;

    // Use POST instead of GET
    std::string raw_request = "POST /health HTTP/1.1\r\nHost: localhost\r\n\r\n";
    Request req(raw_request);

    std::unique_ptr<Response> res = handler.handle_request(req);

    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status(), Response::StatusCode::BAD_REQUEST);
    EXPECT_NE(res->to_string().find("Method not allowed"), std::string::npos);
}
