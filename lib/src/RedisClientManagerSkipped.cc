/**
 *
 *  RedisClientManagerSkipped.cc
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

#include "RedisClientManager.h"
#include <drogon/config.h>
#include <drogon/utils/Utilities.h>
#include <algorithm>
#include <cstdlib>

using namespace drogon::nosql;
using namespace drogon;

void RedisClientManager::createRedisClients(
    const std::vector<trantor::EventLoop *> &)
{
    return;
}

void RedisClientManager::createRedisClient(const std::string &,
                                           const std::string &,
                                           unsigned short,
                                           const std::string &,
                                           size_t,
                                           bool)
{
    LOG_FATAL << "Redis is not supported by drogon, please install the "
                 "hiredis library first.";
    abort();
}

// bool RedisClientManager::areAllRedisClientsAvailable() const noexcept
// {
//     LOG_FATAL << "Redis is supported by drogon, please install the "
//                  "hiredis library first.";
//     abort();
// }