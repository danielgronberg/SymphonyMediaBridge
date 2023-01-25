#include "logger/Logger.h"
#include "logger/LoggerThread.h"
#include "logger/PruneSpam.h"
#include "logger/SuspendSpam.h"
#include <gtest/gtest.h>

TEST(LogSpam, prune)
{
    uint32_t logCount = 0;

    logger::PruneSpam pruner(10, 50);
    for (int i = 0; i < 1001; ++i)
    {
        if (pruner.canLog())
        {
            ++logCount;
        }
    }

    EXPECT_EQ(logCount, 30);
}

TEST(LogSpam, suspend)
{
    uint32_t logCount = 0;

    logger::SuspendSpam pruner(10, utils::Time::sec * 2);
    for (uint64_t timestamp = 1; timestamp < utils::Time::sec * 61; timestamp += utils::Time::ms * 10)
    {
        if (pruner.canLog(timestamp))
        {
            ++logCount;
        }
    }

    EXPECT_EQ(logCount, 40);
}

namespace logger
{

extern std::unique_ptr<LoggerThread> _logThread;
} // namespace logger

TEST(LogSpam, logSizes)
{
    char log[1000];
    std::fill(log, log + sizeof(log) - 1, 'o');
    for (int i = 1; i < 990; ++i)
    {
        log[i] = '\0';
        logger::debug("%s", "", log);
        log[i] = 'o';
    }

    EXPECT_EQ(logger::_logThread->getDroppedLogCount(), 0);
}
