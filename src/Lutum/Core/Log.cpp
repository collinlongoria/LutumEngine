/*
* File: Log.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Core/Log.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <algorithm>

#include <SDL3/SDL_log.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace Lutum {
namespace {
    struct LogState {
        std::FILE* file = nullptr;
        std::mutex writeMutex;
        std::atomic<LogLevel> level{LogLevel::Trace};
        std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
        static constexpr size_t kRingCapacity = 1024;
        std::vector<Log::RingEntry> ring{kRingCapacity};
        uint64_t serial = 0; // total messages ever written
    };

    LogState& State() {
        static LogState state;
        return state;
    }

    constexpr const char* LevelTag(LogLevel level) {
        switch (level) {
            case LogLevel::Trace:
                return "TRACE";
            case LogLevel::Debug:
                return "DEBUG";
            case LogLevel::Info:
                return "INFO ";
            case LogLevel::Warn:
                return "WARN ";
            case LogLevel::Error:
                return "ERROR";
            case LogLevel::Fatal:
                return "FATAL";
            default:
                return "?????";
        }
    }


    constexpr const char* LevelColor(LogLevel level) {
        switch (level) {
            case LogLevel::Trace:
                return "\033[90m"; // bright black
            case LogLevel::Debug:
                return "\033[36m"; // cyan
            case LogLevel::Info:
                return "\033[37m"; // white
            case LogLevel::Warn:
                return "\033[33m"; // yellow
            case LogLevel::Error:
                return "\033[31m"; // red
            case LogLevel::Fatal:
                return "\033[41;97m"; // white on red
            default:
                return "\033[0m";
        }
    }

    // Strip directory from source_location file names
    const char* ShortFileName(const char* path) {
        const char* file = path;
        for (const char* p = path; *p; ++p) {
            if (*p == '/' || *p == '\\') {
                file = p + 1;
            }
        }
        return file;
    }

    void EnableVirtualTerminal() {
#ifdef _WIN32
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        if (out != INVALID_HANDLE_VALUE) {
            DWORD mode = 0;
            if (GetConsoleMode(out, &mode)) {
                SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            }
        }
#endif
    }

    // Forward SDL's logging into Lutum's
    void SDLCALL SDLLogBridge(void*, int, SDL_LogPriority priority, const char* message) {
        LogLevel level = LogLevel::Info;
        switch (priority) {
            case SDL_LOG_PRIORITY_TRACE:
            case SDL_LOG_PRIORITY_VERBOSE:
                level = LogLevel::Trace;
                break;
            case SDL_LOG_PRIORITY_DEBUG:
                level = LogLevel::Debug;
                break;
            case SDL_LOG_PRIORITY_INFO:
                level = LogLevel::Info;
                break;
            case SDL_LOG_PRIORITY_WARN:
                level = LogLevel::Warn;
                break;
            case SDL_LOG_PRIORITY_ERROR:
                level = LogLevel::Error;
                break;
            case SDL_LOG_PRIORITY_CRITICAL:
                level = LogLevel::Fatal;
                break;
            default:
                break;
        }

        if (level < Log::GetLevel())
            return;

        Log::Write(level, message, std::source_location::current());
    }
} // anonymous namespace

namespace Log {

    bool Initialize(const char* filePath) {
        LogState& state = State();

        EnableVirtualTerminal();

        if (filePath && !state.file) {
            state.file = std::fopen(filePath, "w");
            if (!state.file) {
                std::fprintf(stderr, "Log::Initialize failed to open '%s'\n", filePath);
            }
        }

        SDL_SetLogOutputFunction(SDLLogBridge, nullptr);

        return state.file != nullptr;
    }

    void Shutdown() {
        LogState& state = State();
        std::lock_guard lock(state.writeMutex);

        SDL_SetLogOutputFunction(SDL_GetDefaultLogOutputFunction(), nullptr);

        if (state.file) {
            std::fclose(state.file);
            state.file = nullptr;
        }
    }

    void SetLevel(LogLevel level) {
        State().level.store(level, std::memory_order_relaxed);
    }

    LogLevel GetLevel() {
        return State().level.load(std::memory_order_relaxed);
    }

    void Write(LogLevel level, std::string_view message, const std::source_location& loc) {
        LogState& state = State();

        // Timestamp MUST be formatted outside the lock
        const auto now = std::chrono::system_clock::now();
        const std::time_t nowT = std::chrono::system_clock::to_time_t(now);

        std::tm localTime = {};
#ifdef _WIN32
        localtime_s(&localTime, &nowT);
#else
        localtime_r(&nowT, &localTime);
#endif

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - state.startTime).count();

        char timestamp[16];
        std::snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d",
                      localTime.tm_hour, localTime.tm_min, localTime.tm_sec);

        // Detect the SDL bridge
        const bool fromSDL = (ShortFileName(loc.file_name()) == std::string_view("Log.cpp"));

        std::string line;
        if (fromSDL) {
            line = std::format("[{}|{:>8}ms][{}][SDL] {}", timestamp, elapsed, LevelTag(level), message);
        }
        else {
            line = std::format("[{}|{:>8}ms][{}][{}:{}] {}", timestamp, elapsed, LevelTag(level), ShortFileName(loc.file_name()), loc.line(), message);
        }

        // Only the actual IO is serialized
        {
            std::lock_guard lock(state.writeMutex);

            std::FILE* console = (level >= LogLevel::Error) ? stderr : stdout;
            std::fprintf(console, "%s%s\033[0m\n", LevelColor(level), line.c_str());

            if (state.file) {
                std::fprintf(state.file, "%s\n", line.c_str());
                if (level >= LogLevel::Warn) {
                    std::fflush(state.file); // don't lose warnings/errors on crash
                }
            }

            state.ring[state.serial % LogState::kRingCapacity] = RingEntry{level, line};
            ++state.serial;
        }
    }

    uint64_t ReadRing(uint64_t sinceSerial, std::vector<RingEntry>& out) {
        LogState& state = State();
        std::lock_guard lock(state.writeMutex);

        const uint64_t oldest = state.serial > LogState::kRingCapacity
            ? state.serial - LogState::kRingCapacity : 0;
        for (uint64_t s = std::max(sinceSerial, oldest); s < state.serial; ++s)
            out.push_back(state.ring[s % LogState::kRingCapacity]);
        return state.serial;
    }
}
} // Lutum