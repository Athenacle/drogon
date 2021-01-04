#include <drogon/drogon.h>
#include <thread>
#include <chrono>

/**
 * @brief This test program tests to call the app().run() in another thread.
 *
 */
int main()
{
    auto app = reinterpret_cast<drogon::HttpAppFramework*>(drogon::create());
    std::thread([app]() {
        app->getLoop()->runEvery(1, []() { std::cout << "!" << std::endl; });
    }).detach();

    std::thread l([app]() { app->run(); });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    trantor::EventLoop loop;
    l.join();
}