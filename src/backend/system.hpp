//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
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
