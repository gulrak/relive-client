//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

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
    enum Type { eStationAPI, eWeb, eMedia, eStream, eLogo };
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
