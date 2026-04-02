#pragma once
#include <iostream>
#include <string>
#include <sstream>

class Debug {
public:
    enum Level {
        LEVEL_ERROR = 0,
        LEVEL_WARNING = 1,
        LEVEL_INFO = 2,
        LEVEL_VERBOSE = 3,
        LEVEL_TRACE = 4
    };

private:
    static Level s_debugLevel;
    static bool s_enabled;

public:
    static void SetEnabled(bool enabled) { s_enabled = enabled; }
    static void SetLevel(Level level) { s_debugLevel = level; }
    static bool IsEnabled() { return s_enabled; }
    static Level GetLevel() { return s_debugLevel; }

    static void Log(Level level, const std::string& category, const std::string& message) {
        if (!s_enabled || level > s_debugLevel) return;

        std::string prefix;
        switch (level) {
            case LEVEL_ERROR:   prefix = "[ERROR]"; break;
            case LEVEL_WARNING: prefix = "[WARN] "; break;
            case LEVEL_INFO:    prefix = "[INFO] "; break;
            case LEVEL_VERBOSE: prefix = "[VERB] "; break;
            case LEVEL_TRACE:   prefix = "[TRACE]"; break;
        }

        std::cout << prefix << " [" << category << "] " << message << std::endl;
    }

    template<typename... Args>
    static void LogF(Level level, const std::string& category, const std::string& format, Args... args) {
        if (!s_enabled || level > s_debugLevel) return;
        
        std::stringstream ss;
        FormatImpl(ss, format, args...);
        Log(level, category, ss.str());
    }

private:
    template<typename T>
    static void FormatImpl(std::stringstream& ss, const std::string& format, T&& value) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            ss << format.substr(0, pos) << value << format.substr(pos + 2);
        } else {
            ss << format;
        }
    }

    template<typename T, typename... Args>
    static void FormatImpl(std::stringstream& ss, const std::string& format, T&& value, Args... args) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            ss << format.substr(0, pos) << value;
            FormatImpl(ss, format.substr(pos + 2), args...);
        } else {
            ss << format;
        }
    }
};

#define DEBUG_ENABLED_FOR(level) (Debug::IsEnabled() && Debug::GetLevel() >= (level))

#define DEBUG_ERROR(category, message)   do { if (DEBUG_ENABLED_FOR(Debug::LEVEL_ERROR))   Debug::Log(Debug::LEVEL_ERROR,   category, message); } while(0)
#define DEBUG_WARN(category, message)    do { if (DEBUG_ENABLED_FOR(Debug::LEVEL_WARNING)) Debug::Log(Debug::LEVEL_WARNING, category, message); } while(0)
#define DEBUG_INFO(category, message)    do { if (DEBUG_ENABLED_FOR(Debug::LEVEL_INFO))    Debug::Log(Debug::LEVEL_INFO,    category, message); } while(0)
#define DEBUG_VERBOSE(category, message) do { if (DEBUG_ENABLED_FOR(Debug::LEVEL_VERBOSE)) Debug::Log(Debug::LEVEL_VERBOSE, category, message); } while(0)
#define DEBUG_TRACE(category, message)   do { if (DEBUG_ENABLED_FOR(Debug::LEVEL_TRACE))   Debug::Log(Debug::LEVEL_TRACE,   category, message); } while(0)

#define DEBUG_ERROR_F(category, format, ...)   do { if (DEBUG_ENABLED_FOR(Debug::LEVEL_ERROR))   Debug::LogF(Debug::LEVEL_ERROR,   category, format, __VA_ARGS__); } while(0)
#define DEBUG_WARN_F(category, format, ...)    do { if (DEBUG_ENABLED_FOR(Debug::LEVEL_WARNING)) Debug::LogF(Debug::LEVEL_WARNING, category, format, __VA_ARGS__); } while(0)
#define DEBUG_INFO_F(category, format, ...)    do { if (DEBUG_ENABLED_FOR(Debug::LEVEL_INFO))    Debug::LogF(Debug::LEVEL_INFO,    category, format, __VA_ARGS__); } while(0)
#define DEBUG_VERBOSE_F(category, format, ...) do { if (DEBUG_ENABLED_FOR(Debug::LEVEL_VERBOSE)) Debug::LogF(Debug::LEVEL_VERBOSE, category, format, __VA_ARGS__); } while(0)
#define DEBUG_TRACE_F(category, format, ...)   do { if (DEBUG_ENABLED_FOR(Debug::LEVEL_TRACE))   Debug::LogF(Debug::LEVEL_TRACE,   category, format, __VA_ARGS__); } while(0)