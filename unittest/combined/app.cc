#include "combined.h"

#include <drogon/HttpSimpleController.h>
#include <drogon/HttpClient.h>

#include <chrono>
using namespace std::chrono_literals;
int HttpApp::port = 63333;

#include <pthread.h>

using namespace drogon;

class C : public drogon::HttpController<C>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(C::root, "/");
    METHOD_LIST_END
    void root(const HttpRequestPtr &req,
              const HttpOperation &op,
              std::function<void(const HttpResponsePtr &)> &&callback) const
    {
        auto resp = op.newHttpResponse();
        resp->setBody(ROOT_BODY);
        resp->setStatusCode(k200OK);
        callback(resp);
    }
};

KVPairTable HttpApp::emptyTable = {};

HttpApp::HttpApp()
{
    std::string name;
    getMyTestName(name);
    LOG_INFO << name << ": "
             << "HttpApp::HttpApp";
}

void HttpApp::start()
{
    auto info = ::testing::UnitTest::GetInstance()->current_test_info();
    LOG_INFO << info->test_suite_name() << "::" << info->name() << " "
             << __func__;
    setLogName();
    increaseLogIndent();

    Barrier bar(2);

    appThread = std::thread(
        [this, &bar]() { this->app->run([&bar]() { bar.wait(); }); });
    bar.wait();
}

void HttpApp::SetUp()
{
    app = drogon::create();
    app->setLogLevel(trantor::Logger::kTrace);
    app->addListener("127.0.0.1", port);
}

void HttpApp::TearDown()
{
    if (app->isRunning())
    {
        app->quit();
        appThread.join();

        drogon::destroy(app);
        app.reset();
    }

    auto info = ::testing::UnitTest::GetInstance()->current_test_info();
    decendLogIndent();
    resetLogName();
    LOG_INFO << info->test_suite_name() << "::" << info->name() << " "
             << __func__;
}

HttpRequestPtr mkRequest(const HttpOperation &op,
                         HttpMethod mtd,
                         const std::string &host,
                         const std::string &url,
                         const KVPairTable &query,
                         const KVPairTable &header)
{
    auto req = op.newHttpRequest();
    req->setMethod(mtd);
    req->setPath(url);
    for (const auto &kv : query)
    {
        req->setParameter(kv.first, kv.second);
    }
    return req;
}

HttpApp::ForwardReturnType HttpApp::forward(HttpMethod mtd,
                                            const std::string &host,
                                            const std::string &url,
                                            const KVPairTable &query,
                                            const KVPairTable &header)
{
    auto &op = app->getOperations();
    auto client = op.newHttpClient("127.0.0.1", port);
    auto req = mkRequest(app->getOperations(), mtd, host, url, query, header);

    return client->sendRequest(req);
}

HttpApp::ForwardReturnType HttpApp::forward(HttpMethod mtd,
                                            const std::string &host,
                                            const std::string &url,
                                            const DataBlob &blob,
                                            ContentType type,
                                            const KVPairTable &query,
                                            const KVPairTable &header)
{
    return forward(mtd, host, url, query, header);
}

void getMyTestName(std::string &out)
{
    auto info = ::testing::UnitTest::GetInstance()->current_test_info();

    out = fmt::format("{}::{}", info->test_suite_name(), info->name());
}

std::string generatorRandom(const std::string &prefix, size_t len)
{
    std::string tmp_s(prefix);
    if (len > 1000)
    {
        len = 1000;
    }
    if (len == 0)
    {
        len = 10;
    }
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    tmp_s.reserve(len + prefix.length() + 1);

    for (int i = 0; i < len; ++i)
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];

    return tmp_s;
}

TEST_F(HttpApp, rootOK)
{
    start();

    const auto &[req, resp] = forward(Get, "", "/");

    ASSERT_EQ(req, ReqResult::Ok);
    ASSERT_EQ(resp->getStatusCode(), k200OK);
    auto body = resp->getBody();
    ASSERT_EQ(body.length(), strlen(ROOT_BODY));
    ASSERT_TRUE(body == ROOT_BODY);
}