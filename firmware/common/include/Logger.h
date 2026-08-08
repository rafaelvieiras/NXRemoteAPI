#pragma once

#ifdef UNIT_TEST
#include "MockLibnx.h"
#else
#include <switch.h>
#endif
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <mutex>

#define MAX_LOG_MESSAGE_LEN 256
#define MAX_LOG_ENTRIES 50

typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

typedef struct {
    LogLevel level;
    char message[MAX_LOG_MESSAGE_LEN];
    char timestamp[32];
} LogEntry;

// Guarded by m_mutex: the HTTP handler thread and the mDNS responder thread (see
// mdns_responder() in firmware/core/main.cpp) can both log concurrently, and the
// ring buffer's head/count bookkeeping isn't safe under a concurrent writer +
// reader without it.
class Logger {
public:
    void init() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_head = 0;
        m_count = 0;
        m_errorCount = 0;
        memset(m_logs, 0, sizeof(m_logs));
    }

    void log(LogLevel level, const char* message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_count >= 0xFFFFFFF0) return; // Safeguard

        LogEntry* entry = &m_logs[m_head];
        entry->level = level;
        strncpy(entry->message, message, MAX_LOG_MESSAGE_LEN - 1);
        entry->message[MAX_LOG_MESSAGE_LEN - 1] = '\0';

        time_t now = time(0);
        struct tm* tm_info = localtime(&now);
        if (tm_info) {
            strftime(entry->timestamp, sizeof(entry->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        } else {
            strcpy(entry->timestamp, "unknown");
        }

        m_head = (m_head + 1) % MAX_LOG_ENTRIES;
        if (m_count < MAX_LOG_ENTRIES) {
            m_count++;
        }

        if (level == LOG_LEVEL_ERROR) {
            m_errorCount++;
        }
    }

    u32 getErrorCount() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_errorCount;
    }

    u32 getLogCount() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count;
    }

    // Copies the entry out under the lock instead of returning a pointer into
    // m_logs - a returned pointer could be overwritten by a concurrent log() the
    // instant the lock is released.
    bool getLog(u32 index, LogEntry* out) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (index >= m_count) return false;
        u32 actual_index = (m_head - m_count + index + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
        *out = m_logs[actual_index];
        return true;
    }

    static Logger& getInstance();

private:
    std::mutex m_mutex;
    LogEntry m_logs[MAX_LOG_ENTRIES];
    u32 m_head;
    u32 m_count;
    u32 m_errorCount;
};

extern Logger g_logger;

inline Logger& Logger::getInstance() {
    return g_logger;
}

#define LOG_I(msg) Logger::getInstance().log(LOG_LEVEL_INFO, msg)
#define LOG_W(msg) Logger::getInstance().log(LOG_LEVEL_WARN, msg)
#define LOG_E(msg) Logger::getInstance().log(LOG_LEVEL_ERROR, msg)
