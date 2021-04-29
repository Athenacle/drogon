/**
 *
 *  @file HttpResponseImpl.cc
 *  @author An Tao
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  https://github.com/an-tao/drogon
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Drogon
 *
 */

#include "HttpResponseImpl.h"
#include "HttpAppFrameworkImpl.h"
#include "HttpUtils.h"
#include <drogon/HttpViewData.h>
#include <drogon/IOThreadStorage.h>
#include <fstream>
#include <memory>
#include <stdio.h>
#include <sys/stat.h>
#include <trantor/utils/Logger.h>
#ifdef _WIN32
#define stat _stati64
#endif
using namespace trantor;
using namespace drogon;

namespace drogon
{
// "Fri, 23 Aug 2019 12:58:03 GMT" length = 29
static const size_t httpFullDateStringLength = 29;
static inline void doResponseCreateAdvices(
    HttpAppFrameworkImpl *app,
    const HttpResponseImplPtr &responsePtr)
{
    auto &advices = app->getResponseCreationAdvices();
    if (!advices.empty())
    {
        for (auto &advice : advices)
        {
            advice(responsePtr);
        }
    }
}
static inline HttpResponsePtr genHttpResponse(HttpAppFrameworkImpl *app,
                                              const std::string &viewName,
                                              const HttpViewData &data)
{
    auto templ = DrTemplateBase::newTemplate(viewName);
    if (templ)
    {
        auto res = HttpResponse::newHttpResponse(app);
        res->setBody(templ->genText(data));
        return res;
    }
    return drogon::HttpResponse::newNotFoundResponse(app);
}
}  // namespace drogon

HttpResponsePtr HttpResponse::newHttpResponse(HttpAppFrameworkImpl *app)
{
    auto res = std::make_shared<HttpResponseImpl>(k200OK, CT_TEXT_HTML);
    res->app_ = app;
    doResponseCreateAdvices(app, res);
    return res;
}

HttpResponsePtr HttpResponse::newHttpJsonResponse(HttpAppFrameworkImpl *app,
                                                  const Json::Value &data)
{
    auto res = std::make_shared<HttpResponseImpl>(k200OK, CT_APPLICATION_JSON);
    res->setJsonObject(data);
    res->app_ = app;
    doResponseCreateAdvices(app, res);
    return res;
}

HttpResponsePtr HttpResponse::newHttpJsonResponse(HttpAppFrameworkImpl *app,
                                                  Json::Value &&data)
{
    auto res = std::make_shared<HttpResponseImpl>(k200OK, CT_APPLICATION_JSON);
    res->setJsonObject(std::move(data));
    res->app_ = app;
    doResponseCreateAdvices(app, res);
    return res;
}

void HttpResponseImpl::generateBodyFromJson() const
{
    if (!jsonPtr_ || flagForSerializingJson_)
    {
        return;
    }
    flagForSerializingJson_ = true;
    static std::once_flag once;
    static Json::StreamWriterBuilder builder;
    std::call_once(once, [this]() {
        builder["commentStyle"] = "None";
        builder["indentation"] = "";
        if (app_->isUnicodeEscapingUsedInJson())
        {
            builder["emitUTF8"] = true;
        }
        auto &precision = app_->getFloatPrecisionInJson();
        if (precision.first != 0)
        {
            builder["precision"] = precision.first;
            builder["precisionType"] = precision.second;
        }
    });
    bodyPtr_ = std::make_shared<HttpMessageStringBody>(
        writeString(builder, *jsonPtr_));
}

