/**
 *
 *  CustomHeaderFilter.cc
 *
 */

#include "CustomHeaderFilter.h"

using namespace drogon;

void CustomHeaderFilter::doFilter(const HttpRequestPtr &req,
                                  FilterCallback &&fcb,
                                  FilterChainCallback &&fccb)
{
    if (req->getHeader(field_) == value_)
    {
        // Passed
        fccb();
        return;
    }
    // Check failed
    auto res = drogon::HttpResponse::newHttpResponse(req->getApp());
    res->setStatusCode(k500InternalServerError);
    fcb(res);
}
