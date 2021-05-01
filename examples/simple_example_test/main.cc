/**
 *
 *  @file test.cc
 *  @author An Tao
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  https://github.com/an-tao/drogon
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Drogon
 *
 */

// Make a http client to test the example server app;

#include <drogon/drogon.h>
#include <trantor/net/EventLoopThread.h>
#include <trantor/net/TcpClient.h>

#include <mutex>
#include <future>
#include <algorithm>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <sys/stat.h>

#define RESET "\033[0m"
#define RED "\033[31m"    /* Red */
#define GREEN "\033[32m"  /* Green */
#define YELLOW "\033[33m" /* Yellow */

#define JPG_LEN 44618
size_t indexLen;
size_t indexImplicitLen;

using namespace drogon;

Cookie sessionID;

void outputGood(const HttpRequestPtr &req, bool isHttps)
{
    static int i = 0;
    // auto start = req->creationDate();
    // auto end = trantor::Date::now();
    // int ms = end.microSecondsSinceEpoch() - start.microSecondsSinceEpoch();
    // ms = ms / 1000;
    // char str[32];
    // sprintf(str, "%6dms", ms);
    static std::mutex mtx;
    {
        std::lock_guard<std::mutex> lock(mtx);
        ++i;
        std::cout << i << GREEN << '\t' << "Good" << '\t' << RED
                  << req->methodString() << " " << req->path();
        if (isHttps)
            std::cout << "\t(https)";
        std::cout << RESET << std::endl;
    }
}

auto &getBadCount()
{
    static std::atomic_int count;
    return count;
}

const std::string &dispatchReqResult(const ReqResult &r)
{
    static std::string un{"UnknownResult"};
    static std::map<ReqResult, std::string> map = {
        {ReqResult::Ok, "Ok"},
        {ReqResult::BadResponse, "BadResponse"},
        {ReqResult::BadServerAddress, "BadServerAddress"},
        {ReqResult::NetworkFailure, "NetworkFailure"},
        {ReqResult::Timeout, "Timeout"}};
    const auto iter = map.find(r);
    if (iter == map.end())
    {
        return un;
    }
    else
    {
        return iter->second;
    }
}

void outputBad(const HttpRequestPtr &req,
               bool isHttps,
               const ReqResult &result,
               const HttpResponsePtr &)
{
    static std::mutex mtx;
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto i = getBadCount().fetch_add(1);
        std::cout << i << YELLOW << '\t' << "Bad" << '\t' << req->methodString()
                  << "------"
                  << " " << req->path() << ": " << dispatchReqResult(result);
        if (isHttps)
            std::cout << "\t(https)";
        std::cout << RESET << std::endl;
    }
}