HttpResponsePtr HttpResponse::newNotFoundResponse(HttpAppFrameworkImpl *app)
{
    auto loop = trantor::EventLoop::getEventLoopOfCurrentThread();
    auto &resp = app->getCustom404Page();
    if (resp)
    {
        if (loop && loop->index() < app->getThreadNum())
        {
            return resp;
        }
        else
        {
            auto ret = HttpResponsePtr{new HttpResponseImpl(
                *static_cast<HttpResponseImpl *>(resp.get()))};
            ret->app_ = app;
            return ret;
        }
    }
    else
    {
        if (loop && loop->index() < app->getThreadNum())
        {
            // If the current thread is an IO thread
            static std::once_flag threadOnce;
            static IOThreadStorage<HttpResponsePtr> thread404Pages(app);
            std::call_once(threadOnce, [app] {
                thread404Pages.init([app](drogon::HttpResponsePtr &resp,
                                          size_t index) {
                    if (app->isUsingCustomErrorHandler())
                    {
                        HttpRequestPtr ptr(nullptr);
                        resp =
                            app->getCustomErrorHandler()(k404NotFound,
                                                         ptr,
                                                         app->getOperations());
                        resp->setExpiredTime(0);
                    }
                    else
                    {
                        HttpViewData data;
                        data.insert("version", drogon::getVersion());
                        resp = HttpResponse::newHttpViewResponse(
                            app, "drogon::NotFound", data);
                        resp->setStatusCode(k404NotFound);
                        resp->setExpiredTime(0);
                    }
                });
            });
            LOG_TRACE << "Use cached 404 response";
            return thread404Pages.getThreadData();
        }
        else
        {
            if (app->isUsingCustomErrorHandler())
            {
                HttpRequestPtr ptr(nullptr);
                auto resp = app->getCustomErrorHandler()(k404NotFound,
                                                         ptr,
                                                         app->getOperations());
                return resp;
            }
            HttpViewData data;
            data.insert("version", drogon::getVersion());
            auto notFoundResp =
                HttpResponse::newHttpViewResponse(app,
                                                  "drogon::NotFound",
                                                  data);
            notFoundResp->setStatusCode(k404NotFound);
            return notFoundResp;
        }
    }
}
HttpResponsePtr HttpResponse::newRedirectionResponse(
    HttpAppFrameworkImpl *app,
    const std::string &location,
    HttpStatusCode status)
{
    auto res = std::make_shared<HttpResponseImpl>(app);
    res->setStatusCode(status);
    res->redirect(location);
    doResponseCreateAdvices(app, res);
    return res;
}

HttpResponsePtr HttpResponse::newHttpViewResponse(HttpAppFrameworkImpl *app,
                                                  const std::string &viewName,
                                                  const HttpViewData &data)
{
    auto ret = genHttpResponse(app, viewName, data);
    ret->app_ = app;
    return ret;
}

HttpResponsePtr HttpResponse::newFileResponse(
    HttpAppFrameworkImpl *app,
    const unsigned char *pBuffer,
    size_t bufferLength,
    const std::string &attachmentFileName,
    ContentType type)
{
    // Make Raw HttpResponse
    auto resp = std::make_shared<HttpResponseImpl>(app);

    // Set response body and length
    resp->setBody(
        std::string(reinterpret_cast<const char *>(pBuffer), bufferLength));

    // Set status of message
    resp->setStatusCode(k200OK);

    // Check for type and assign proper content type in header
    if (type != CT_NONE)
    {
        resp->setContentTypeCode(type);
    }
    else if (!attachmentFileName.empty())
    {
        resp->setContentTypeCode(drogon::getContentType(attachmentFileName));
    }
    else
    {
        resp->setContentTypeCode(
            CT_APPLICATION_OCTET_STREAM);  // default content-type for file;
    }

    // Add additional header values
    if (!attachmentFileName.empty())
    {
        resp->addHeader("Content-Disposition",
                        "attachment; filename=" + attachmentFileName);
    }

    // Finalize and return response
    doResponseCreateAdvices(app, resp);
    return resp;
}

HttpResponsePtr HttpResponse::newFileResponse(
    HttpAppFrameworkImpl *app,
    const std::string &fullPath,
    const std::string &attachmentFileName,
    ContentType type)
{
    std::ifstream infile(fullPath, std::ifstream::binary);
    LOG_TRACE << "send http file:" << fullPath;
    if (!infile)
    {
        auto resp = HttpResponse::newNotFoundResponse(app);
        return resp;
    }
    auto resp = std::make_shared<HttpResponseImpl>(app);
    resp->app_ = app;
    std::streambuf *pbuf = infile.rdbuf();
    std::streamsize filesize = pbuf->pubseekoff(0, infile.end);
    pbuf->pubseekoff(0, infile.beg);  // rewind
    if (app->useSendfile() && filesize > 1024 * 200)
    // TODO : Is 200k an appropriate value? Or set it to be configurable
    {
        // The advantages of sendfile() can only be reflected in sending large
        // files.
        resp->setSendfile(fullPath);
    }
    else
    {
        std::string str;
        str.resize(filesize);
        pbuf->sgetn(&str[0], filesize);
        resp->setBody(std::move(str));
    }
    resp->setStatusCode(k200OK);

    if (type == CT_NONE)
    {
        if (!attachmentFileName.empty())
        {
            resp->setContentTypeCode(
                drogon::getContentType(attachmentFileName));
        }
        else
        {
            resp->setContentTypeCode(drogon::getContentType(fullPath));
        }
    }
    else
    {
        resp->setContentTypeCode(type);
    }

    if (!attachmentFileName.empty())
    {
        resp->addHeader("Content-Disposition",
                        "attachment; filename=" + attachmentFileName);
    }
    doResponseCreateAdvices(app, resp);
    return resp;
}

