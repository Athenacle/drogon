
#include <drogon/HttpAppFrameworkManager.h>
#include <trantor/utils/Logger.h>

using namespace drogon;

void HttpAppFrameworkManager::registerAutoCreationHandlers(
    HttpAppFramework *app)
{
    std::lock_guard<std::mutex> lock(lock_);
    for (const auto &fun : autoCreationHandlerRegistor_)
    {
        fun(app);
    }
}

void HttpAppFrameworkManager::destroyAppInstance(HttpAppFramework *impl)
{
    std::lock_guard<std::mutex> lock(lock_);
    auto iter = std::find(apps_.cbegin(), apps_.cend(), impl);
    assert(iter != apps_.end());
    delete *iter;
    apps_.erase(iter);
}
void HttpAppFrameworkManager::loopAppFramework(
    const std::function<void(HttpAppFramework *)> &fn)
{
    std::lock_guard<std::mutex> lock(lock_);
    std::for_each(apps_.cbegin(), apps_.cend(), fn);
}

HttpAppFrameworkManager::~HttpAppFrameworkManager()
{
    LOG_TRACE << apps_.size() << " instance of HttpAppFramework not freed";
    std::lock_guard<std::mutex> lock(lock_);
    for (auto iter : apps_)
    {
        delete iter;
    }
}