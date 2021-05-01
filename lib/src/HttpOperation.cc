
#include <drogon/HttpOperation.h>
#include <trantor/utils/Logger.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpClient.h>
#include <drogon/WebSocketClient.h>
#include "HttpAppFrameworkImpl.h"

using namespace drogon;

#if __has_cpp_attribute(noreturn)
#define NORETURN [[noreturn]]
#else
#define NORETURN
#define NO_NORETURN
#endif

namespace
{
#ifdef NO_NORETURN
auto empty()
{
    static std::shared_ptr<HttpResponse> resp;
    return resp;
}
#endif

NORETURN
HttpResponsePtr emptyResponse()
{
    std::abort();
    LOG_ERROR << "empty newHttpResponse called";
#ifdef NO_NORETURN
    return empty();
#endif
}

NORETURN
HttpResponsePtr json(const Json::Value &)
{
    std::abort();
    LOG_ERROR << "empty newHttpResponse called";
#ifdef NO_NORETURN
    return empty();
#endif
}

NORETURN
HttpResponsePtr jsonLV(Json::Value &&)
{
    std::abort();
    LOG_ERROR << "empty newJsonLVResponse called";
#ifdef NO_NORETURN
    return empty();
#endif
}

NORETURN
HttpResponsePtr httpView(const std::string &, const HttpViewData &)
{
    std::abort();
    LOG_ERROR << "empty newHttpView called";
#ifdef NO_NORETURN
    return empty();
#endif
}

NORETURN
HttpResponsePtr redirect(const std::string &, HttpStatusCode)
{
    std::abort();
    LOG_ERROR << "empty newRedirection called";
#ifdef NO_NORETURN
    return empty();
#endif
}

NORETURN
HttpResponsePtr file(const std::string &, const std::string &, ContentType)
{
    std::abort();
    LOG_ERROR << "empty newFileResponse called";
#ifdef NO_NORETURN
    return empty();
#endif
}

NORETURN
HttpRequestPtr httpReq()
{
    std::abort();
    LOG_ERROR << "empty newHttpRequest called";
#ifdef NO_NORETURN
    return std::shared_ptr<HttpRequest>();
#endif
}

NORETURN
HttpClientPtr clientIpPort(const std::string &,
                           uint16_t,
                           bool,
                           trantor::EventLoop *,
                           bool)
{
    std::abort();
    LOG_ERROR << "empty newHttpClient called";
#ifdef NO_NORETURN
    return std::shared_ptr<HttpClient>();
#endif
}

NORETURN
HttpClientPtr clientString(const std::string &, trantor::EventLoop *, bool)
{
    std::abort();
    LOG_ERROR << "empty newHttpClient called";
#ifdef NO_NORETURN
    return std::shared_ptr<HttpClient>();
#endif
}

NORETURN
WebSocketClientPtr wsClientIpPort(const std::string &,
                                  uint16_t,
                                  bool,
                                  trantor::EventLoop *,
                                  bool)
{
    std::abort();
    LOG_ERROR << "empty newWebSocketClient called";
#ifdef NO_NORETURN
    return std::shared_ptr<WebSocketClient>();
#endif
}

NORETURN
WebSocketClientPtr wsClientString(const std::string &,
                                  trantor::EventLoop *,
                                  bool)
{
    std::abort();
    LOG_ERROR << "empty newWebSocketClient called";
#ifdef NO_NORETURN
    return std::shared_ptr<WebSocketClient>();
#endif
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
    :

      json_(json),
      jsonLV_(jsonLv),
      redirect_(redirection),
      http_(http),
      notFound_(notfound),
      httpView_(httpView),
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

static HttpResponsePtr emptyResponse(nullptr);
static HttpRequestPtr emptyRequest(nullptr);

HttpOperation *HttpOperation::createInstance(HttpAppFrameworkImpl *app)
{
    HttpOperation *op = new HttpOperation;
    op->http_ = bind(&HttpResponse::newHttpResponse, app);

    op->notFound__ = [app](HttpRequestPtr &req) {
        return HttpResponse::newNotFoundResponse(app, req);
    };
    op->notFound_ = [app]() {
        return HttpResponse::newNotFoundResponse(app, emptyRequest);
    };

    op->redirect_ = bind(&HttpResponse::newRedirectionResponse, app, _1, _2);
    op->file_ = [app](const std::string &a,
                      const std::string &b,
                      drogon::ContentType ct) {
        return HttpResponse::newFileResponse(app, emptyRequest, a, b, ct);
    };

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