void HttpResponseImpl::makeHeaderString(trantor::MsgBuffer &buffer)
{
    buffer.ensureWritableBytes(128);
    int len{0};
    if (version_ == Version::kHttp11)
    {
        len = snprintf(buffer.beginWrite(),
                       buffer.writableBytes(),
                       "HTTP/1.1 %d ",
                       statusCode_);
    }
    else
    {
        len = snprintf(buffer.beginWrite(),
                       buffer.writableBytes(),
                       "HTTP/1.0 %d ",
                       statusCode_);
    }
    buffer.hasWritten(len);

    if (!statusMessage_.empty())
        buffer.append(statusMessage_.data(), statusMessage_.length());
    buffer.append("\r\n");
    generateBodyFromJson();
    if (!passThrough_)
    {
        buffer.ensureWritableBytes(64);
        if (sendfileName_.empty())
        {
            auto bodyLength = bodyPtr_ ? bodyPtr_->length() : 0;
            len = snprintf(buffer.beginWrite(),
                           buffer.writableBytes(),
                           contentLengthFormatString<decltype(bodyLength)>(),
                           bodyLength);
        }
        else
        {
            struct stat filestat;
            if (stat(sendfileName_.data(), &filestat) < 0)
            {
                LOG_SYSERR << sendfileName_ << " stat error";
                return;
            }
            len = snprintf(
                buffer.beginWrite(),
                buffer.writableBytes(),
                contentLengthFormatString<decltype(filestat.st_size)>(),
                filestat.st_size);
        }
        buffer.hasWritten(len);
        if (headers_.find("connection") == headers_.end())
        {
            if (closeConnection_)
            {
                buffer.append("connection: close\r\n");
            }
            else if (version_ == Version::kHttp10)
            {
                buffer.append("connection: Keep-Alive\r\n");
            }
        }
        buffer.append(contentTypeString_.data(), contentTypeString_.length());
        if (app_->sendServerHeader())
        {
            buffer.append(app_->getServerHeaderString());
        }
    }

    for (auto it = headers_.begin(); it != headers_.end(); ++it)
    {
        buffer.append(it->first);
        buffer.append(": ");
        buffer.append(it->second);
        buffer.append("\r\n");
    }
}
void HttpResponseImpl::renderToBuffer(trantor::MsgBuffer &buffer)
{
    if (expriedTime_ >= 0)
    {
        auto strPtr = renderToBuffer();
        buffer.append(strPtr->peek(), strPtr->readableBytes());
        return;
    }

    if (!fullHeaderString_)
    {
        makeHeaderString(buffer);
    }
    else
    {
        buffer.append(*fullHeaderString_);
    }

    // output cookies
    if (cookies_.size() > 0)
    {
        for (auto it = cookies_.begin(); it != cookies_.end(); ++it)
        {
            buffer.append(it->second.cookieString());
        }
    }

    // output Date header
    if (!passThrough_ && app_->sendDateHeader())
    {
        buffer.append("date: ");
        buffer.append(utils::getHttpFullDate(trantor::Date::date()),
                      httpFullDateStringLength);
        buffer.append("\r\n\r\n");
    }
    else
    {
        buffer.append("\r\n");
    }
    if (bodyPtr_)
        buffer.append(bodyPtr_->data(), bodyPtr_->length());
}
std::shared_ptr<trantor::MsgBuffer> HttpResponseImpl::renderToBuffer()
{
    if (expriedTime_ >= 0)
    {
        if (!passThrough_ && app_->sendDateHeader())
        {
            if (datePos_ != static_cast<size_t>(-1))
            {
                auto now = trantor::Date::now();
                bool isDateChanged =
                    ((now.microSecondsSinceEpoch() / MICRO_SECONDS_PRE_SEC) !=
                     httpStringDate_);
                assert(httpString_);
                if (isDateChanged)
                {
                    httpStringDate_ =
                        now.microSecondsSinceEpoch() / MICRO_SECONDS_PRE_SEC;
                    auto newDate = utils::getHttpFullDate(now);

                    httpString_ =
                        std::make_shared<trantor::MsgBuffer>(*httpString_);
                    memcpy((void *)&(*httpString_)[datePos_],
                           newDate,
                           httpFullDateStringLength);
                    return httpString_;
                }

                return httpString_;
            }
        }
        else
        {
            if (httpString_)
                return httpString_;
        }
    }
    auto httpString = std::make_shared<trantor::MsgBuffer>(256);
    if (!fullHeaderString_)
    {
        makeHeaderString(*httpString);
    }
    else
    {
        httpString->append(*fullHeaderString_);
    }

    // output cookies
    if (cookies_.size() > 0)
    {
        for (auto it = cookies_.begin(); it != cookies_.end(); ++it)
        {
            httpString->append(it->second.cookieString());
        }
    }

    // output Date header
    if (!passThrough_ && app_->sendDateHeader())
    {
        httpString->append("date: ");
        auto datePos = httpString->readableBytes();
        httpString->append(utils::getHttpFullDate(trantor::Date::date()),
                           httpFullDateStringLength);
        httpString->append("\r\n\r\n");
        datePos_ = datePos;
    }
    else
    {
        httpString->append("\r\n");
    }

    LOG_TRACE << "reponse(no body):"
              << string_view{httpString->peek(), httpString->readableBytes()};
    if (bodyPtr_)
        httpString->append(bodyPtr_->data(), bodyPtr_->length());
    if (expriedTime_ >= 0)
    {
        httpString_ = httpString;
    }
    return httpString;
}

