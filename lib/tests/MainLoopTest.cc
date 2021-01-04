#include <drogon/drogon.h>
#include <thread>

int main()
{
    auto app = reinterpret_cast<drogon::HttpAppFramework*>(drogon::create());

    std::thread([app]() {
        app->getLoop()->runEvery(1, []() { std::cout << "!" << std::endl; });
    }).detach();
    app->run();
}