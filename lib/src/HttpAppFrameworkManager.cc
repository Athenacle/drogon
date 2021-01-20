
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

void HttpAppFrameworkManager::destroyAppInstance(
    const std::shared_ptr<HttpAppFramework> &impl)
{
    std::lock_guard<std::mutex> lock(lock_);
    auto iter = std::find(apps_.cbegin(), apps_.cend(), impl);
    assert(iter != apps_.end());
    apps_.erase(iter);
}
void HttpAppFrameworkManager::loopAppFramework(
    const std::function<void(HttpAppFramework *)> &fn)
{
    std::lock_guard<std::mutex> lock(lock_);
    std::for_each(apps_.begin(),
                  apps_.end(),
                  [&fn](std::shared_ptr<HttpAppFramework> &ptr) {
                      fn(ptr.get());
                  });
}

HttpAppFrameworkManager::~HttpAppFrameworkManager()
{
    LOG_TRACE << apps_.size() << " instance of HttpAppFramework not freed";
    apps_.clear();
    autoCreationHandlerRegistor_.clear();
}