#pragma once

#ifdef CONFIG_H
#include "drogon_config.h"
#endif

#ifndef MAYBE_UNUSED
#define MAYBE_UNUSED
#endif

#ifndef NORETURN
#define NORETURN
#endif

#ifndef likely
#define likely(expr) (((expr)))
#endif

#ifndef unlikely
#define unlikely(expr) (((expr)))
#endif

#include "gtest/gtest.h"
#include <drogon/HttpAppFramework.h>

using namespace drogon;

#include <unordered_map>
#include <thread>
#include <mutex>
#include <pthread.h>

using KVPairTable = std::unordered_map<std::string, std::string>;
using DataBlob = std::tuple<const uint8_t *, size_t>;

#define ROOT_BODY "<div><p>hello world!</p></div>"

class Barrier
{
    pthread_barrier_t *b;

    Barrier() = delete;
    Barrier(Barrier &) = delete;

  public:
    Barrier(Barrier &&other)
    {
        b = other.b;
        other.b = nullptr;
    }
    ~Barrier()
    {
        pthread_barrier_destroy(b);
        delete b;
    }

    Barrier(unsigned int count)
    {
        b = new pthread_barrier_t;
        pthread_barrier_init(b, nullptr, count);
    }

    void wait()
    {
        pthread_barrier_wait(b);
    }
};

class HttpApp : public ::testing::Test
{
    static int port;

    static KVPairTable emptyTable;

    std::thread appThread;
    std::mutex mutex;

    using ForwardReturnType = std::pair<ReqResult, HttpResponsePtr>;

  protected:
    std::shared_ptr<HttpAppFramework> app;

    static int getPort()
    {
        return port;
    }

    void SetUp() override;

    void start();

    void TearDown() override;

    ForwardReturnType forward(HttpMethod mtd,
                              const std::string &host,
                              const std::string &url,
                              const KVPairTable &query = emptyTable,
                              const KVPairTable &header = emptyTable);

    ForwardReturnType forward(HttpMethod mtd,
                              const std::string &host,
                              const std::string &url,
                              const DataBlob &blob,
                              ContentType type,
                              const KVPairTable &query = emptyTable,
                              const KVPairTable &header = emptyTable);

  public:
    HttpApp();
};

void increaseLogIndent();
void decendLogIndent();
void setLogName(std::string &&);
void setLogName();

void resetLogName();
void getMyTestName(std::string &out);

std::string generatorRandom(const std::string &prefix = "", size_t len = 10);