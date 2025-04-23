#include "gtest/gtest.h"
#include "server.h"

class ServerTest:public::testing::Test
{
    protected:
        boost::asio::io_service io_service;
        short port = 8080;
};


TEST_F(ServerTest, ServerConstruction) {
    server s(io_service,port);
    std::cout << "test" << std::endl;
    EXPECT_TRUE(true);
}
