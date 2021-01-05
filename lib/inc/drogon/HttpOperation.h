
#pragma once

#include <drogon/HttpResponse.h>
#include <trantor/net/EventLoop.h>

namespace Json
{
class Value;
}

namespace drogon
{
class HttpOperation
{
    friend class HttpAppFramework;
    friend class HttpAppFrameworkImpl;

#ifndef NDEBUG
    bool init{false};
#endif

  private:
    trantor::EventLoop *loop_{nullptr};
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

    std::function<HttpResponsePtr(const std::string &, const HttpViewData &)>
        httpView_;

    std::function<
        HttpResponsePtr(const std::string &, const std::string &, ContentType)>
        file_;

  public:
    HttpOperation &operator=(HttpOperation &&);

    HttpResponsePtr newFileResponse(const std::string &fullPath,
                                    const std::string &attachmentFileName,
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
                                            ContentType)> &file);

    trantor::EventLoop *getLoop() const
    {
        return loop_;
    }

    static HttpOperation defaultOperation;
};
}  // namespace drogon