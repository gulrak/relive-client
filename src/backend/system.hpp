#pragma once

#include <string>

namespace relive {

void setAppName(const std::string& appName);
std::string appName();

std::string userAgent();
int64_t currentTime();
std::string formattedDuration(int64_t seconds);
std::string formattedTime(int64_t unixTimestamp);

std::string dataPath();
bool isInstanceRunning();

}
