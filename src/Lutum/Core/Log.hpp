/*
* File: Log.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_LOG_HPP
#define LUTUM_LOG_HPP
#include <cstdint>
#include <format>
#include <source_location>
#include <string_view>
#include <utility>

// Compile-time floor
// 0=Trace 1=Debug 2=Info 3=Warn 4=Error 5=Fatal
#ifndef LUTUM_LOG_MIN_LEVEL
    #ifdef NDEBUG
        #define LUTUM_LOG_MIN_LEVEL 2
    #else
        #define LUTUM_LOG_MIN_LEVEL 0
    #endif
#endif

namespace Lutum {

enum class LogLevel : uint8_t {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
    Off
};

namespace Log {
    // Call once at startup, before other systems
    // NOTE: safe to log before Initialize; output just goes to console only
    bool Initialize(const char* filePath = "Lutum.log");
    void Shutdown();

    void SetLevel(LogLevel level);
    [[nodiscard]]
    LogLevel GetLevel();

    // Internal. Use LUTUM_LOG_* macros
    void Write(LogLevel level, std::string_view message, const std::source_location& loc);

    template<typename... Args>
    void Print(LogLevel level, const std::source_location& loc, std::format_string<Args...> fmt, Args&&... args) {
        if (level < GetLevel())
            return;

        Write(level, std::format(fmt, std::forward<Args>(args)...), loc);
    }
} // Log
} // Lutum

#if LUTUM_LOG_MIN_LEVEL <= 0
#define LUTUM_TRACE(...) ::Lutum::Log::Print(::Lutum::LogLevel::Trace, std::source_location::current(), __VA_ARGS__)
#else
#define LUTUM_TRACE(...) ((void)0)
#endif

#if LUTUM_LOG_MIN_LEVEL <= 1
#define LUTUM_DEBUG(...) ::Lutum::Log::Print(::Lutum::LogLevel::Debug, std::source_location::current(), __VA_ARGS__)
#else
#define LUTUM_DEBUG(...) ((void)0)
#endif

#if LUTUM_LOG_MIN_LEVEL <= 2
#define LUTUM_INFO(...) ::Lutum::Log::Print(::Lutum::LogLevel::Info, std::source_location::current(), __VA_ARGS__)
#else
#define LUTUM_INFO(...) ((void)0)
#endif

#define LUTUM_WARN(...)  ::Lutum::Log::Print(::Lutum::LogLevel::Warn,  std::source_location::current(), __VA_ARGS__)
#define LUTUM_ERROR(...) ::Lutum::Log::Print(::Lutum::LogLevel::Error, std::source_location::current(), __VA_ARGS__)
#define LUTUM_FATAL(...) ::Lutum::Log::Print(::Lutum::LogLevel::Fatal, std::source_location::current(), __VA_ARGS__)

// Assert: message args only evaluated on failure. Enabled in debug builds.
#ifndef NDEBUG
    #define LUTUM_ASSERT(condition, ...)                                             \
        do {                                                                         \
            if (!(condition)) {                                                      \
                LUTUM_FATAL("Assert failed: (" #condition ") " __VA_ARGS__);         \
                SDL_TriggerBreakpoint();                                             \
            }                                                                        \
        } while (0)
#else
    #define LUTUM_ASSERT(condition, ...) ((void)0)
#endif

#endif //LUTUM_LOG_HPP
