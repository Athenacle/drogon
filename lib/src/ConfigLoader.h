/**
 *
 *  ConfigLoader.h
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

#pragma once

#include <json/json.h>
#include <string>
#include <trantor/utils/NonCopyable.h>

namespace drogon
{
class HttpAppFrameworkImpl;
class ConfigLoader : public trantor::NonCopyable
{
  public:
    explicit ConfigLoader(HttpAppFrameworkImpl *app,
                          const std::string &configFile);
    explicit ConfigLoader(HttpAppFrameworkImpl *app, const Json::Value &data);
    explicit ConfigLoader(HttpAppFrameworkImpl *app, Json::Value &&data);
    ~ConfigLoader();
    const Json::Value &jsonValue() const
    {
        return configJsonRoot_;
    }
    void load();

  private:
    HttpAppFrameworkImpl *app_;
    std::string configFile_;
    Json::Value configJsonRoot_;
};
}  // namespace drogon
