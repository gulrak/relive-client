//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------

#include <backend/rldata.hpp>

#define RELIVE_URL_PATTERN "(?:.*?[^0-9a-zA-Z])?(station|stream|track)[^0-9a-zA-Z]([0-9a-zA-Z]+)(?:[^0-9a-zA-Z]([0-9a-zA-Z]+))?(?:[^0-9a-zA-Z]([0-9a-zA-Z]+))?"
const char* base62Chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static std::string base62Encode(int64_t val)
{
    std::string result;
    while(val)
    {
        result = base62Chars[val % 62] + result;
        val /= 62;
    }
    if(result.empty())
        result = "0";
    return result;
}

static int64_t base62Decode(std::string val)
{
    int64_t result = 0;
    for(char digit : val)
    {
        const char* c = strchr(base62Chars, digit);
        if(c)
        {
            result = result * 62 + (c - base62Chars);
        }
    }
    return result;
}


std::string Track::reLiveURL(int64_t offset) const
{
    if(_stream && _stream->_station) {
        return "track-" + base62Encode(_stream->_station->_reliveId) + "-" + base62Encode(_stream->_reliveId) + "-" + base62Encode(offset ? offset : _time);
    }
    return "";
}

std::string Stream::reLiveURL() const
{
    if(_station) {
        return "stream-" + base62Encode(_station->_reliveId) + "-" + base62Encode(_reliveId);
    }
    return "";
}

std::string Station::reLiveURL() const
{
    return "station-" + base62Encode(_reliveId);
}
