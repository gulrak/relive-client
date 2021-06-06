#include "logging.h"
#include "applog.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

#ifndef WIN32
#include <unistd.h>
#include <pthread.h>
#endif

#ifdef __APPLE__
#include <sys/_types.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#include <stdlib.h>
#endif

using namespace igx;

#if defined(__GNUC__) || defined(__clang__)

static std::string demangle(const std::string&  s)
{
    int status(0);
    std::string result;

    char* ss = abi::__cxa_demangle(s.c_str(), 0, 0, &status);

    if(status == -1)
    {
        throw std::bad_alloc();
    }
    else if(status == -2)
    {
        result = s;
    }
    else if(status == -3)
    {
        throw std::runtime_error("__cxa_demangle returned -3");
    }
    else
    {
        result = ss;
    }

    if(ss)
    {
        ::free(ss);
    }

    if(result[result.length()-1] == '*')
    {
        result.erase(result.length()-1);
    }
    std::string::size_type pos = 0;
    while((pos = result.find(", ", pos)) != std::string::npos)
    {
        result.erase(pos+1, 1);
    }

    return result;
}

#endif

std::string igx::stripTypeName(const std::string& classname)
{
#ifdef WIN32
    std::string::size_type pos1 = classname.find_first_of(" ");
        std::string::size_type pos3 = classname.find_last_of(">");
        if(pos3 != std::string::npos)
        {
            return classname.substr(pos1+1, (pos3 - pos1));
        }
        std::string::size_type pos2 = classname.find_last_of(" ");
        return classname.substr(pos1+1, (pos2 - pos1)-1);
#else
    return demangle(classname);
#endif
}

LogManager::LogManager()
: _logStyle(eShowPID | eShowTID | eShowType | eShowTimestamp | eShowFunction)
, _defaultLevel(0)
{
    _symbolicThreadIds[std::this_thread::get_id()] = 0;
    _outfile.open("logfile.txt");
}

LogManager::~LogManager()
{
}

uint64_t LogManager::symbolicThreadId()
{
    uint64_t symId = 0;
    auto sti = _symbolicThreadIds.find(std::this_thread::get_id());
    if (sti != _symbolicThreadIds.end()) {
        symId = sti->second;
    }
    else {
        symId = _symbolicThreadIds.size();
        _symbolicThreadIds[std::this_thread::get_id()] = symId;
    }
    return symId;
}

void LogManager::defaultLevel(int level)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _defaultLevel = level;
    refreshActiveStates();
}

std::string LogManager::detemplatedClassName(const std::string& className)
{
    auto pos = className.find('<');
    return pos != std::string::npos ? className.substr(0,pos) : className;
}

void LogManager::logLevel(const std::string& className, int level)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _logLevels.insert(LogLevels::value_type(detemplatedClassName(className), level));
    refreshActiveStates();
}

bool LogManager::isActive(const std::string& className, int level)
{
    std::lock_guard<std::mutex> lock(_mutex);
    LogLevels::const_iterator i = _logLevels.find(detemplatedClassName(className));
    if(i != _logLevels.end())
    {
        return i->second >= level;
    }
    else
    {
        return _defaultLevel >= level;
    }    
}

void LogManager::logStyle(int logStyle)
{
    _logStyle = logStyle;
}

void LogManager::registerIsActive(const std::string& className, int level, std::atomic<bool>& isInit, volatile bool& isActive)
{
    LogManager* logManager = instance();
    std::lock_guard<std::mutex> lock(logManager->_mutex);
    if (!isInit.load(std::memory_order_relaxed)) {
        std::string name = detemplatedClassName(className);
        isInit.store(true, std::memory_order_release);
        LogPoint point;
        point._level = level;
        point._isActive = &isActive;
        logManager->_logPoints.insert(LogPoints::value_type(name, point));
        LogLevels::const_iterator i = logManager->_logLevels.find(name);
        if(i != logManager->_logLevels.end())
        {
            isActive = i->second >= level;
        }
        else
        {
            isActive = logManager->_defaultLevel >= level;
        }
    }
}

void LogManager::log(LogType type, const std::string& className, const std::string& file, int line, const std::string& function, int level, const std::string& message)
{
    LogManager* logManager = instance();
    if(logManager->isActive(className, level))
    {
        std::lock_guard<std::mutex> lock(logManager->_mutex);
        std::ostringstream ss;
        std::string timestamp;
        std::string logtype;
        int fields = 0;
        ss << "[";
#ifndef WIN32
        if (logManager->_logStyle & eShowPID) {
            ss << getpid() << ":";
        }
        if (logManager->_logStyle & eShowTID) {
            ss << logManager->symbolicThreadId() << ":";
        }
#if 0
#ifdef __APPLE__
        if (logManager->_logStyle & eShowTID) {
            //ss << pthread_mach_thread_np(pthread_self()) << ":";
            ss << std::this_thread::get_id() << ":";
        }
#else
        if (logManager->_logStyle & eShowTID) {
            ss << pthread_self() << ":";
        }
#endif
#endif
#endif
        if (logManager->_logStyle & eShowType) {
            switch (type) {
                case LogType::eError:
                    logtype = "ERROR: ";
                    break;
                case LogType::eWarning:
                    logtype =  "WARNING: ";
                    break;
                case LogType::eInfo:
                    logtype =  "INFO: ";
                    break;
                case LogType::eDebug:
                    logtype =  "DEBUG: ";
                    break;
            }
            ss << logtype;
        }
        if (logManager->_logStyle & eShowTimestamp) {
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
            const time_t time = std::chrono::system_clock::to_time_t(now);
            ss << std::put_time(std::localtime(&time), "%FT%T.") << std::setfill('0') << std::setw(3) << millis;
            std::ostringstream os;
            os << std::put_time(std::localtime(&time), "%T.") << std::setfill('0') << std::setw(3) << millis;
            timestamp = os.str();
            ++fields;
        }
        if (logManager->_logStyle & eShowFile) {
            if (fields++) {
                ss << ", ";
            }
            ss << file;
        }
        if (logManager->_logStyle & eShowLine) {
            if (logManager->_logStyle & eShowFile) {
                ss << ":";
            } else if (fields) {
                ss << ", line ";
            }
            ss << line;
            ++fields;
        }
        if (logManager->_logStyle & eShowFunction) {
            if (fields++) {
                ss << ", ";
            }
            ss << "[" << function << "]";
        }
        ss << "] ";
        AppLog::instance().addLog("[%s] %s%s\n", timestamp.c_str(), logtype.c_str(), message.c_str());
        logManager->_outfile << ss.str() << message << std::endl;
    }
}

LogManager* LogManager::instance()
{
    static LogManager logManager;
    return &logManager;
}

void LogManager::refreshActiveStates()
{
    std::string currentClass;
    bool currentActive = false;
    for(const LogPoints::value_type& logPointInfo: _logPoints)
    {
        if(logPointInfo.first != currentClass)
        {
            currentClass = logPointInfo.first;
            LogLevels::const_iterator i = _logLevels.find(currentClass);
            if(i != _logLevels.end())
            {
                currentActive = i->second >= logPointInfo.second._level;
            }
            else
            {
                currentActive = _defaultLevel >= logPointInfo.second._level;
            }    
        }
        *logPointInfo.second._isActive = currentActive;
    }
}
