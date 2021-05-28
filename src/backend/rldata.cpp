//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------

#include <backend/rldata.hpp>

namespace relive {

#define RELIVE_URL_PATTERN "(?:.*?[^0-9a-zA-Z])?(station|stream|track)[^0-9a-zA-Z]([0-9a-zA-Z]+)(?:[^0-9a-zA-Z]([0-9a-zA-Z]+))?(?:[^0-9a-zA-Z]([0-9a-zA-Z]+))?"
const char* base62Chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

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

}