void doTest(const HttpClientPtr &client,
            std::promise<int> &pro,
            bool isHttps = false)
{
    /// Get cookie
    if (!sessionID)
    {
        auto req = HttpRequest::newHttpRequest(client->getApp());
        req->setMethod(drogon::Get);
        req->setPath("/");
        std::promise<int> waitCookie;
        auto f = waitCookie.get_future();
        client->sendRequest(req,
                            [client, &waitCookie](ReqResult result,
                                                  const HttpResponsePtr &resp) {
                                if (result == ReqResult::Ok)
                                {
                                    auto &id = resp->getCookie("JSESSIONID");
                                    if (!id)
                                    {
                                        LOG_ERROR << "Error!";
                                        exit(1);
                                    }
                                    sessionID = id;
                                    client->addCookie(id);
                                    waitCookie.set_value(1);
                                }
                                else
                                {
                                    LOG_ERROR << "Error!";
                                    exit(1);
                                }
                            });
        f.get();
    }
    /// Test session
    auto app = client->getApp();
    auto req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/slow");
    client->sendRequest(
        req,
        [req, isHttps, client, app](ReqResult result, const HttpResponsePtr &) {
            if (result == ReqResult::Ok)
            {
                outputGood(req, isHttps);
                auto req1 = HttpRequest::newHttpRequest(app);
                req1->setMethod(drogon::Get);
                req1->setPath("/slow");
                client->sendRequest(
                    req1,
                    [req1, isHttps](ReqResult result,
                                    const HttpResponsePtr &resp1) {
                        if (result == ReqResult::Ok)
                        {
                            auto &json = resp1->jsonObject();
                            if (json && json->get("message", std::string(""))
                                                .asString() ==
                                            "Access interval should be "
                                            "at least 10 "
                                            "seconds")
                            {
                                outputGood(req1, isHttps);
                            }
                            else
                            {
                                outputBad(req1, isHttps, result, resp1);
                            }
                        }
                        else
                        {
                            outputBad(req1, isHttps, result, resp1);
                        }
                    });
            }
            else
            {
                LOG_ERROR << "Error!";
                exit(1);
            }
        });
    /// Test gzip
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->addHeader("accept-encoding", "gzip");
    req->setPath("/api/v1/apitest/get/111");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getBody().length() == 4994)
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });
/// Test brotli
#ifdef USE_BROTLI
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->addHeader("accept-encoding", "br");
    req->setPath("/api/v1/apitest/get/111");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getBody().length() == 4994)
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });
#endif
    /// Post json
    Json::Value json;
    json["request"] = "json";
    req = HttpRequest::newCustomHttpRequest(app, json);
    req->setMethod(drogon::Post);
    req->setPath("/api/v1/apitest/json");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                std::shared_ptr<Json::Value> ret = *resp;
                                if (ret && (*ret)["result"].asString() == "ok")
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });
    // Post json again
    req = HttpRequest::newHttpJsonRequest(app, json);
    assert(req->jsonObject());
    req->setMethod(drogon::Post);
    req->setPath("/api/v1/apitest/json");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                std::shared_ptr<Json::Value> ret = *resp;
                                if (ret && (*ret)["result"].asString() == "ok")
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });

    // Post json again
    req = HttpRequest::newHttpJsonRequest(app, json);
    req->setMethod(drogon::Post);
    req->setPath("/api/v1/apitest/json");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                std::shared_ptr<Json::Value> ret = *resp;
                                if (ret && (*ret)["result"].asString() == "ok")
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });
    // Test 404
    req = HttpRequest::newHttpJsonRequest(app, json);
    req->setMethod(drogon::Get);
    req->setPath("/api/v1/apitest/notFoundRouting");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getStatusCode() == k404NotFound)
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    LOG_DEBUG << resp->getBody();
                                    LOG_ERROR << "Error!";
                                    exit(1);
                                }
                            }
                            else
                            {
                                LOG_ERROR << "Error!";
                                exit(1);
                            }
                        });
    /// 1 Get /
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getBody() == "<p>Hello, world!</p>")
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });
    /// 3. Post to /tpost to test Http Method constraint
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/tpost");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->statusCode() == k405MethodNotAllowed)
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });

    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Post);
    req->setPath("/tpost");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getBody() == "<p>Hello, world!</p>")
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });

    /// 4. Http OPTIONS Method
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Options);
    req->setPath("/tpost");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                auto allow = resp->getHeader("allow");
                                if (resp->statusCode() == k200OK &&
                                    allow.find("POST") != std::string::npos)
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });

    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Options);
    req->setPath("/api/v1/apitest");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                auto allow = resp->getHeader("allow");
                                if (resp->statusCode() == k200OK &&
                                    allow == "OPTIONS,GET,HEAD,POST")
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });

    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Options);
    req->setPath("/slow");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->statusCode() == k403Forbidden)
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });

    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Options);
    req->setPath("/*");
    client->sendRequest(
        req, [req, isHttps](ReqResult result, const HttpResponsePtr &resp) {
            if (result == ReqResult::Ok)
            {
                auto allow = resp->getHeader("allow");
                if (resp->statusCode() == k200OK &&
                    allow == "GET,HEAD,POST,PUT,DELETE,OPTIONS,PATCH")
                {
                    outputGood(req, isHttps);
                }
                else
                {
                    outputBad(req, isHttps, result, resp);
                }
            }
            else
            {
                outputBad(req, isHttps, result, resp);
            }
        });

    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Options);
    req->setPath("/api/v1/apitest/static");
    client->sendRequest(
        req, [req, isHttps](ReqResult result, const HttpResponsePtr &resp) {
            if (result == ReqResult::Ok)
            {
                auto allow = resp->getHeader("allow");
                if (resp->statusCode() == k200OK && allow == "OPTIONS,GET,HEAD")
                {
                    outputGood(req, isHttps);
                }
                else
                {
                    outputBad(req, isHttps, result, resp);
                }
            }
            else
            {
                outputBad(req, isHttps, result, resp);
            }
        });

    /// 4. Test HttpController
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Post);
    req->setPath("/api/v1/apitest");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getBody() == "ROOT Post!!!")
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });

    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/api/v1/apitest");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getBody() == "ROOT Get!!!")
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });

    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/api/v1/apitest/get/abc/123");
    client->sendRequest(
        req, [req, isHttps](ReqResult result, const HttpResponsePtr &resp) {
            if (result == ReqResult::Ok)
            {
                if (resp->getBody().find("<td>p1</td>\n        <td>123</td>") !=
                        std::string::npos &&
                    resp->getBody().find("<td>p2</td>\n        <td>abc</td>") !=
                        std::string::npos)
                {
                    outputGood(req, isHttps);
                }
                else
                {
                    outputBad(req, isHttps, result, resp);
                }
            }
            else
            {
                outputBad(req, isHttps, result, resp);
            }
        });

    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/api/v1/apitest/3.14/List");
    req->setParameter("P2", "1234");
    client->sendRequest(
        req, [req, isHttps](ReqResult result, const HttpResponsePtr &resp) {
            if (result == ReqResult::Ok)
            {
                if (resp->getBody().find(
                        "<td>p1</td>\n        <td>3.140000</td>") !=
                        std::string::npos &&
                    resp->getBody().find(
                        "<td>p2</td>\n        <td>1234</td>") !=
                        std::string::npos)
                {
                    outputGood(req, isHttps);
                }
                else
                {
                    outputBad(req, isHttps, result, resp);
                }
            }
            else
            {
                outputBad(req, isHttps, result, resp);
            }
        });

    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/reg/123/rest/of/the/path");
    client->sendRequest(
        req, [req, isHttps](ReqResult result, const HttpResponsePtr &resp) {
            if (result == ReqResult::Ok && resp->getJsonObject())
            {
                auto &json = resp->getJsonObject();
                if (json->isMember("p1") && json->get("p1", 0).asInt() == 123 &&
                    json->isMember("p2") &&
                    json->get("p2", "").asString() == "rest/of/the/path")
                {
                    outputGood(req, isHttps);
                }
                else
                {
                    outputBad(req, isHttps, result, resp);
                }
            }
            else
            {
                outputBad(req, isHttps, result, resp);
            }
        });

    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/api/v1/apitest/static");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getBody() == "staticApi,hello!!")
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });

    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Post);
    req->setPath("/api/v1/apitest/static");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getBody() == "staticApi,hello!!")
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });

    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/api/v1/apitest/get/111");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getBody().length() == 4994)
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });

    /// Test static function
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/api/v1/handle11/11/2 2/?p3=3 x");
    req->setParameter("p4", "44");
    client->sendRequest(
        req, [req, isHttps](ReqResult result, const HttpResponsePtr &resp) {
            if (result == ReqResult::Ok)
            {
                if (resp->getBody().find(
                        "<td>int p1</td>\n        <td>11</td>") !=
                        std::string::npos &&
                    resp->getBody().find(
                        "<td>int p4</td>\n        <td>44</td>") !=
                        std::string::npos &&
                    resp->getBody().find(
                        "<td>string p2</td>\n        <td>2 2</td>") !=
                        std::string::npos &&
                    resp->getBody().find(
                        "<td>string p3</td>\n        <td>3 x</td>") !=
                        std::string::npos)
                {
                    outputGood(req, isHttps);
                }
                else
                {
                    outputBad(req, isHttps, result, resp);
                }
            }
            else
            {
                outputBad(req, isHttps, result, resp);
            }
        });
    /// Test Incomplete URL
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/api/v1/handle11/11/2 2/");
    client->sendRequest(
        req, [req, isHttps](ReqResult result, const HttpResponsePtr &resp) {
            if (result == ReqResult::Ok)
            {
                if (resp->getBody().find(
                        "<td>int p1</td>\n        <td>11</td>") !=
                        std::string::npos &&
                    resp->getBody().find(
                        "<td>int p4</td>\n        <td>0</td>") !=
                        std::string::npos &&
                    resp->getBody().find(
                        "<td>string p2</td>\n        <td>2 2</td>") !=
                        std::string::npos &&
                    resp->getBody().find(
                        "<td>string p3</td>\n        <td></td>") !=
                        std::string::npos)
                {
                    outputGood(req, isHttps);
                }
                else
                {
                    outputBad(req, isHttps, result, resp);
                }
            }
            else
            {
                outputBad(req, isHttps, result, resp);
            }
        });
    /// Test lambda
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/api/v1/handle2/111/222");
    client->sendRequest(
        req, [req, isHttps](ReqResult result, const HttpResponsePtr &resp) {
            if (result == ReqResult::Ok)
            {
                if (resp->getBody().find("<td>a</td>\n        <td>111</td>") !=
                        std::string::npos &&
                    resp->getBody().find(
                        "<td>b</td>\n        <td>222.000000</td>") !=
                        std::string::npos)
                {
                    outputGood(req, isHttps);
                }
                else
                {
                    outputBad(req, isHttps, result, resp);
                }
            }
            else
            {
                outputBad(req, isHttps, result, resp);
            }
        });

    /// Test std::bind and std::function
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/api/v1/handle4/444/333/111");
    client->sendRequest(
        req, [req, isHttps](ReqResult result, const HttpResponsePtr &resp) {
            if (result == ReqResult::Ok)
            {
                if (resp->getBody().find(
                        "<td>int p1</td>\n        <td>111</td>") !=
                        std::string::npos &&
                    resp->getBody().find(
                        "<td>int p4</td>\n        <td>444</td>") !=
                        std::string::npos &&
                    resp->getBody().find(
                        "<td>string p3</td>\n        <td>333</td>") !=
                        std::string::npos &&
                    resp->getBody().find(
                        "<td>string p2</td>\n        <td></td>") !=
                        std::string::npos)
                {
                    outputGood(req, isHttps);
                }
                else
                {
                    outputBad(req, isHttps, result, resp);
                }
            }
            else
            {
                outputBad(req, isHttps, result, resp);
            }
        });
    /// Test gzip_static
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/index.html");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getBody().length() == indexLen)
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/index.html");
    req->addHeader("accept-encoding", "gzip");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getBody().length() == indexLen)
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });
    // Test 405
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Post);
    req->setPath("/drogon.jpg");
    client->sendRequest(
        req,
        [req, client, isHttps, &pro](ReqResult result,
                                     const HttpResponsePtr &resp) {
            if (result == ReqResult::Ok)
            {
                if (resp->getStatusCode() == k405MethodNotAllowed)
                {
                    outputGood(req, isHttps);
                }
                else
                {
                    LOG_DEBUG << resp->getBody().length();
                    LOG_ERROR << "Error!";
                    exit(1);
                }
            }
            else
            {
                LOG_ERROR << "Error!";
                exit(1);
            }
        });
    /// Test file download
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/drogon.jpg");
    client->sendRequest(
        req,
        [req, client, isHttps, &pro, app](ReqResult result,
                                          const HttpResponsePtr &resp) {
            if (result == ReqResult::Ok)
            {
                if (resp->getBody().length() == JPG_LEN)
                {
                    outputGood(req, isHttps);
                    auto &lastModified = resp->getHeader("last-modified");
                    // LOG_DEBUG << lastModified;
                    // Test 'Not Modified'
                    auto req = HttpRequest::newHttpRequest(app);
                    req->setMethod(drogon::Get);
                    req->setPath("/drogon.jpg");
                    req->addHeader("If-Modified-Since", lastModified);
                    client->sendRequest(
                        req,
                        [req, isHttps, &pro](ReqResult result,
                                             const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->statusCode() == k304NotModified)
                                {
                                    outputGood(req, isHttps);
                                    pro.set_value(1);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });
                }
                else
                {
                    outputBad(req, isHttps, result, resp);
                }
            }
            else
            {
                outputBad(req, isHttps, result, resp);
            }
        });

    /// Test file download, It is forbidden to download files from the
    /// parent folder
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/../../drogon.jpg");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->statusCode() == k403Forbidden)
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });
    /// Test controllers created and initialized by users
    req = HttpRequest::newHttpRequest(app);
    req->setPath("/customctrl/antao");
    req->addHeader("custom_header", "yes");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                auto ret = resp->getBody();
                                if (ret == "<P>Hi, antao</P>")
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });
    /// Test controllers created and initialized by users
    req = HttpRequest::newHttpRequest(app);
    req->setPath("/absolute/123");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->statusCode() == k200OK)
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });
    /// Test synchronous advice
    req = HttpRequest::newHttpRequest(app);
    req->setPath("/plaintext");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getBody() == "Hello, World!")
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });
    /// Test form post
    req = HttpRequest::newHttpFormPostRequest(app);
    req->setPath("/api/v1/apitest/form");
    req->setParameter("k1", "1");
    req->setParameter("k2", "安");
    req->setParameter("k3", "test@example.com");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                auto ret = resp->getJsonObject();
                                if (ret && (*ret)["result"].asString() == "ok")
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });

    /// Test attributes
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/api/v1/apitest/attrs");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                auto ret = resp->getJsonObject();
                                if (ret && (*ret)["result"].asString() == "ok")
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });

    /// Test attachment download
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/api/attachment/download");
    client->sendRequest(req,
                        [req, isHttps](ReqResult result,
                                       const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getBody().length() == JPG_LEN)
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });
    // Test implicit pages
    std::string body;
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/a-directory");
    client->sendRequest(
        req,
        [req, isHttps, &body](ReqResult result, const HttpResponsePtr &resp) {
            if (result == ReqResult::Ok)
            {
                if (resp->getBody().length() == indexImplicitLen)
                {
                    body = std::string(resp->getBody().data(),
                                       resp->getBody().length());
                    outputGood(req, isHttps);
                }
                else
                {
                    outputBad(req, isHttps, result, resp);
                }
            }
            else
            {
                outputBad(req, isHttps, result, resp);
            }
        });
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/a-directory/page.html");
    client->sendRequest(req,
                        [req, isHttps, &body](ReqResult result,
                                              const HttpResponsePtr &resp) {
                            if (result == ReqResult::Ok)
                            {
                                if (resp->getBody().length() ==
                                        indexImplicitLen &&
                                    std::equal(body.begin(),
                                               body.end(),
                                               resp->getBody().begin()))
                                {
                                    outputGood(req, isHttps);
                                }
                                else
                                {
                                    outputBad(req, isHttps, result, resp);
                                }
                            }
                            else
                            {
                                outputBad(req, isHttps, result, resp);
                            }
                        });
    // return;
    // Test file upload
    UploadFile file1("./drogon.jpg");
    UploadFile file2("./drogon.jpg", "drogon1.jpg");
    UploadFile file3("./config.example.json", "config.json", "file3");
    req = HttpRequest::newFileUploadRequest(app, {file1, file2, file3});
    req->setPath("/api/attachment/upload");
    req->setParameter("P1", "upload");
    req->setParameter("P2", "test");
    client->sendRequest(
        req, [req, isHttps](ReqResult result, const HttpResponsePtr &resp) {
            if (result == ReqResult::Ok)
            {
                auto json = resp->getJsonObject();
                if (json && (*json)["result"].asString() == "ok" &&
                    (*json)["P1"] == "upload" && (*json)["P2"] == "test")
                {
                    outputGood(req, isHttps);
                }
                else
                {
                    outputBad(req, isHttps, result, resp);
                }
            }
            else
            {
                outputBad(req, isHttps, result, resp);
            }
        });
    req = HttpRequest::newHttpRequest(app);
    req->setMethod(drogon::Get);
    req->setPath("/api/v1/this_will_fail");
    client->sendRequest(
        req, [req, isHttps](ReqResult result, const HttpResponsePtr &resp) {
            if (result == ReqResult::Ok)
            {
                if (resp->getStatusCode() != k500InternalServerError)
                {
                    LOG_DEBUG << resp->getStatusCode();
                    LOG_ERROR << "Error!";
                    exit(1);
                }
                outputGood(req, isHttps);
            }
            else
            {
                LOG_ERROR << "Error!";
                exit(1);
            }
        });

