
#include <drogon/HttpAppFrameworkManager.h>

using namespace drogon;

void HttpAppFrameworkManager::registerAutoCreationHandlers(
    HttpAppFramework *app)
{
    for (const auto &fun : autoCreationHandlerRegistor_)
    {
        fun(app);
    }
}
