#ifndef VIOSASSISTANT_LOGGER_H
#define VIOSASSISTANT_LOGGER_H

#pragma once

#include <cstdint>
#include <cstdio>

namespace utils {

enum class LogLevel : uint8_t {
    NONE = 0,
    ERROR = 1,
    WARN = 2,
    INFO = 3,
    DEBUG = 4,
    VERBOSE = 5
};

class Logger {
public:
    static void begin(LogLevel level = LogLevel::INFO);
    static void setLevel(LogLevel level);
    static LogLevel getLevel();
    static void error(const char* tag, const char* fmt, ...);
    static void warn(const char* tag, const char* fmt, ...);
    static void info(const char* tag, const char* fmt, ...);
    static void debug(const char* tag, const char* fmt, ...);
    static void verbose(const char* tag, const char* fmt, ...);
    static void print(LogLevel level, const char* tag, const char* fmt, va_list args);

private:
    static LogLevel current_level_;
    static char log_buffer_[256];
    static const char* levelToStr(LogLevel level);
};

#define LOG_ERROR(tag, ...)   utils::Logger::error(tag, __VA_ARGS__)
#define LOG_WARN(tag, ...)    utils::Logger::warn(tag, __VA_ARGS__)
#define LOG_INFO(tag, ...)    utils::Logger::info(tag, __VA_ARGS__)
#define LOG_DEBUG(tag, ...)   utils::Logger::debug(tag, __VA_ARGS__)
#define LOG_VERBOSE(tag, ...) utils::Logger::verbose(tag, __VA_ARGS__)

} // namespace utils

#endif // VIOSASSISTANT_LOGGER_H
