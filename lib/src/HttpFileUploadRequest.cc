/**
 *
 *  HttpFileUploadRequest.cc
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

#include "HttpFileUploadRequest.h"
#include <drogon/UploadFile.h>
#include <drogon/utils/Utilities.h>

using namespace drogon;

HttpFileUploadRequest::HttpFileUploadRequest(
    HttpAppFrameworkImpl* app_,
    const std::vector<UploadFile>& files)
    : HttpRequestImpl(app_, nullptr),
      boundary_(utils::genRandomString(32)),
      files_(files)
{
    setMethod(drogon::Post);
    setVersion(drogon::Version::kHttp11);
    setContentType("content-type: multipart/form-data; boundary=" + boundary_ +
                   "\r\n");
    contentType_ = CT_MULTIPART_FORM_DATA;
}