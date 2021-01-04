#include "PipeliningTest.h"
#include <trantor/net/EventLoop.h>
#include <atomic>

void PipeliningTest::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto app = req->getApp();
    static std::atomic<int> counter{0};
    int c = counter.fetch_add(1);
    if (c % 3 == 1)
    {
        auto resp = HttpResponse::newHttpResponse(app);
        auto str = utils::formattedString("<P>the %dth response</P>", c);
        resp->addHeader("counter", utils::formattedString("%d", c));
        resp->setBody(std::move(str));
        callback(resp);
        return;
    }
    double delay = ((double)(10 - (c % 10))) / 10.0;
    if (c % 3 == 2)
    {
        // call the callback in another thread.
        reinterpret_cast<HttpAppFramework *>(app)->getLoop()->runAfter(
            delay, [c, callback, app]() {
                auto resp = HttpResponse::newHttpResponse(app);
                auto str =
                    utils::formattedString("<P>the %dth response</P>", c);
                resp->addHeader("counter", utils::formattedString("%d", c));
                resp->setBody(std::move(str));
                callback(resp);
            });
        return;
    }
    trantor::EventLoop::getEventLoopOfCurrentThread()->runAfter(
        delay, [c, callback, app]() {
            auto resp = HttpResponse::newHttpResponse(app);
            auto str = utils::formattedString("<P>the %dth response</P>", c);
            resp->addHeader("counter", utils::formattedString("%d", c));
            resp->setBody(std::move(str));
            callback(resp);
        });
}
