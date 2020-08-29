//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
#pragma once

#include <atomic>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

namespace relive {

enum class LogType { eError, eWarning, eInfo, eDebug };

class LogManager
{
public:
    enum LogStyle { eShowPID = 1, eShowTID = 2, eShowType = 4, eShowTimestamp = 8, eShowFunction = 0x10, eShowFile = 0x20, eShowLine = 0x40 };
    ~LogManager();

    void defaultLevel(int level);
    void logLevel(const std::string& className, int level);
    bool isActive(const std::string& className, int level);
    void logStyle(int logStyle);

    static void setOutputFile(const std::string& file);
    static void registerIsActive(const std::string& className, int level, std::atomic<bool>& isInit, volatile bool& isActive);
    static void log(LogType type, const std::string& className, const std::string& file, int line, const std::string& function, int level, const std::string& message);
    static LogManager* instance(const std::string& file = std::string());

private:
    LogManager(const std::string& file = std::string());
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;
    void refreshActiveStates();
    uint64_t symbolicThreadId();
    static std::string detemplatedClassName(const std::string& className);
    struct LogPoint
    {
        int _level;
        volatile bool* _isActive;
    };
    typedef std::multimap<std::string, LogPoint> LogPoints;
    typedef std::map<std::string, int> LogLevels;
    std::mutex _mutex;
    LogPoints _logPoints;
    LogLevels _logLevels;
    int _logStyle;
    int _defaultLevel;
    std::ostream* _os = nullptr;
    std::unique_ptr<std::ofstream> _file;
    std::map<std::thread::id, uint64_t> _symbolicThreadIds;
};

std::string stripTypeName(const std::string& classname);

}  // namespace relive

#ifdef _WIN32
#define __PRETTY_FUNCTION__ __FUNCTION__

#define GLUE(x, y) x y

#define RETURN_ARG_COUNT(_1_, _2_, _3_, _4_, _5_, count, ...) count
#define EXPAND_ARGS(args) RETURN_ARG_COUNT args
#define COUNT_ARGS_MAX5(...) EXPAND_ARGS((__VA_ARGS__, 5, 4, 3, 2, 1, 0))

#define OVERLOAD_MACRO2(name, count) name##count
#define OVERLOAD_MACRO1(name, count) OVERLOAD_MACRO2(name, count)
#define OVERLOAD_MACRO(name, count) OVERLOAD_MACRO1(name, count)

#define CALL_OVERLOAD(name, ...) GLUE(OVERLOAD_MACRO(name, COUNT_ARGS_MAX5(__VA_ARGS__)), (__VA_ARGS__))
#define DEBUG_LOG(...) CALL_OVERLOAD(DEBUG_LOG, __VA_ARGS__)
#define INFO_LOG(...) CALL_OVERLOAD(INFO_LOG, __VA_ARGS__)
#define ERROR_LOG(...) CALL_OVERLOAD(ERROR_LOG, __VA_ARGS__)
#else
#define GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define DEBUG_LOG(...) GET_MACRO(__VA_ARGS__, DEBUG_LOG3, DEBUG_LOG2, 0)(__VA_ARGS__)
#define INFO_LOG(...) GET_MACRO(__VA_ARGS__, INFO_LOG3, INFO_LOG2, 0)(__VA_ARGS__)
#define ERROR_LOG(...) GET_MACRO(__VA_ARGS__, ERROR_LOG3, ERROR_LOG2, 0)(__VA_ARGS__)
#endif

#define DEBUG_LOG3(cls, lvl, msg)                                                                                                                                \
    do {                                                                                                                                                         \
        static std::atomic<bool> _init{false};                                                                                                                   \
        static volatile bool _isActive{true};                                                                                                                    \
        if (_isActive) {                                                                                                                                         \
            if (!_init.load(std::memory_order_acquire)) {                                                                                                        \
                relive::LogManager::registerIsActive(relive::stripTypeName(typeid(cls).name()), lvl, _init, _isActive);                                          \
                if (!_isActive)                                                                                                                                  \
                    break;                                                                                                                                       \
            }                                                                                                                                                    \
            std::ostringstream os;                                                                                                                               \
            os << msg;                                                                                                                                           \
            relive::LogManager::log(relive::LogType::eDebug, relive::stripTypeName(typeid(cls).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
        }                                                                                                                                                        \
    } while (0)

#define DEBUG_LOG2(lvl, msg)                                                                                                                                       \
    do {                                                                                                                                                           \
        static std::atomic<bool> _init{false};                                                                                                                     \
        static volatile bool _isActive{true};                                                                                                                      \
        if (_isActive) {                                                                                                                                           \
            if (!_init.load(std::memory_order_acquire)) {                                                                                                          \
                relive::LogManager::registerIsActive(relive::stripTypeName(typeid(*this).name()), lvl, _init, _isActive);                                          \
                if (!_isActive)                                                                                                                                    \
                    break;                                                                                                                                         \
            }                                                                                                                                                      \
            std::ostringstream os;                                                                                                                                 \
            os << msg;                                                                                                                                             \
            relive::LogManager::log(relive::LogType::eDebug, relive::stripTypeName(typeid(*this).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
        }                                                                                                                                                          \
    } while (0)

#define INFO_LOG3(cls, lvl, msg)                                                                                                                            \
    do {                                                                                                                                                    \
        std::ostringstream os;                                                                                                                              \
        os << msg;                                                                                                                                          \
        relive::LogManager::log(relive::LogType::eInfo, relive::stripTypeName(typeid(cls).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
    } while (0)

#define INFO_LOG2(lvl, msg)                                                                                                                   \
    do {                                                                                                                                      \
        std::ostringstream os;                                                                                                                \
        os << msg;                                                                                                                            \
        LogManager::log(LogType::eInfo, relive::stripTypeName(typeid(*this).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
    } while (0)

#define WARNING_LOG3(cls, lvl, msg)                                                                                                                            \
    do {                                                                                                                                                       \
        std::ostringstream os;                                                                                                                                 \
        os << msg;                                                                                                                                             \
        relive::LogManager::log(relive::LogType::eWarning, relive::stripTypeName(typeid(cls).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
    } while (0)

#define WARNING_LOG2(lvl, msg)                                                                                                                                   \
    do {                                                                                                                                                         \
        std::ostringstream os;                                                                                                                                   \
        os << msg;                                                                                                                                               \
        relive::LogManager::log(relive::LogType::eWarning, relive::stripTypeName(typeid(*this).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
    } while (0)

#define ERROR_LOG3(cls, lvl, msg)                                                                                                                            \
    do {                                                                                                                                                     \
        std::ostringstream os;                                                                                                                               \
        os << msg;                                                                                                                                           \
        relive::LogManager::log(relive::LogType::eError, relive::stripTypeName(typeid(cls).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
    } while (0)

#define ERROR_LOG2(lvl, msg)                                                                                                                                   \
    do {                                                                                                                                                       \
        std::ostringstream os;                                                                                                                                 \
        os << msg;                                                                                                                                             \
        relive::LogManager::log(relive::LogType::eError, relive::stripTypeName(typeid(*this).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
    } while (0)
