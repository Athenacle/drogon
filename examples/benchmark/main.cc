#include <drogon/drogon.h>

using namespace drogon;
int main()
{
    auto app = drogon::create();
    auto &op = app->getOperations();
    app->setLogLevel(trantor::Logger::kTrace)
        .addListener("0.0.0.0", 7770)
        .setThreadNum(0)
        .registerSyncAdvice(
            [&op](const HttpRequestPtr &req) -> HttpResponsePtr {
                const auto &path = req->path();
                if (path.length() == 1 && path[0] == '/')
                {
                    auto response = op.newHttpResponse();
                    response->setBody("<p>Hello, world!</p>");
                    return response;
                }
                return nullptr;
            })
        .run();
}
