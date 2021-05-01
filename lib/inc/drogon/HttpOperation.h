
#pragma once

#include <memory>
#include <functional>
#include <drogon/HttpTypes.h>
#include <drogon/HttpViewData.h>

namespace Json
{
class Value;
}

namespace trantor
{
class EventLoop;
}
namespace drogon
{
class HttpAppFrameworkImpl;
class HttpResponse;
using HttpResponsePtr = std::shared_ptr<HttpResponse>;
class HttpRequest;
using HttpRequestPtr = std::shared_ptr<HttpRequest>;
class WebSocketClient;
using WebSocketClientPtr = std::shared_ptr<WebSocketClient>;
class HttpClient;
using HttpClientPtr = std::shared_ptr<HttpClient>;
class HttpOperation
{
    friend class HttpAppFramework;
    friend class HttpAppFrameworkImpl;

#ifndef NDEBUG
    bool init{false};
#endif

  private:
    using EvLoop = trantor::EventLoop;

    EvLoop *loop_{nullptr};
    HttpAppFrameworkImpl *app_;

    static HttpOperation *createInstance(HttpAppFrameworkImpl *);

    HttpOperation() = default;
    HttpOperation(HttpOperation &&);
    ~HttpOperation() = default;

    std::function<HttpResponsePtr(const Json::Value &)> json_;

    std::function<HttpResponsePtr(Json::Value &&)> jsonLV_;

    std::function<HttpResponsePtr(const std::string &, HttpStatusCode)>
        redirect_;

    std::function<HttpResponsePtr()> http_;

    std::function<HttpResponsePtr()> notFound_;

    std::function<HttpResponsePtr(HttpRequestPtr &)> notFound__;

    std::function<HttpResponsePtr(const std::string &, const HttpViewData &)>
        httpView_;

    std::function<
        HttpResponsePtr(const std::string &, const std::string &, ContentType)>
        file_;

    std::function<HttpRequestPtr()> httpRequest_;

    std::function<
        HttpClientPtr(const std::string &, uint16_t, bool, EvLoop *, bool)>
        newClientIPPort_;

    std::function<HttpClientPtr(const std::string &, EvLoop *, bool)>
        newClientString_;

    std::function<
        WebSocketClientPtr(const std::string &, uint16_t, bool, EvLoop *, bool)>
        newWsClientIPPort_;

    std::function<WebSocketClientPtr(const std::string &, EvLoop *, bool)>
        newWsClientString_;

  public:
    HttpOperation &operator=(HttpOperation &&);

    WebSocketClientPtr newWebSocketClient(const std::string &hostString,
                                          trantor::EventLoop *loop = nullptr,
                                          bool useOldTLS = false) const
    {
        return newWsClientString_(hostString, loop, useOldTLS);
    }

    WebSocketClientPtr newWebSocketClient(const std::string &ip,
                                          uint16_t port,
                                          bool useSSL = false,
                                          EvLoop *loop = nullptr,
                                          bool useOldTLS = false) const
    {
        return newWsClientIPPort_(ip, port, useSSL, loop, useOldTLS);
    }

    HttpClientPtr newHttpClient(const std::string &hostString,
                                EvLoop *loop = nullptr,
                                bool useOldTLS = false) const
    {
        return newClientString_(hostString, loop, useOldTLS);
    }

    HttpRequestPtr newHttpRequest() const
    {
        return httpRequest_();
    }

    HttpClientPtr newHttpClient(const std::string &ip,
                                uint16_t port,
                                bool useSSL = false,
                                EvLoop *loop = nullptr,
                                bool useOldTLS = false) const
    {
        return newClientIPPort_(ip, port, useSSL, loop, false);
    }

    HttpResponsePtr newFileResponse(const std::string &fullPath,
                                    const std::string &attachmentFileName = "",
                                    ContentType ct = CT_NONE) const
    {
        return file_(fullPath, attachmentFileName, ct);
    }
    HttpResponsePtr newHttpViewResponse(const std::string &s1,
                                        const HttpViewData &view) const
    {
        return httpView_(s1, view);
    }

    HttpResponsePtr newHttpResponse() const
    {
        return http_();
    }

    HttpResponsePtr newNotFoundResponse() const
    {
        return notFound_();
    }

    HttpResponsePtr newHttpJsonResponse(const Json::Value &value) const
    {
        return json_(value);
    }

    HttpResponsePtr newHttpJsonResponse(Json::Value &&value) const
    {
        return jsonLV_(std::move(value));
    }

    HttpResponsePtr newRedirectionResponse(const std::string &loc,
                                           HttpStatusCode sc = k302Found) const
    {
        return redirect_(loc, sc);
    }

    HttpOperation(
        const std::function<HttpResponsePtr()> &http,
        const std::function<HttpResponsePtr()> &notfound,
        const std::function<HttpResponsePtr(const Json::Value &)> &json,
        const std::function<HttpResponsePtr(Json::Value &&)> &jsonLv,
        const std::function<HttpResponsePtr(const std::string &,
                                            const HttpViewData &)> &httpView,
        const std::function<HttpResponsePtr(const std::string &,
                                            HttpStatusCode)> &redirection,
        const std::function<HttpResponsePtr(const std::string &,
                                            const std::string &,
                                            ContentType)> &file,
        const std::function<HttpRequestPtr()> &httpReq,
        const std::function<
            HttpClientPtr(const std::string &, uint16_t, bool, EvLoop *, bool)>
            &clientIPPort,

        const std::function<HttpClientPtr(const std::string &, EvLoop *, bool)>
            &clientString,
        const std::function<WebSocketClientPtr(const std::string &,
                                               uint16_t,
                                               bool,
                                               EvLoop *,
                                               bool)> &wsClientIPPort,
        const std::function<WebSocketClientPtr(const std::string &,
                                               EvLoop *,
                                               bool)> &wsClientString);

    trantor::EventLoop *getLoop() const
    {
        return loop_;
    }

    auto getApp() const
    {
        return app_;
    }

    static HttpOperation defaultOperation;
};
}  // namespace drogon