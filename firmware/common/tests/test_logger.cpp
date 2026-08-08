#include "catch.hpp"
#include "../include/Logger.h"

TEST_CASE("Logger stores and retrieves entries", "[logger]") {
    Logger logger;
    logger.init();

    SECTION("A logged entry round-trips through getLog") {
        logger.log(LOG_LEVEL_INFO, "hello");

        REQUIRE(logger.getLogCount() == 1);
        LogEntry entry;
        REQUIRE(logger.getLog(0, &entry));
        REQUIRE(std::string(entry.message) == "hello");
        REQUIRE(entry.level == LOG_LEVEL_INFO);
    }

    SECTION("getLog rejects an out-of-range index") {
        LogEntry entry;
        REQUIRE_FALSE(logger.getLog(0, &entry));
        logger.log(LOG_LEVEL_INFO, "only one");
        REQUIRE_FALSE(logger.getLog(1, &entry));
    }

    SECTION("Error level increments the error counter, others don't") {
        logger.log(LOG_LEVEL_INFO, "info");
        logger.log(LOG_LEVEL_WARN, "warn");
        logger.log(LOG_LEVEL_ERROR, "error");

        REQUIRE(logger.getErrorCount() == 1);
    }

    SECTION("The ring buffer wraps and drops the oldest entry past MAX_LOG_ENTRIES") {
        char msg[32];
        for (int i = 0; i < MAX_LOG_ENTRIES + 5; i++) {
            snprintf(msg, sizeof(msg), "entry-%d", i);
            logger.log(LOG_LEVEL_INFO, msg);
        }

        REQUIRE(logger.getLogCount() == MAX_LOG_ENTRIES);

        // Oldest surviving entry should be "entry-5" (the first 5 got evicted),
        // logical index 0 always maps to the oldest surviving entry.
        LogEntry oldest;
        REQUIRE(logger.getLog(0, &oldest));
        REQUIRE(std::string(oldest.message) == "entry-5");

        LogEntry newest;
        REQUIRE(logger.getLog(MAX_LOG_ENTRIES - 1, &newest));
        REQUIRE(std::string(newest.message) == "entry-" + std::to_string(MAX_LOG_ENTRIES + 4));
    }

    SECTION("A message longer than the buffer is truncated, not overrun") {
        std::string long_msg(MAX_LOG_MESSAGE_LEN + 50, 'x');
        logger.log(LOG_LEVEL_INFO, long_msg.c_str());

        LogEntry entry;
        REQUIRE(logger.getLog(0, &entry));
        REQUIRE(strlen(entry.message) == MAX_LOG_MESSAGE_LEN - 1);
    }
}