std::shared_ptr<trantor::MsgBuffer> HttpResponseImpl::
    renderHeaderForHeadMethod()
{
    auto httpString = std::make_shared<trantor::MsgBuffer>(256);
    if (!fullHeaderString_)
    {
        makeHeaderString(*httpString);
    }
    else
    {
        httpString->append(*fullHeaderString_);
    }

    // output cookies
    if (cookies_.size() > 0)
    {
        for (auto it = cookies_.begin(); it != cookies_.end(); ++it)
        {
            httpString->append(it->second.cookieString());
        }
    }

    // output Date header
    if (!passThrough_ && app_->sendDateHeader())
    {
        httpString->append("date: ");
        httpString->append(utils::getHttpFullDate(trantor::Date::date()),
                           httpFullDateStringLength);
        httpString->append("\r\n\r\n");
    }
    else
    {
        httpString->append("\r\n");
    }

    return httpString;
}

void HttpResponseImpl::addHeader(const char *start,
                                 const char *colon,
                                 const char *end)
{
    fullHeaderString_.reset();
    std::string field(start, colon);
    transform(field.begin(), field.end(), field.begin(), ::tolower);
    ++colon;
    while (colon < end && isspace(*colon))
    {
        ++colon;
    }
    std::string value(colon, end);
    while (!value.empty() && isspace(value[value.size() - 1]))
    {
        value.resize(value.size() - 1);
    }

    if (field == "set-cookie")
    {
        // LOG_INFO<<"cookies!!!:"<<value;
        auto values = utils::splitString(value, ";");
        Cookie cookie;
        cookie.setHttpOnly(false);
        for (size_t i = 0; i < values.size(); ++i)
        {
            std::string &coo = values[i];
            std::string cookie_name;
            std::string cookie_value;
            auto epos = coo.find('=');
            if (epos != std::string::npos)
            {
                cookie_name = coo.substr(0, epos);
                std::string::size_type cpos = 0;
                while (cpos < cookie_name.length() &&
                       isspace(cookie_name[cpos]))
                    ++cpos;
                cookie_name = cookie_name.substr(cpos);
                ++epos;
                while (epos < coo.length() && isspace(coo[epos]))
                    ++epos;
                cookie_value = coo.substr(epos);
            }
            else
            {
                std::string::size_type cpos = 0;
                while (cpos < coo.length() && isspace(coo[cpos]))
                    ++cpos;
                cookie_name = coo.substr(cpos);
            }
            if (i == 0)
            {
                cookie.setKey(cookie_name);
                cookie.setValue(cookie_value);
            }
            else
            {
                std::transform(cookie_name.begin(),
                               cookie_name.end(),
                               cookie_name.begin(),
                               tolower);
                if (cookie_name == "path")
                {
                    cookie.setPath(cookie_value);
                }
                else if (cookie_name == "domain")
                {
                    cookie.setDomain(cookie_value);
                }
                else if (cookie_name == "expires")
                {
                    cookie.setExpiresDate(utils::getHttpDate(cookie_value));
                }
                else if (cookie_name == "secure")
                {
                    cookie.setSecure(true);
                }
                else if (cookie_name == "httponly")
                {
                    cookie.setHttpOnly(true);
                }
            }
        }
        if (!cookie.key().empty())
        {
            cookies_[cookie.key()] = cookie;
        }
    }
    else
    {
        headers_[std::move(field)] = std::move(value);
    }
}

