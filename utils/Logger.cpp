#include "Logger.h"
#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

namespace utils {

LogLevel Logger::current_level_ = LogLevel::INFO;
char Logger::log_buffer_[256];

void Logger::begin(LogLevel level) {
    current_level_ = level;
    Serial.begin(115200);
}

void Logger::setLevel(LogLevel level) {
    current_level_ = level;
}

LogLevel Logger::getLevel() {
    return current_level_;
}

const char* Logger::levelToStr(LogLevel level) {
    switch (level) {
        case LogLevel::ERROR: return "E";
        case LogLevel::WARN: return "W";
        case LogLevel::INFO: return "I";
        case LogLevel::DEBUG: return "D";
        case LogLevel::VERBOSE: return "V";
        default: return "?";
    }
}

void Logger::print(LogLevel level, const char* tag, const char* fmt, va_list args) {
    if (level > current_level_) return;
    if (!tag) tag = "APP";
    if (!fmt) return;

    uint32_t ms = millis();
    int prefix_len = snprintf(log_buffer_, sizeof(log_buffer_), "[%lu][%s][%s] ", ms, levelToStr(level), tag);
    if (prefix_len < 0 || prefix_len >= (int)sizeof(log_buffer_)) return;

    va_list args_copy;
    va_copy(args_copy, args);
    int msg_len = vsnprintf(log_buffer_ + prefix_len, sizeof(log_buffer_) - prefix_len, fmt, args_copy);
    va_end(args_copy);

    if (msg_len < 0) return;

    Serial.println(log_buffer_);
}

void Logger::error(const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print(LogLevel::ERROR, tag, fmt, args);
    va_end(args);
}

void Logger::warn(const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print(LogLevel::WARN, tag, fmt, args);
    va_end(args);
}

void Logger::info(const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print(LogLevel::INFO, tag, fmt, args);
    va_end(args);
}

void Logger::debug(const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print(LogLevel::DEBUG, tag, fmt, args);
    va_end(args);
}

void Logger::verbose(const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print(LogLevel::VERBOSE, tag, fmt, args);
    va_end(args);
}

} // namespace utils
