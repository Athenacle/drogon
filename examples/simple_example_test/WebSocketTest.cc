#include <drogon/WebSocketClient.h>
#include <drogon/HttpAppFramework.h>
#include <trantor/net/EventLoopThread.h>

#include <iostream>

using namespace drogon;
using namespace std::chrono_literals;

int main(int argc, char *argv[])
{
    auto app = drogon::create();
    auto appp = reinterpret_cast<HttpAppFramework *>(app);
    auto wsPtr = WebSocketClient::newWebSocketClient("127.0.0.1", 8848, app);
    auto req = HttpRequest::newHttpRequest(app);
    req->setPath("/chat");
    bool continually = true;
    if (argc > 1)
    {
        if (std::string(argv[1]) == "-t")
            continually = false;
        else if (std::string(argv[1]) == "-p")
        {
            // Connect to a public web socket server.
            wsPtr =
                WebSocketClient::newWebSocketClient("wss://echo.websocket.org",
                                                    app);
            req->setPath("/");
        }
    }
    wsPtr->setMessageHandler(
        [continually, appp](const std::string &message,
                            const WebSocketClientPtr &wsPtr,
                            const WebSocketMessageType &type) {
            std::cout << "new message:" << message << std::endl;
            if (type == WebSocketMessageType::Pong)
            {
                std::cout << "recv a pong" << std::endl;
                if (!continually)
                {
                    appp->getLoop()->quit();
                }
            }
        });
    wsPtr->setConnectionClosedHandler([](const WebSocketClientPtr &wsPtr) {
        std::cout << "ws closed!" << std::endl;
    });
    wsPtr->connectToServer(req,
                           [continually](ReqResult r,
                                         const HttpResponsePtr &resp,
                                         const WebSocketClientPtr &wsPtr) {
                               if (r == ReqResult::Ok)
                               {
                                   std::cout << "ws connected!" << std::endl;
                                   wsPtr->getConnection()->setPingMessage("",
                                                                          2s);
                                   wsPtr->getConnection()->send("hello!");
                               }
                               else
                               {
                                   std::cout << "ws failed!" << std::endl;
                                   if (!continually)
                                   {
                                       exit(1);
                                   }
                               }
                           });
    appp->getLoop()->runAfter(5.0, [continually]() {
        if (!continually)
        {
            exit(1);
        }
    });
    appp->setLogLevel(trantor::Logger::kTrace);
    appp->run();
}