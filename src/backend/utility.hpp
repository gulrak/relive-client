//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
#pragma once

#include <string>
#include <type_traits>

namespace relive {

inline std::string trim_left(std::string str)
{
    std::string result(std::move(str));
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return result;
}

inline std::string trim_right(std::string str)
{
    std::string result(std::move(str));
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return result;
}

inline std::string trim(std::string str)
{
    return trim_left(trim_right(str));
}

inline bool starts_with(const std::string& str, const std::string& prefix)
{
    if (prefix.length() > str.length()) {
        return false;
    }
    return std::equal(prefix.begin(), prefix.end(), str.begin());
}

inline bool ends_with(const std::string& str, const std::string& suffix)
{
    if (suffix.length() > str.length()) {
        return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

}  // namespace relive
