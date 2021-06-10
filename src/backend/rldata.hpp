//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
#pragma once

#include <backend/utility.hpp>

#include <cstdint>
#include <memory>
#include <regex>
#include <string>
#include <vector>

namespace relive {

struct Track;
struct Stream;
struct Station;

struct KeyValue {
    std::string key;
    std::string value;
};

struct ChatMessage {
    enum MessageType {
        eUnknown, eMessage, eMe, eJoin, eLeave, eQuit, eNick, eTopic, eMode, eKick
    };
    int _time;
    MessageType _type;
    std::vector<std::string> _strings;

    bool hasNick() const {
        return !_strings.empty() && !_strings.front().empty() && _type != ChatMessage::eUnknown;
    }

    std::string nick() const {
        if(!hasNick()) {
            return std::string();
        }
        auto result = _strings.front();
        if(result[result.length()-1] == '@') {
            result.erase(result.length()-1);
        }
        return result;
    }
};

struct Track
{
    enum Type { eDefault = 0, eMusic = 1, eConversation = 2, eJingle = 3, eNarration = 4 };
    enum Flags { eInfoAvailable = 1, ePlayed = 2 };
    int64_t _id = -1;
    int64_t _streamId = 0;
    std::string _name;
    std::string _artist;
    int _type = 0;
    int64_t _time = 0;
    // non api fields:
    int64_t _lastUpdate = 0;
    int _flags = 0;
    std::string _metaInfo;
    
    // deep fetch data:
    int64_t _duration = 0;
    std::shared_ptr<Stream> _stream;
    
    bool needsUpdate(const Track& nt)
    {
        return (_streamId != nt._streamId || _name != nt._name || _artist != nt._artist ||
                _type != nt._type || _time != nt._time || _flags != nt._flags || _metaInfo != nt._metaInfo);
    }

    std::string reLiveURL(int64_t offset = 0) const;
};

struct Stream
{
    enum Flags { eChatAvailable = 1, ePlayed = 2, eHideNewTracks = 4 };
    int64_t _id = -1;
    int64_t _reliveId = 0;
    int64_t _stationId = 0;
    std::string _name;
    std::string _host;
    std::string _description;
    int64_t _timestamp = 0;
    int64_t _duration = 0;
    int64_t _size = 0;
    std::string _format;
    int64_t _mediaOffset = 0;
    int64_t _streamInfoChecksum = 0;
    int64_t _chatChecksum = 0;
    int64_t _mediaChecksum = 0;
    // non api fields:
    int64_t _lastUpdate = 0;
    int _flags = 0;
    std::string _metaInfo;
    
    // deep fetch data:
    std::vector<std::string> _media;
    std::vector<Track> _tracks;
    std::shared_ptr<Station> _station;

    bool needsUpdate(const Stream& ns)
    {
        return (_reliveId != ns._reliveId || _stationId != ns._stationId || _name != ns._name ||
                _host != ns._host || _description != ns._description || _timestamp != ns._timestamp ||
                _duration != ns._duration || _size != ns._size || _format != ns._format || _mediaOffset != ns._mediaOffset ||
                _streamInfoChecksum != ns._streamInfoChecksum || _chatChecksum != ns._chatChecksum || _mediaChecksum != ns._mediaChecksum ||
                _flags != ns._flags || _metaInfo != ns._metaInfo);
    }
    
    int trackIndexForTime(int64_t t)
    {
        int lastIndex = 0;
        int index = 0;
        for(const auto& track : _tracks) {
            if(track._time > t) {
                return lastIndex;
            }
            lastIndex = index++;
        }
        return _tracks.empty() ? 0 : _tracks.size() - 1;
    }

    std::string reLiveURL() const;
};

struct Url
{
    enum Type { eStationAPI, eWeb, eMedia, eLiveStream, eLogo };
    int64_t _id = -1;
    int64_t _ownerId = 0;
    std::string _url;
    // non api fields:
    int64_t _lastUpdate = 0;
    int _type = 0;
    std::string _metaInfo;
    
    bool needsUpdate(const Url& nu)
    {
        return (_ownerId != nu._ownerId || _url != nu._url || _type != nu._type || _metaInfo != nu._metaInfo);
    }
};

struct Station
{
    int64_t _id = -1;
    int64_t _reliveId = 0;
    int _protocol = 0;
    std::string _name;
    // non api fields:
    int64_t _lastUpdate = 0;
    int _flags = 0;
    std::string _metaInfo;
    
    // deep fetch data:
    std::string _webSiteUrl;
    std::vector<std::string> _api;
    std::vector<Stream> _streams;
    
    bool needsUpdate(const Station& ns)
    {
        return (_reliveId != ns._reliveId || _protocol != ns._protocol || _name != ns._name || _flags != ns._flags || _metaInfo != ns._metaInfo);
    }

    std::string reLiveURL() const;
};

extern const char* base62Chars;

inline std::string base62Encode(int64_t val)
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

inline int64_t base62Decode(std::string val)
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


inline auto parseUrl(const std::string& url)
{
    struct Result {
        int64_t stationId{-1};
        int64_t streamId{-1};
        int64_t trackOffset{-1};
    };
    std::regex re("(?:relive:)?(station|stream|track)[^0-9a-zA-Z]([0-9a-zA-Z]+)(?:[^0-9a-zA-Z]([0-9a-zA-Z]+))?(?:[^0-9a-zA-Z]([0-9a-zA-Z]+))?");
    auto text = trim(url);
    if(starts_with(text, "http://") || starts_with(text, "https://")) {
        if(text.find("track") != std::string::npos)
            text.erase(0, text.find("track"));
        else if(text.find("stream") != std::string::npos)
            text.erase(0, text.find("stream"));
        else if(text.find("station") != std::string::npos)
            text.erase(0, text.find("station"));
    }
    std::smatch match;
    Result result;
    if(std::regex_match(text, match, re)) {
        if(match[1] == "track")
        {
            result.stationId = base62Decode(match[2]);
            result.streamId = base62Decode(match[3]);
            result.trackOffset = base62Decode(match[4]);
        }
        else if(match[1] == "stream")
        {
            result.stationId = base62Decode(match[2]);
            result.streamId = base62Decode(match[3]);
        }
        else if(match[1] == "station")
        {
            result.stationId = base62Decode(match[2]);
        }
    }
    return result;
}

}
