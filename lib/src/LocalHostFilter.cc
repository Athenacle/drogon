/**
 *
 *  LocalHostFilter.cc
 *  An Tao
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  https://github.com/an-tao/drogon
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Drogon
 *
 */

#include "HttpResponseImpl.h"
#include <drogon/LocalHostFilter.h>
using namespace drogon;
void LocalHostFilter::doFilter(const HttpRequestPtr &req,
                               const HttpOperation &op,
                               FilterCallback &&fcb,
                               FilterChainCallback &&fccb)
{
    if (req->peerAddr().isLoopbackIp())
    {
        fccb();
        return;
    }
    auto res = op.newNotFoundResponse();
    fcb(res);
}
