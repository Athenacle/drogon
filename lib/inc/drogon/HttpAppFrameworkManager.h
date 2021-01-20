#pragma once

#include "HttpAppFramework.h"

namespace drogon
{
class HttpAppFrameworkManager
{
    friend class drogon::HttpAppFrameworkImpl;
    friend class drogon::HttpAppFramework;

    std::vector<std::shared_ptr<HttpAppFramework>> apps_;
    std::vector<std::function<void(HttpAppFramework *)>>
        autoCreationHandlerRegistor_;
    std::mutex lock_;
    HttpAppFrameworkManager() = default;
    ~HttpAppFrameworkManager();

    void registerAppInstance(const std::shared_ptr<HttpAppFramework> &app)
    {
        std::lock_guard<std::mutex> lock(lock_);
        apps_.emplace_back(app);
    }
    void destroyAppInstance(const std::shared_ptr<HttpAppFramework> &impl);

    void stopAppInstance(HttpAppFramework *app);

  public:
    void loopAppFramework(const std::function<void(HttpAppFramework *)> &);

    static HttpAppFrameworkManager &instance()
    {
        static HttpAppFrameworkManager manager;
        return manager;
    }
    void pushAutoCreationFunction(
        const std::function<void(HttpAppFramework *)> &func)
    {
        std::lock_guard<std::mutex> lock(lock_);
        autoCreationHandlerRegistor_.emplace_back(func);
    }
    void registerAutoCreationHandlers(HttpAppFramework *app);
};

}  // namespace drogon