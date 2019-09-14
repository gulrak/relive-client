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
        return (_protocol != ns._protocol || _name != ns._name || _flags != ns._flags || _metaInfo != ns._metaInfo);
    }
};
