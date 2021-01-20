#include <drogon/HttpClient.h>
#include <drogon/HttpAppFramework.h>
#include <string>
#include <iostream>
#include <atomic>
using namespace drogon;
int main()
{
    auto app = drogon::create();
    auto &op = app->getOperations();

    trantor::Logger::setLogLevel(trantor::Logger::kTrace);
    auto client = op.newHttpClient("127.0.0.1", 8848);
    client->setPipeliningDepth(64);
    int counter = -1;
    int n = 0;

    auto request1 = HttpRequest::newHttpRequest(nullptr);
    request1->setPath("/pipe");
    request1->setMethod(Head);

    client->sendRequest(request1,
                        [&counter, &n, &op](ReqResult r,
                                            const HttpResponsePtr &resp) {
                            if (r == ReqResult::Ok)
                            {
                                auto counterHeader = resp->getHeader("counter");
                                int c = atoi(counterHeader.data());
                                if (c <= counter)
                                {
                                    LOG_ERROR << "The response was received in "
                                                 "the wrong order!";
                                    exit(1);
                                }
                                else
                                {
                                    counter = c;
                                    ++n;
                                }
                                if (resp->getBody().length() > 0)
                                {
                                    LOG_ERROR << "The response has a body:"
                                              << resp->getBody();
                                    exit(1);
                                }
                            }
                            else
                            {
                                exit(1);
                            }
                        });

    auto request2 = op.newHttpRequest();
    request2->setPath("/drogon.jpg");
    client->sendRequest(request2, [](ReqResult r, const HttpResponsePtr &resp) {
        if (r == ReqResult::Ok)
        {
            if (resp->getBody().length() != 44618)
            {
                LOG_ERROR << "The response is error!";
                exit(1);
            }
        }
        else
        {
            exit(1);
        }
    });

    auto request = HttpRequest::newHttpRequest(nullptr);
    request->setPath("/pipe");
    for (int i = 0; i < 19; ++i)
    {
        client->sendRequest(
            request,
            [&counter, &n, app, &op](ReqResult r, const HttpResponsePtr &resp) {
                if (r == ReqResult::Ok)
                {
                    auto counterHeader = resp->getHeader("counter");
                    int c = atoi(counterHeader.data());
                    if (c <= counter)
                    {
                        LOG_ERROR
                            << "The response was received in the wrong order!";
                        exit(1);
                    }
                    else
                    {
                        counter = c;
                        ++n;
                        if (n == 20)
                        {
                            LOG_DEBUG << "Good!";
                            op.getLoop()->quit();
                        }
                    }
                    if (resp->getBody().length() == 0)
                    {
                        LOG_ERROR << "The response hasn't a body!";
                        exit(1);
                    }
                }
                else
                {
                    exit(1);
                }
            });
    }
    app->run();
}
