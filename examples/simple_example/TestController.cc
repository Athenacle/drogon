#include "TestController.h"
using namespace example;
void TestController::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    const HttpOperation &op,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    // write your application logic here
    LOG_WARN << req->matchedPathPatternData();
    LOG_DEBUG << "index=" << threadIndex_.getThreadData();
    ++(threadIndex_.getThreadData());
    auto resp = op.newHttpResponse();
    resp->setContentTypeCodeAndCustomString(CT_TEXT_PLAIN,
                                            "Content-Type: plaintext\r\n");
    resp->setBody("<p>Hello, world!</p>");
    resp->setExpiredTime(20);
    callback(resp);
}