#ifdef __cpp_impl_coroutine
    // Test coroutine requests
    [client, isHttps]() -> AsyncTask {
        try
        {
            auto req = HttpRequest::newHttpRequest();
            req->setPath("/api/v1/corotest/get");
            auto resp = co_await client->sendRequestCoro(req);
            if (resp->getBody() != "DEADBEEF")
            {
                LOG_ERROR << resp->getBody();
                LOG_ERROR << "Error!";
                exit(1);
            }
            outputGood(req, isHttps);
        }
        catch (const std::exception &e)
        {
            LOG_DEBUG << e.what();
            LOG_ERROR << "Error!";
            exit(1);
        }
    }();

    // Test coroutine request with co_return
    [client, isHttps]() -> AsyncTask {
        try
        {
            auto req = HttpRequest::newHttpRequest();
            req->setPath("/api/v1/corotest/get2");
            auto resp = co_await client->sendRequestCoro(req);
            if (resp->getBody() != "BADDBEEF")
            {
                LOG_ERROR << resp->getBody();
                LOG_ERROR << "Error!";
                exit(1);
            }
            outputGood(req, isHttps);
        }
        catch (const std::exception &e)
        {
            LOG_DEBUG << e.what();
            LOG_ERROR << "Error!";
            exit(1);
        }
    }();