void HttpResponseImpl::swap(HttpResponseImpl &that) noexcept
{
    using std::swap;
    headers_.swap(that.headers_);
    cookies_.swap(that.cookies_);
    swap(statusCode_, that.statusCode_);
    swap(version_, that.version_);
    swap(statusMessage_, that.statusMessage_);
    swap(closeConnection_, that.closeConnection_);
    bodyPtr_.swap(that.bodyPtr_);
    swap(contentType_, that.contentType_);
    swap(flagForParsingContentType_, that.flagForParsingContentType_);
    swap(flagForParsingJson_, that.flagForParsingJson_);
    swap(sendfileName_, that.sendfileName_);
    jsonPtr_.swap(that.jsonPtr_);
    fullHeaderString_.swap(that.fullHeaderString_);
    httpString_.swap(that.httpString_);
    swap(datePos_, that.datePos_);
    swap(jsonParsingErrorPtr_, that.jsonParsingErrorPtr_);
    swap(app_, that.app_);
}

void HttpResponseImpl::clear()
{
    statusCode_ = kUnknown;
    version_ = Version::kHttp11;
    statusMessage_ = string_view{};
    fullHeaderString_.reset();
    jsonParsingErrorPtr_.reset();
    sendfileName_.clear();
    headers_.clear();
    cookies_.clear();
    bodyPtr_.reset();
    jsonPtr_.reset();
    expriedTime_ = -1;
    datePos_ = std::string::npos;
    flagForParsingContentType_ = false;
    flagForParsingJson_ = false;
}

void HttpResponseImpl::parseJson() const
{
    static std::once_flag once;
    static Json::CharReaderBuilder builder;
    std::call_once(once, []() { builder["collectComments"] = false; });
    JSONCPP_STRING errs;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (bodyPtr_)
    {
        jsonPtr_ = std::make_shared<Json::Value>();
        if (!reader->parse(bodyPtr_->data(),
                           bodyPtr_->data() + bodyPtr_->length(),
                           jsonPtr_.get(),
                           &errs))
        {
            LOG_ERROR << errs;
            LOG_ERROR << "body: " << bodyPtr_->getString();
            jsonPtr_.reset();
            jsonParsingErrorPtr_ =
                std::make_shared<std::string>(std::move(errs));
        }
        else
        {
            jsonParsingErrorPtr_.reset();
        }
    }
    else
    {
        jsonPtr_.reset();
        jsonParsingErrorPtr_ =
            std::make_shared<std::string>("empty response body");
    }
}

HttpResponseImpl::~HttpResponseImpl()
{
}

bool HttpResponseImpl::shouldBeCompressed() const
{
    if (!sendfileName_.empty() ||
        contentType() >= CT_APPLICATION_OCTET_STREAM ||
        getBody().length() < 1024 || !(getHeaderBy("content-encoding").empty()))
    {
        return false;
    }
    return true;
}
