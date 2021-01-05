#include "JsonCtrl.h"
void JsonCtrl::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    const HttpOperation &op,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    Json::Value ret;
    ret["message"] = "Hello, World!";
    auto resp = op.newHttpJsonResponse(ret);
    callback(resp);
}