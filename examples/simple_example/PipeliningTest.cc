#include "PipeliningTest.h"
#include <trantor/net/EventLoop.h>
#include <atomic>

void PipeliningTest::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    const HttpOperation &op,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    // auto app = req->getApp();
    static std::atomic<int> counter{0};
    int c = counter.fetch_add(1);
    if (c % 3 == 1)
    {
        auto resp = op.newHttpResponse();
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
        op.getLoop()->runAfter(delay, [c, callback, &op]() {
            auto resp = op.newHttpResponse();
            auto str = utils::formattedString("<P>the %dth response</P>", c);
            resp->addHeader("counter", utils::formattedString("%d", c));
            resp->setBody(std::move(str));
            callback(resp);
        });
        return;
    }
    trantor::EventLoop::getEventLoopOfCurrentThread()->runAfter(
        delay, [c, callback, &op]() {
            auto resp = op.newHttpResponse();
            auto str = utils::formattedString("<P>the %dth response</P>", c);
            resp->addHeader("counter", utils::formattedString("%d", c));
            resp->setBody(std::move(str));
            callback(resp);
        });
}
