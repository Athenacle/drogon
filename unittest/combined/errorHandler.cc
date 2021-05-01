#include "combined.h"

TEST_F(HttpApp, base404)
{
    start();
    auto ret = forward(Get, "", generatorRandom("/"));
    ASSERT_EQ(std::get<0>(ret), ReqResult::Ok);
    auto resp = std::get<1>(ret);
    ASSERT_EQ(resp->statusCode(), k404NotFound);

    auto& ct = resp->getHeader("content-type");

    EXPECT_EQ(strncmp(ct.c_str(), "text/html", sizeof("text/html") - 1), 0)
        << ct;
}

namespace
{
auto errorHandler(int* count,
                  bool cache,
                  HttpStatusCode sc,
                  const HttpRequestPtr& req,
                  const HttpOperation& op)
{
    auto resp = op.newHttpResponse();

    resp->setStatusCode(sc);

    resp->setBody(
        fmt::format("Request {}", req == nullptr ? "null" : "not null"));

    if (req)
    {
        resp->addHeader("path", req->getPath());
    }

    if (cache)
    {
        EXPECT_EQ(req, nullptr);
    }
    else
    {
        EXPECT_NE(req, nullptr);
    }

    *count += 1;

    return resp;
}
}  // namespace

TEST_F(HttpApp, custom404Cached)
{
    int called = 0;

    app->setCustomErrorHandler(std::bind(errorHandler,
                                         &called,
                                         true,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3),
                               true);

    size_t tn = 3;
    app->setThreadNum(tn);

    start();

    int times = 10;
    for (int i = 0; i < 10; i++)
    {
        const auto& [st, resp] = forward(Get, "", generatorRandom("/", 10));
        EXPECT_EQ(st, ReqResult::Ok);
        EXPECT_EQ(resp->getStatusCode(), k404NotFound);
        auto body = resp->getBody();
        EXPECT_TRUE(body == "Request null");
        /*  tn + 1:
         *  refer to
         *      HttpResponse::newNotFoundResponse
         *                std::call_once(threadOnce, [app] {
         *                     thread404Pages.init(...);
         *                });
         *  and
         *      void IOThreadStorage::init(const InitCallback &initCB)
         *  and
         *      IOThreadStorage::IOThreadStorage(...)
         *          size_t numThreads = app_->getThreadNum();
         *          storage_.reserve(numThreads + 1);
         */

        EXPECT_EQ(called, tn + 1);
    }
    EXPECT_EQ(called, tn + 1);
}

TEST_F(HttpApp, custom404NoCached)
{
    int called = 0;

    app->setCustomErrorHandler(std::bind(errorHandler,
                                         &called,
                                         false,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3),
                               false);
    app->setThreadNum(2);
    start();

    int times = 10;
    for (int i = 0; i < times; i++)
    {
        std::string&& p = generatorRandom("/", 10);
        const auto& [st, resp] = forward(Get, "", p);
        EXPECT_EQ(st, ReqResult::Ok);
        EXPECT_EQ(resp->getStatusCode(), k404NotFound);
        auto body = resp->getBody();
        EXPECT_TRUE(body == "Request not null");
        auto path = resp->getHeader("path");
        EXPECT_TRUE(path == p);
        EXPECT_EQ(called, i + 1);
    }
    EXPECT_EQ(called, times);
}