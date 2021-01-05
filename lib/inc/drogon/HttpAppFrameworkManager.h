#pragma once

#include "HttpAppFramework.h"

namespace drogon
{
class HttpAppFrameworkManager
{
    friend class drogon::HttpAppFrameworkImpl;
    friend class drogon::HttpAppFramework;

    std::vector<HttpAppFramework *> apps_;
    std::vector<std::function<void(HttpAppFramework *)>>
        autoCreationHandlerRegistor_;
    HttpAppFrameworkManager() = default;
    ~HttpAppFrameworkManager() = default;

    void registerAppInstance(HttpAppFramework *app)
    {
        apps_.emplace_back(app);
    }
    void stopAppInstance(HttpAppFramework *app);

  public:
    static HttpAppFrameworkManager &instance()
    {
        static HttpAppFrameworkManager manager;
        return manager;
    }
    void pushAutoCreationFunction(
        const std::function<void(HttpAppFramework *)> &func)
    {
        autoCreationHandlerRegistor_.emplace_back(func);
    }
    void registerAutoCreationHandlers(HttpAppFramework *app);
};

}  // namespace drogon