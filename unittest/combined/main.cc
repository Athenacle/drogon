#include "gtest/gtest.h"
#include "combined.h"

#include <atomic>

#include <trantor/utils/Logger.h>

#include <fmt/chrono.h>

#define RED "\e[31m"
#define FATAL "\e[1m" RED
#define GREEN "\e[32m"
#define YELLOW "\e[33m"
#define BLUE "\e[34m"
#define MAGENTA "\e[35m"

#define RESET "\e[0m"

namespace
{
auto& getIntent()
{
    static std::atomic<int> indent = 0;
    return indent;
}

auto& testNameMutex()
{
    static std::mutex m;
    return m;
}

auto& testName()
{
    static std::string name;

    return name;
}

}  // namespace

void increaseLogIndent()
{
    getIntent().fetch_add(1);

    if (getIntent() >= 5)
    {
        getIntent() = 5;
    }
}

void setLogName()
{
#define DIM " \e[2m"
#define LG "\e[37m"
    std::string name;
    getMyTestName(name);
    setLogName(fmt::format("{}{}{}", DIM LG, name, RESET));
}

void setLogName(std::string&& n)
{
    testNameMutex().lock();
    testName().swap(n);
    testNameMutex().unlock();
}

void resetLogName()
{
    setLogName("");
}

void decendLogIndent()
{
    getIntent().fetch_sub(1);

    if (getIntent() < 0)
    {
        getIntent() = 0;
    }
}

class TrantorLogger : public trantor::logger::AbstractLogger
{
    using level = trantor::Logger::LogLevel;

    std::mutex m;

  public:
    virtual void flush() override
    {
        std::flush(std::cerr);
    }

    virtual void output(level l, const std::string& msg) override
    {
        static const char* lv[16] = {MAGENTA " [TRACE] " RESET,
                                     BLUE " [DEBUG] " RESET,
                                     GREEN " [INFO] " RESET,
                                     YELLOW " [WARN] " RESET,
                                     RED " [ERROR] " RESET,
                                     FATAL " [FATAL] " RESET

        };
        const char* lvs;
        std::string indent;

        if (l >= trantor::Logger::LogLevel::kTrace &&
            l < trantor::Logger::LogLevel::kNumberOfLogLevels)
        {
            lvs = lv[l];
        }
        else
        {
            lvs = lv[2];  // fatal
            output(trantor::Logger::LogLevel::kWarn,
                   fmt::format("unknown logger level {}, assume as INFO", l));
        }
        for (int i = getIntent(); i > 0; i--)
        {
            indent.append(2, '<');
        }
        if (indent.length() > 0)
        {
            indent.append(" ");
        }

        auto now =
            fmt::format("{:%Y-%m-%d %k:%M:%S} ", fmt::localtime(time(nullptr)));

        {
            std::lock_guard _(m);
            std::cerr << now << lvs << indent;
            testNameMutex().lock();
            if (testName().length() > 0)
            {
                std::cerr << testName() << ": ";
            }
            testNameMutex().unlock();
            std::cerr << msg;
        }
    }

    TrantorLogger()
    {
    }
};

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    srand(time(nullptr));
    trantor::logger::LoggerManager::setLoggerImplement<TrantorLogger>();
    return RUN_ALL_TESTS();
}