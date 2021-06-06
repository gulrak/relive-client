///////////////////////////////////////////////////////////////////////////
//
//  FILE:
//  igx/logging.h
//
//  Copyright (c) 2011-2014 Schümann Development. All rights reserved.
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <atomic>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

namespace igx
{

    enum class LogType { eError, eWarning, eInfo, eDebug };

    class LogManager
    {
    public:
        enum LogStyle {
            eShowPID = 1,
            eShowTID = 2,
            eShowType = 4,
            eShowTimestamp = 8,
            eShowFunction = 0x10,
            eShowFile = 0x20,
            eShowLine = 0x40
        };
        LogManager(const LogManager&) = delete;
        LogManager& operator=(const LogManager&) = delete;
        ~LogManager();

        void defaultLevel(int level);
        void logLevel(const std::string& className, int level);
        bool isActive(const std::string& className, int level);
        void logStyle(int logStyle);

        static void registerIsActive(const std::string& className, int level, std::atomic<bool>& isInit, volatile bool& isActive);
        static void log(LogType type, const std::string& className, const std::string& file, int line, const std::string& function, int level, const std::string& message);
        static LogManager* instance();

    private:
        LogManager();
        void refreshActiveStates();
        uint64_t symbolicThreadId();
        static std::string detemplatedClassName(const std::string& className);
        struct LogPoint
        {
            int _level;
            volatile bool* _isActive;
        };
        typedef std::multimap<std::string, LogPoint> LogPoints;
        typedef std::map<std::string,int> LogLevels;
        std::ofstream _outfile;
        std::mutex _mutex;
        LogPoints _logPoints;
        LogLevels _logLevels;
        int _logStyle;
        int _defaultLevel;
        std::map<std::thread::id,uint64_t> _symbolicThreadIds;
    };

std::string stripTypeName(const std::string& classname);

}

#ifdef _WIN32
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

#define GET_MACRO(_1,_2,_3,NAME,...) NAME
#define DEBUG_LOG(...) GET_MACRO(__VA_ARGS__, DEBUG_LOG3, DEBUG_LOG2, 0)(__VA_ARGS__)
#define INFO_LOG(...) GET_MACRO(__VA_ARGS__, INFO_LOG3, INFO_LOG2, 0)(__VA_ARGS__)
#define ERROR_LOG(...) GET_MACRO(__VA_ARGS__, ERROR_LOG3, ERROR_LOG2, 0)(__VA_ARGS__)

#define DEBUG_LOG3(cls,lvl,msg) \
do {\
    static std::atomic<bool> _init{false}; \
    static volatile bool _isActive{true}; \
    if(_isActive) {\
        if(!_init.load(std::memory_order_acquire)) { \
            igx::LogManager::registerIsActive(igx::stripTypeName(typeid(cls).name()), lvl, _init, _isActive); \
            if(!_isActive) break; \
        } \
        std::ostringstream os; os << msg; \
        igx::LogManager::log(igx::LogType::eDebug, igx::stripTypeName(typeid(cls).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
    }\
} while(0)

#define DEBUG_LOG2(lvl,msg) \
do {\
    static std::atomic<bool> _init{false}; \
    static volatile bool _isActive{true}; \
    if(_isActive) {\
        if(!_init.load(std::memory_order_acquire)) { \
            igx::LogManager::registerIsActive(igx::stripTypeName(typeid(*this).name()), lvl, _init, _isActive); \
            if(!_isActive) break; \
        } \
        std::ostringstream os; os << msg; \
        igx::LogManager::log(igx::LogType::eDebug, igx::stripTypeName(typeid(*this).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
    }\
} while(0)

#define INFO_LOG3(cls,lvl,msg) \
do {\
    std::ostringstream os; os << msg; \
    igx::LogManager::log(igx::LogType::eInfo, igx::stripTypeName(typeid(cls).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
} while(0)

#define INFO_LOG2(lvl,msg) \
do {\
    std::ostringstream os; os << msg; \
    LogManager::log(LogType::eInfo, igx::stripTypeName(typeid(*this).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
} while(0)

#define WARNING_LOG3(cls,lvl,msg) \
do {\
    std::ostringstream os; os << msg; \
    igx::LogManager::log(igx::LogType::eWarning, igx::stripTypeName(typeid(cls).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
} while(0)

#define WARNING_LOG2(lvl,msg) \
do {\
    std::ostringstream os; os << msg; \
    igx::LogManager::log(igx::LogType::eWarning, igx::stripTypeName(typeid(*this).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
} while(0)

#define ERROR_LOG3(cls,lvl,msg) \
do {\
    std::ostringstream os; os << msg; \
    igx::LogManager::log(igx::LogType::eError, igx::stripTypeName(typeid(cls).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
} while(0)

#define ERROR_LOG2(lvl,msg) \
do {\
    std::ostringstream os; os << msg; \
    igx::LogManager::log(igx::LogType::eError, igx::stripTypeName(typeid(*this).name()), __FILE__, __LINE__, __PRETTY_FUNCTION__, lvl, os.str()); \
} while(0)


