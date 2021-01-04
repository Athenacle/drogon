//
// Created by antao on 2018/5/22.
//

#include "TimeFilter.h"
#define VDate "visitDate"
void TimeFilter::doFilter(const HttpRequestPtr &req,
                          FilterCallback &&cb,
                          FilterChainCallback &&ccb)
{
    trantor::Date now = trantor::Date::date();
    if (!req->session())
    {
        // no session support by framework,pls enable session
        auto resp = HttpResponse::newNotFoundResponse(req->getApp());
        cb(resp);
        return;
    }
    auto lastDate = req->session()->getOptional<trantor::Date>(VDate);
    if (lastDate)
    {
        LOG_TRACE << "last:" << lastDate.value().toFormattedString(false);
        req->session()->modify<trantor::Date>(VDate,
                                              [now](trantor::Date &vdate) {
                                                  vdate = now;
                                              });
        LOG_TRACE << "update visitDate";
        if (now > lastDate.value().after(10))
        {
            // 10 sec later can visit again;
            ccb();
            return;
        }
        else
        {
            Json::Value json;
            json["result"] = "error";
            json["message"] = "Access interval should be at least 10 seconds";
            auto res = HttpResponse::newHttpJsonResponse(req->getApp(), json);
            cb(res);
            return;
        }
    }
    LOG_TRACE << "first visit,insert visitDate";
    req->session()->insert(VDate, now);
    ccb();
}