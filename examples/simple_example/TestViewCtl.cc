#include "TestViewCtl.h"
void TestViewCtl::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    const HttpOperation &op,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    // write your application logic here
    drogon::HttpViewData data;
    data.insert("title", std::string("TestView"));
    auto res = op.newHttpViewResponse("TestView", data);
    callback(res);
}
