//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
#define GHC_FILESYSTEM_IMPLEMENTATION
#include <ghc/filesystem.hpp>

#include <backend/logging.hpp>
#include <backend/system.hpp>
#include <version/version.hpp>
#include <chrono>
#include <ctime>
#include <regex>
#include <stdexcept>

#ifdef __linux__
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>
#elif defined(__APPLE__) && defined(__MACH__)
#include <sys/param.h>
#include <sys/sysctl.h>
#include <fcntl.h>
#include <unistd.h>
#elif _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace fs = ghc::filesystem;

namespace {

static std::string g_appName;
static std::string g_dataPath;

static std::string getSysEnv(const std::string& name)
{
#ifdef _WIN32
    std::wstring result;
    result.resize(65535);
    auto size = ::GetEnvironmentVariableW(fs::detail::fromUtf8<std::wstring>(name).c_str(), &result[0], 65535);
    result.resize(size);
    return fs::detail::toUtf8(result);
#else
    auto env = ::getenv(name.c_str());
    return env ? std::string(env) : "";
#endif
}

static std::string getOS()
{
#ifdef __linux__
    struct utsname unameData;
    int LinuxInfo = uname(&unameData);
    return std::string(unameData.sysname) + " " + unameData.release;
#endif

#ifdef _WIN32
    OSVERSIONINFOEX info;
    ZeroMemory(&info, sizeof(OSVERSIONINFOEX));
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((LPOSVERSIONINFO)&info);
    return std::string("Windows ") + std::to_string(info.dwMajorVersion) + "." + std::to_string(info.dwMinorVersion);
#endif

#if !defined(__linux__) && !defined(_WIN32)
    const std::regex version(R"((\d+).(\d+).(\d+))");
    std::smatch match;
    int mib[2];
    size_t len;
    char* kernelVersion = NULL;
    mib[0] = CTL_KERN;
    mib[1] = KERN_OSRELEASE;

    if (::sysctl(mib, 2, NULL, &len, NULL, 0) < 0) {
        fprintf(stderr, "%s: Error during sysctl probe call!\n", __PRETTY_FUNCTION__);
        fflush(stdout);
        exit(4);
    }

    kernelVersion = static_cast<char*>(malloc(len));
    if (kernelVersion == NULL) {
        fprintf(stderr, "%s: Error during malloc!\n", __PRETTY_FUNCTION__);
        fflush(stdout);
        exit(4);
    }
    if (::sysctl(mib, 2, kernelVersion, &len, NULL, 0) < 0) {
        fprintf(stderr, "%s: Error during sysctl get verstring call!\n", __PRETTY_FUNCTION__);
        fflush(stdout);
        exit(4);
    }
    std::string kernel = kernelVersion;
    free(kernelVersion);
    if(std::regex_match(kernel, match, version)) {
        auto kernelMajor = std::stoi(match[1].str());
        if(kernelMajor >= 20) {
            return std::string("macOS 11.") + std::to_string(kernelMajor-20);
        }
        return std::string("macOS 10.") + std::to_string(kernelMajor-4);
    }
    return "unknown macOS";
#endif
}

static void append2Digits(std::string& s, unsigned val)
{
    if(val < 10) {
        s += '0';
        s += (char)('0'+val);
    }
    else {
        auto v1 = val / 10;
        auto v2 = val % 10;
        s += (char)('0'+v1);
        s += (char)('0'+v2);
    }
}

}

namespace relive {

void setAppName(const std::string& appName)
{
    if(!g_appName.empty()) {
        throw std::runtime_error("Application name set more than once!");
    }
    g_appName = appName;
}

std::string appName()
{
    if(g_appName.empty()) {
        throw std::runtime_error("No application name set!");
    }
    return g_appName;
}

std::string userAgent()
{
    static std::string userAgent = "relive/11 (" + getOS() + ") " + appName() + "/" + RELIVE_VERSION_STRING_LONG;
    return userAgent;
}

int64_t currentTime()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string formattedDuration(int64_t seconds)
{
    std::string result;
    result.reserve(9);
    if(seconds < 0) {
        seconds = std::abs(seconds);
        result += '-';
    }
    else {
        if(seconds / 3600 < 10) {
            result += '0';
        }
    }
    result += std::to_string(seconds / 3600 );
    result +=  ':';
    append2Digits(result, (unsigned)((seconds % 3600)/60));
    result +=  ':';
    append2Digits(result, (unsigned)(seconds % 60));
    return result;
}

std::string formattedTime(int64_t unixTimestamp)
{
    std::string result;
    struct std::tm* ti = nullptr;
    std::time_t t = unixTimestamp;
    result.reserve(9);
    ti = localtime (&t);
    append2Digits(result, 19+ti->tm_year/100);
    append2Digits(result, ti->tm_year%100);
    result += '-';
    append2Digits(result, ti->tm_mon+1);
    result += '-';
    append2Digits(result, ti->tm_mday);
    return result;
}

void dataPath(const std::string& path)
{
    g_dataPath = path;
}

std::string dataPath()
{
    if(!g_dataPath.empty()) {
        fs::create_directories(g_dataPath);
        return g_dataPath;
    }
    std::string dir;
#ifdef GHC_OS_WINDOWS
    auto localAppData = getSysEnv("localappdata");
    if (!localAppData.empty()) {
		dir = fs::path(localAppData) / appName();
	}
    else {
        throw std::runtime_error("Need %localappdata% to create configuration directory!");
	}
#else
    auto home = ::getenv("HOME");
    if(home) {
#ifdef GHC_OS_MACOS
        dir = fs::path(home) / "Library/Application Support" / appName();
#elif defined(GHC_OS_LINUX)
        dir = fs::path(home) / ".local/share" / appName();
#else
#error "Unsupported OS!"
#endif
    }
    else {
        throw std::runtime_error("Need $HOME to create configuration directory!");
    }
#endif
    fs::create_directories(dir);
    return dir;
}
    
bool isInstanceRunning()
{
#ifdef GHC_OS_WINDOWS
    return false;
#else
    auto lockFile = fs::path(dataPath()) / (appName() + ".pid");
    DEBUG_LOG("isInstanceRunning", 1, "PID file " << lockFile);
    auto fd = open(lockFile.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        throw std::runtime_error("Couldn't open lock file: " + lockFile.string());
    }
    else {
        //auto pid = std::to_string(::getpid());
        //write(fd, pid.c_str(), pid.size());
    }
    struct ::flock fl;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    auto rc = ::fcntl(fd, F_SETLK, &fl);
    DEBUG_LOG("isInstanceRunning", 1, "::fcntl() returned " << rc);
    return rc == -1 ? true : false;
#endif
}

}
