#include "ForwardCtrl.h"
void ForwardCtrl::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    const HttpOperation &op,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    req->setPath("/repos/an-tao/drogon/git/refs/heads/master");
    (reinterpret_cast<HttpAppFramework *>(req->getApp()))
        ->forward(
            req,
            [callback = std::move(callback)](const HttpResponsePtr &resp) {
                callback(resp);
            },
            "https://api.github.com");
}