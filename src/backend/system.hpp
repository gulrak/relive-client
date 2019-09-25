#pragma once

#include <string>
#define GHC_FILESYSTEM_FWD
#include <ghc/filesystem.hpp>

namespace relive {

void setAppName(const std::string& appName);
std::string appName();

std::string userAgent();
int64_t currentTime();
std::string formattedDuration(int64_t seconds);
std::string formattedTime(int64_t unixTimestamp);

void dataPath(const std::string& path);
std::string dataPath();
bool isInstanceRunning();

}
