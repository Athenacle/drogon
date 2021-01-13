#include <drogon/drogon.h>
int main()
{
    // Set HTTP listener address and port
    drogon::create()
        ->loadConfigFile("../config.json")
        // Run HTTP framework,the method will block in the internal event loop
        .run();
    return 0;
}
