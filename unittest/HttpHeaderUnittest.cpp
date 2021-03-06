#include "../lib/src/HttpRequestImpl.h"
#include "../lib/src/HttpResponseImpl.h"
#include "../lib/inc/drogon/HttpAppFramework.h"

#include <gtest/gtest.h>
using namespace drogon;

std::shared_ptr<drogon::HttpAppFramework> app;

TEST(HttpHeader, Request)
{
    auto req = std::dynamic_pointer_cast<HttpRequestImpl>(
        app->getOperations().newHttpRequest());
    req->addHeader("Abc", "abc");
    EXPECT_STREQ("abc", req->getHeader("Abc").c_str());
    EXPECT_STREQ("abc", req->getHeader("abc").c_str());
    req->removeHeader("Abc");
    EXPECT_STREQ("", req->getHeader("abc").c_str());
}
TEST(HttpHeader, Response)
{
    auto resp = std::dynamic_pointer_cast<HttpResponseImpl>(
        app->getOperations().newHttpResponse());
    resp->addHeader("Abc", "abc");
    EXPECT_STREQ("abc", resp->getHeader("Abc").c_str());
    EXPECT_STREQ("abc", resp->getHeader("abc").c_str());
    resp->makeHeaderString();
    auto buffer = resp->renderToBuffer();
    auto str = std::string{buffer->peek(), buffer->readableBytes()};
    EXPECT_TRUE(str.find("abc") != std::string::npos);
    resp->removeHeader("Abc");
    buffer = resp->renderToBuffer();
    str = std::string{buffer->peek(), buffer->readableBytes()};
    EXPECT_TRUE(str.find("abc") == std::string::npos);
    EXPECT_STREQ("", resp->getHeader("abc").c_str());
}

int main(int argc, char** argv)
{
    app = drogon::create();
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}