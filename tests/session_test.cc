#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "session.h"

class SessionTest:public::testing::Test
{
    protected:
        boost::asio::io_service io_service;
        char request_data_1[1] = "";
        char request_data_2[5] = "\r\n\r\n";
        char request_data_3[40] = "GET / HTTP/1.1\r\nHost: Bhe\r\n\r\n";
        char request_data_4[6] = "hello";
        bool status;
};

// test session constructor
TEST_F(SessionTest, SessionConstruction) {
    session s(io_service);
    EXPECT_TRUE(true);
}