#endif
}
void loadFileLengths()
{
    struct stat filestat;
    if (stat("index.html", &filestat) < 0)
    {
        LOG_SYSERR << "Unable to retrieve index.html file sizes";
        exit(1);
    }
    indexLen = filestat.st_size;
    if (stat("a-directory/page.html", &filestat) < 0)
    {
        LOG_SYSERR << "Unable to retrieve a-directory/page.html file sizes";
        exit(1);
    }
    indexImplicitLen = filestat.st_size;
}

int main(int argc, char *argv[])
{
    auto app = HttpAppFramework::create();
    auto &op = app->getOperations();

    trantor::EventLoopThread loop[2];
    trantor::Logger::setLogLevel(trantor::Logger::LogLevel::kDebug);
    bool ever = false;
    if (argc > 1 && std::string(argv[1]) == "-f")
        ever = true;
    loop[0].run();
    loop[1].run();
    loadFileLengths();
    do
    {
        std::promise<int> pro1;
        auto client =
            op.newHttpClient("http://127.0.0.1:8848", loop[0].getLoop());

        client->setPipeliningDepth(10);
        if (sessionID)
            client->addCookie(sessionID);
        doTest(client, pro1);
        if (app->supportSSL())
        {
            std::promise<int> pro2;
            auto sslClient =
                op.newHttpClient("127.0.0.1", 8849, true, loop[1].getLoop());
            // auto sslClient = HttpClient::newHttpClient(
            //     "127.0.0.1", 8849, true, loop[1].getLoop(), false, false);
            if (sessionID)
                sslClient->addCookie(sessionID);
            doTest(sslClient, pro2, true);
            auto f2 = pro2.get_future();
            f2.get();
        }
        auto f1 = pro1.get_future();
        f1.get();
    } while (ever);
    // getchar();
    loop[0].getLoop()->quit();
    loop[1].getLoop()->quit();

    return getBadCount();
}
