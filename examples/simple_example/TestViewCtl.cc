#include "TestViewCtl.h"
void TestViewCtl::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    // write your application logic here
    drogon::HttpViewData data;
    data.insert("title", std::string("TestView"));
    auto res = drogon::HttpResponse::newHttpViewResponse(req->getApp(),
                                                         "TestView",
                                                         data);
    callback(res);
}
