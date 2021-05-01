#include "CustomCtrl.h"
// add definition of your processing function here

void CustomCtrl::hello(const HttpRequestPtr &,
                       const HttpOperation &op,
                       std::function<void(const HttpResponsePtr &)> &&callback,
                       const std::string &userName) const
{
    auto resp = op.newHttpResponse();
    resp->setBody("<P>" + greetings_ + ", " + userName + "</P>");
    callback(resp);
}