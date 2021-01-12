
#include <drogon/HttpOperation.h>
#include <trantor/utils/Logger.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpClient.h>
#include <drogon/WebSocketClient.h>
#include "HttpAppFrameworkImpl.h"

using namespace drogon;

namespace
{
HttpResponsePtr emptyResponse()
{
    assert(false);
    LOG_ERROR << "empty newHttpResponse called";
}

HttpResponsePtr json(const Json::Value &)
{
    assert(false);
    LOG_ERROR << "empty newHttpResponse called";
}

HttpResponsePtr jsonLV(Json::Value &&)
{
    assert(false);
    LOG_ERROR << "empty newJsonLVResponse called";
}

HttpResponsePtr httpView(const std::string &, const HttpViewData &)
{
    assert(false);
    LOG_ERROR << "empty newHttpView called";
}

HttpResponsePtr redirect(const std::string &, HttpStatusCode)
{
    assert(false);
    LOG_ERROR << "empty newRedirection called";
}

HttpResponsePtr file(const std::string &, const std::string &, ContentType)
{
    assert(false);
    LOG_ERROR << "empty newFileResponse called";
}

HttpRequestPtr httpReq()
{
    assert(false);
    LOG_ERROR << "empty newHttpRequest called";
}

HttpClientPtr clientIpPort(const std::string &,
                           uint16_t,
                           bool,
                           trantor::EventLoop *,
                           bool)
{
    assert(false);
    LOG_ERROR << "empty newHttpClient called";
}

HttpClientPtr clientString(const std::string &, trantor::EventLoop *, bool)
{
    assert(false);
    LOG_ERROR << "empty newHttpClient called";
}
WebSocketClientPtr wsClientIpPort(const std::string &,
                                  uint16_t,
                                  bool,
                                  trantor::EventLoop *,
                                  bool)
{
    assert(false);
    LOG_ERROR << "empty newWebSocketClient called";
}

WebSocketClientPtr wsClientString(const std::string &,
                                  trantor::EventLoop *,
                                  bool)
{
    assert(false);
    LOG_ERROR << "empty newWebSocketClient called";
}

}  // namespace

HttpOperation HttpOperation::defaultOperation(emptyResponse,
                                              emptyResponse,
                                              json,
                                              jsonLV,
                                              httpView,
                                              redirect,
                                              file,
                                              httpReq,
                                              clientIpPort,
                                              clientString,
                                              wsClientIpPort,
                                              wsClientString);

HttpOperation::HttpOperation(
    const std::function<HttpResponsePtr()> &http,
    const std::function<HttpResponsePtr()> &notfound,
    const std::function<HttpResponsePtr(const Json::Value &)> &json,
    const std::function<HttpResponsePtr(Json::Value &&)> &jsonLv,
    const std::function<HttpResponsePtr(const std::string &,
                                        const HttpViewData &)> &httpView,
    const std::function<HttpResponsePtr(const std::string &, HttpStatusCode)>
        &redirection,
    const std::function<HttpResponsePtr(const std::string &,
                                        const std::string &,
                                        ContentType)> &file,
    const std::function<HttpRequestPtr()> &httpReq,
    const std::function<
        HttpClientPtr(const std::string &, uint16_t, bool, EvLoop *, bool)>
        &clientIPPort,
    const std::function<HttpClientPtr(const std::string &, EvLoop *, bool)>
        &clientString,
    const std::function<
        WebSocketClientPtr(const std::string &, uint16_t, bool, EvLoop *, bool)>
        &wsClientIPPort,
    const std::function<WebSocketClientPtr(const std::string &, EvLoop *, bool)>
        &wsClientString)
    : http_(http),
      notFound_(notfound),
      json_(json),
      jsonLV_(jsonLv),
      httpView_(httpView),
      redirect_(redirection),
      file_(file),
      httpRequest_(httpReq),
      newClientIPPort_(clientIPPort),
      newClientString_(clientString),
      newWsClientIPPort_(wsClientIPPort),
      newWsClientString_(wsClientString)
{
#ifndef NDEBUG
    init = false;
#endif
}

using namespace std::placeholders;
using std::bind;

HttpOperation *HttpOperation::createInstance(HttpAppFrameworkImpl *app)
{
    HttpOperation *op = new HttpOperation;
    op->http_ = bind(&HttpResponse::newHttpResponse, app);
    op->notFound_ = bind(&HttpResponse::newNotFoundResponse, app);

    op->redirect_ = bind(&HttpResponse::newRedirectionResponse, app, _1, _2);
    op->file_ = bind(&HttpResponse::newFileResponse, app, _1, _2, _3);

    op->httpView_ = bind(&HttpResponse::newHttpViewResponse, app, _1, _2);

    op->json_ = [app](const Json::Value &value) {
        return HttpResponse::newHttpJsonResponse(app, value);
    };

    op->jsonLV_ = [app](Json::Value &&value) {
        return HttpResponse::newHttpJsonResponse(app, std::move(value));
    };

    op->httpRequest_ = std::bind(HttpRequest::newHttpRequest, app);

    op->newClientIPPort_ =
        [app](const std::string &h, uint16_t p, bool u, EvLoop *l, bool o) {
            return HttpClient::newHttpClient(h, p, app, u, l, o);
        };

    op->newClientString_ = [app](const std::string &s, EvLoop *l, bool o) {
        return HttpClient::newHttpClient(s, app, l, o);
    };

    op->newWsClientIPPort_ =
        [app](const std::string &h, uint16_t p, bool u, EvLoop *l, bool o) {
            return WebSocketClient::newWebSocketClient(h, p, app, u, l, o);
        };

    op->newWsClientString_ = [app](const std::string &s, EvLoop *l, bool o) {
        return WebSocketClient::newWebSocketClient(s, app, l, o);
    };

    op->loop_ = app->getLoop();

#ifndef NDEBUG
    op->init = true;
#endif
    op->app_ = app;
    return op;
}

HttpOperation::HttpOperation(HttpOperation &&another)
{
    std::swap(http_, another.http_);
    std::swap(notFound_, another.notFound_);
    std::swap(json_, another.json_);
    std::swap(jsonLV_, another.jsonLV_);
    std::swap(redirect_, another.redirect_);
    std::swap(httpView_, another.httpView_);
    std::swap(file_, another.file_);

    std::swap(httpRequest_, another.httpRequest_);
    std::swap(newClientIPPort_, another.newClientIPPort_);
    std::swap(newClientString_, another.newClientString_);
    std::swap(newWsClientIPPort_, another.newWsClientIPPort_);
    std::swap(newWsClientString_, another.newWsClientString_);
#ifndef NDEBUG
    std::swap(init, another.init);
#endif
}