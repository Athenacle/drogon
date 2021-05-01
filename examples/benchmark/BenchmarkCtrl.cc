#include "BenchmarkCtrl.h"
void BenchmarkCtrl::asyncHandleHttpRequest(
    const HttpRequestPtr &,
    const HttpOperation &op,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    // write your application logic here
    auto resp = op.newHttpResponse();
    resp->setBody("<p>Hello, world!</p>");
    resp->setExpiredTime(0);
    callback(resp);
}
