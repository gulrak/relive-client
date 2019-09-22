//---------------------------------------------------------------------------------------
//
// relivedb - A C++ implementation of the reLive protocoll and an sqlite backend
//
//---------------------------------------------------------------------------------------
//
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software without
//    specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//---------------------------------------------------------------------------------------

#include "logging.hpp"
#include <ghc/filesystem.hpp>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

#ifndef _WIN32
#include <unistd.h>
#include <pthread.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <sys/_types.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#include <stdlib.h>
#endif

namespace relive {

#if defined(__GNUC__) || defined(__clang__)
    
static std::string demangle(const std::string&  s)
{
    int status(0);
    std::string result;
    
    char* ss = abi::__cxa_demangle(s.c_str(), 0, 0, &status);
    
    if(status == -1) {
        throw std::bad_alloc();
    }
    else if(status == -2) {
        result = s;
    }
    else if(status == -3) {
        throw std::runtime_error("__cxa_demangle returned -3");
    }
    else {
        result = ss;
    }
    
    if(ss) {
        ::free(ss);
    }
    
    if(result[result.length()-1] == '*') {
        result.erase(result.length()-1);
    }
    std::string::size_type pos = 0;
    while((pos = result.find(", ", pos)) != std::string::npos) {
        result.erase(pos+1, 1);
    }
    
    return result;
}
    
#endif
    
std::string stripTypeName(const std::string& classname)
{
#ifdef _WIN32
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
    
LogManager::LogManager(const std::string& file)
: _logStyle(eShowPID | eShowTID | eShowType | eShowTimestamp | eShowFunction)
, _defaultLevel(0)
{
    if(!file.empty()) {
        auto path = ghc::filesystem::u8path(file);
        _file.reset(new ghc::filesystem::ofstream(path));
        _os = _file.get();
    }
    else {
        _os = &std::clog;
    }
    _symbolicThreadIds[std::this_thread::get_id()] = 0;
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

void LogManager::setOutputFile(const std::string& file)
{
    LogManager* logManager = instance(file);
    if(logManager->_os != logManager->_file.get()) {
        throw std::runtime_error("Filename cannot be set after first use of LogManager!");
    }
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
        std::stringstream ss;
        int fields = 0;
        ss << "[";
#ifndef _WIN32
        if (logManager->_logStyle & eShowPID) {
            ss << getpid() << ":";
        }
        if (logManager->_logStyle & eShowTID) {
            ss << logManager->symbolicThreadId() << ":";
        }
#if 0
#if defined(__APPLE__) && defined(__MACH__)
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
                    ss << "ERROR: ";
                    break;
                case LogType::eWarning:
                    ss << "WARNING: ";
                    break;
                case LogType::eInfo:
                    ss << "INFO: ";
                    break;
                case LogType::eDebug:
                    ss << "DEBUG: ";
                    break;
            }
        }
        if (logManager->_logStyle & eShowTimestamp) {
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
            const time_t time = std::chrono::system_clock::to_time_t(now);
            ss << std::put_time(std::localtime(&time), "%FT%T.") << std::setfill('0') << std::setw(3) << millis;
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
        *logManager->_os << ss.str() << message << std::endl;
    }
}

LogManager* LogManager::instance(const std::string& file)
{
    static LogManager logManager(file);
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

}
