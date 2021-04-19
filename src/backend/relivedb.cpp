//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
#include "logging.hpp"
#include "relivedb.hpp"
#include "rldata.hpp"
#include "system.hpp"
#include <version/version.hpp>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <ghc/uri.hpp>
#include <pearce/threadpool.hpp>
#include <sqlite_orm/sqlite_orm.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <regex>
#include <string>
#include <iostream>
#include <list>
#include <mutex>
#include <utility>

namespace http = httplib;
using json = nlohmann::json;
using namespace sqlite_orm;
namespace fs = ghc::filesystem;

using namespace relive;


static int64_t getTime()
{
    return std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch().count();
}

inline auto initStorage(const std::string& dbfile)
{
    return make_storage(dbfile,
                        make_table("config_values",
                                   make_column("key", &KeyValue::key, primary_key()),
                                   make_column("value", &KeyValue::value)
                                   ),
                        make_table("stations",
                                   make_column("id", &Station::_id, autoincrement(), primary_key()),
                                   make_column("relive_id", &Station::_reliveId),
                                   make_column("protocol", &Station::_protocol),
                                   make_column("name", &Station::_name),
                                   make_column("last_update", &Station::_lastUpdate),
                                   make_column("flags", &Station::_flags),
                                   make_column("meta_info", &Station::_metaInfo)
                                   ),
                        make_index("idx_stream_name", &Stream::_name),
                        make_index("idx_stream_host", &Stream::_host),
                        make_table("streams",
                                   make_column("id", &Stream::_id, autoincrement(), primary_key()),
                                   make_column("relive_id", &Stream::_reliveId),
                                   make_column("station_id", &Stream::_stationId),
                                   make_column("name", &Stream::_name),
                                   make_column("host", &Stream::_host),
                                   make_column("description", &Stream::_description),
                                   make_column("timestamp", &Stream::_timestamp),
                                   make_column("duration", &Stream::_duration),
                                   make_column("size", &Stream::_size),
                                   make_column("format", &Stream::_format),
                                   make_column("media_offset", &Stream::_mediaOffset),
                                   make_column("info_chk", &Stream::_streamInfoChecksum),
                                   make_column("chat_chk", &Stream::_chatChecksum),
                                   make_column("media_chk", &Stream::_mediaChecksum),
                                   make_column("last_update", &Stream::_lastUpdate),
                                   make_column("flags", &Stream::_flags),
                                   make_column("meta_info", &Stream::_metaInfo),
                                   foreign_key(&Stream::_stationId).references(&Station::_id).on_delete.cascade()
                                   ),
                        make_table("urls",
                                   make_column("id", &Url::_id, autoincrement(), primary_key()),
                                   make_column("owner_id", &Url::_ownerId),
                                   make_column("url", &Url::_url),
                                   make_column("last_update", &Url::_lastUpdate),
                                   make_column("type", &Url::_type),
                                   make_column("meta_info", &Url::_metaInfo)
                                   ),
                        make_index("idx_track_name", &Track::_name),
                        make_index("idx_tack_artist", &Track::_artist),
                        make_table("tracks",
                                   make_column("id", &Track::_id, autoincrement(), primary_key()),
                                   make_column("stream_id", &Track::_streamId),
                                   make_column("name", &Track::_name),
                                   make_column("artist", &Track::_artist),
                                   make_column("type", &Track::_type),
                                   make_column("time", &Track::_time),
                                   make_column("last_update", &Track::_lastUpdate),
                                   make_column("flags", &Track::_flags),
                                   make_column("meta_info", &Track::_metaInfo),
                                   foreign_key(&Track::_streamId).references(&Stream::_id).on_delete.cascade()
                                   )
                        );
}

using Storage = decltype(initStorage(""));

static Storage& storage()
{
    static Storage _storage = initStorage(fs::path(dataPath()) / "relive.sqlite");
    return _storage;
}

// key-value config keys
std::string Keys::version = "version";
std::string Keys::relive_root_server = "relive_root_server";
std::string Keys::last_relive_sync = "last_relive_sync";
std::string Keys::default_station = "default_station";
std::string Keys::play_position = "play_position";

ReLiveDB::ReLiveDB(std::function<void(int)> progressHandler, const ghc::net::uri& master)
: _worker(8)
, _progressHandler(progressHandler)
, _master(master)
, _busy(false)
{
    DEBUG_LOG(1, "Database location: " << (fs::path(dataPath()) / "relive.sqlite"));
    auto versionNum = RELIVE_VERSION_MAJOR * 10000 + RELIVE_VERSION_MINOR * 100 + RELIVE_VERSION_PATCH;
    auto dbVersion = getConfigValue(Keys::version, 0);
    if(versionNum < dbVersion) {
        throw std::runtime_error("Database version is newer (" + std::to_string(dbVersion) + ") than this applications version (" + std::to_string(versionNum) + ")!");
    }
    storage().sync_schema(true);
    setConfigValue(Keys::version, versionNum);
    _master = ghc::net::uri(getConfigValue(Keys::relive_root_server, _master.str()));
}

ReLiveDB::~ReLiveDB()
{
}

void ReLiveDB::setConfigValueString(const std::string& key, const std::string& value)
{
    using namespace sqlite_orm;
    KeyValue kv{key, value};
    std::lock_guard<Mutex> lock{_mutex};
    storage().replace(kv);
}

std::string ReLiveDB::getConfigValueString(const std::string& key, const std::string& defaultValue)
{
    using namespace sqlite_orm;
    std::lock_guard<Mutex> lock{_mutex};
    try {
        if(auto kv = storage().get_pointer<KeyValue>(key)) {
            return kv->value;
        }
        else {
            return defaultValue;
        }
    }
    catch(const std::system_error&) {
        return defaultValue;
    }
}

void ReLiveDB::setPlayed(Stream& stream)
{
    if(!(stream._flags & Stream::ePlayed)) {
        std::lock_guard<Mutex> lock{_mutex};
        stream._flags |= Stream::ePlayed;
        storage().update(stream);
    }
}

std::vector<Station> ReLiveDB::fetchStations()
{
    std::lock_guard<Mutex> lock{_mutex};
    return storage().get_all<Station>();
}

void ReLiveDB::deepFetch(Station& station, bool withoutStreams)
{
    std::lock_guard<Mutex> lock{_mutex};
    if(!withoutStreams) {
        station._streams = storage().get_all<Stream>(where(c(&Stream::_stationId) == station._id), order_by(&Stream::_timestamp).desc());
    }
    auto webSites = storage().get_all<Url>(where(c(&Url::_ownerId) == station._id and c(&Url::_type) == int(Url::eWeb)));
    if(!webSites.empty()) {
        station._webSiteUrl = webSites.front()._url;
    }
    station._api.clear();
    auto apis = storage().get_all<Url>(where(c(&Url::_ownerId) == station._id and c(&Url::_type) == int(Url::eStationAPI)));
    for(const auto& api : apis ) {
        station._api.push_back(api._url);
    }
}

void ReLiveDB::deepFetch(Stream& stream, bool parentsOnly)
{
    {
        std::lock_guard<Mutex> lock{_mutex};
        if(!parentsOnly) {
            stream._tracks = storage().get_all<Track>(where(c(&Track::_streamId) == stream._id), order_by(&Track::_time));
            if(!stream._tracks.empty()) {
                for(int i = 0; i < stream._tracks.size() - 1; ++i) {
                    stream._tracks[i]._duration = stream._tracks[i+1]._time - stream._tracks[i]._time;
                }
                stream._tracks[stream._tracks.size()-1]._duration = stream._duration - stream._tracks[stream._tracks.size()-1]._time;
            }
        }
        auto station = storage().get_pointer<Station>(stream._stationId);
        if(station) {
            stream._station = std::make_shared<Station>(*station);
        }
        else {
            stream._station.reset();
        }
    }
    if(stream._station) {
        deepFetch(*stream._station, true);
    }
}

void ReLiveDB::deepFetch(Track& track)
{
    {
        std::lock_guard<Mutex> lock{_mutex};
        track._stream.reset();
        auto stream = storage().get_pointer<Stream>(track._streamId);
        if(stream) {
            track._stream = std::make_shared<Stream>(*stream);
        }
    }
    if(track._stream) {
        deepFetch(*track._stream, true);
    }
}

std::vector<Station> ReLiveDB::findStations(const std::string& pattern)
{
    std::lock_guard<Mutex> lock{_mutex};
    return storage().get_all<Station>(where(like(&Station::_name, pattern)));
}

std::vector<Stream> ReLiveDB::findStreams(const std::string& pattern)
{
    std::lock_guard<Mutex> lock{_mutex};
    return storage().get_all<Stream>(where(like(&Stream::_name, pattern) or like(&Stream::_host, pattern)));
}

std::vector<Track> ReLiveDB::findTracks(const std::string& pattern)
{
    std::lock_guard<Mutex> lock{_mutex};
    return storage().get_all<Track>(where(like(&Track::_name, pattern) or like(&Track::_artist, pattern)));
}

std::vector<ChatMessage> ReLiveDB::fetchChat(const Stream& stream)
{
    std::vector<ChatMessage> chat;
    Stream tstream = stream;
    deepFetch(tstream, true);
    auto station = tstream._station;
    if(station && !station->_api.empty()) {
        auto uri = ghc::net::uri(station->_api[0]);
        DEBUG_LOG(2, uri.request_path() << "/getstreamchat?v=11&streamid=" << stream._reliveId);
        http::Headers headers = {
            { "User-Agent", relive::userAgent() }
        };
        std::shared_ptr<http::Response> res;
        if(uri.scheme() == "https") {
            http::SSLClient cli(uri.host().c_str(), uri.port());
            cli.enable_server_certificate_verification(false);
            res = cli.Get((uri.request_path() + "/getstreamchat?v=11&streamid="+std::to_string(stream._reliveId)).c_str());
        }
        else {
            http::Client cli(uri.host().c_str(), uri.port());
            res = cli.Get((uri.request_path() + "/getstreamchat?v=11&streamid="+std::to_string(stream._reliveId)).c_str());
        }
        if (res && res->status == 200) {
            try {
                auto result = json::parse(res->body);
                chat.reserve(result.at("messages").size());
                for(const auto& message : result.at("messages")) {
                    ChatMessage msg;
                    msg._time = message.at("time").get<int64_t>();
                    auto type = message.at("messageType").get<std::string>();
                    if(type == "Message") {
                        msg._type = ChatMessage::eMessage;
                    }
                    else if(type == "Me") {
                        msg._type = ChatMessage::eMe;
                    }
                    else if(type == "Join") {
                        msg._type = ChatMessage::eJoin;
                    }
                    else if(type == "Leave") {
                        msg._type = ChatMessage::eLeave;
                    }
                    else if(type == "Quit") {
                        msg._type = ChatMessage::eQuit;
                    }
                    else if(type == "Nick") {
                        msg._type = ChatMessage::eNick;
                    }
                    else if(type == "Topic") {
                        msg._type = ChatMessage::eTopic;
                    }
                    else if(type == "Mode") {
                        msg._type = ChatMessage::eMode;
                    }
                    else if(type == "Kick") {
                        msg._type = ChatMessage::eKick;
                    }
                    else {
                        msg._type = ChatMessage::eUnknown;
                    }
                    msg._strings.reserve(message.at("strings").size());
                    for(const auto& str : message.at("strings")) {
                        msg._strings.push_back(str);
                    }
                    chat.push_back(msg);
                }
            }
            catch(const json::exception& ex) {
                ERROR_LOG(0, "JSON exception: " << ex.what());
            }
        }
    }
    return chat;
}

void ReLiveDB::refreshStations(std::function<void()> yield, bool force)
{
    using namespace std::chrono_literals;
    if(_busy) {
        return;
    }
    _busy = true;
    std::shared_ptr<void> doneGuard(nullptr, [&](void*){ _busy = false; });
    auto lastFetch = getConfigValue(Keys::last_relive_sync, INT64_C(0));
    auto now = currentTime();
    if(!force && now - lastFetch < 7200) {
        DEBUG_LOG(2, "skipped refreshStations because last fetch was " << formattedDuration(now - lastFetch) << " ago");
        return;
    }
    DEBUG_LOG(1, "refreshStations start...");
    {
        std::lock_guard<Mutex> lock{_mutex};
        _jobs.emplace_back(_worker.submit([this](){ doRefreshStations(); }));
    }
    int maxJobs = 0;
    while(true) {
        if(yield) {
            yield();
        }
        else {
            std::this_thread::sleep_for(500ms);
        }
        {
            std::lock_guard<Mutex> lock{_mutex};
            if(_jobs.size() > maxJobs) {
                maxJobs = _jobs.size();
            }
            if(maxJobs && _progressHandler) {
                _progressHandler((maxJobs - _jobs.size()) * 100 / maxJobs);
            }
            for(auto iter = _jobs.begin(); iter != _jobs.end();) {
                if(iter->isReady()) {
                    iter = _jobs.erase(iter);
                }
                else {
                    ++iter;
                }
            }
            DEBUG_LOG(3, "jobs: " << _jobs.size() << ", tracks: " << _numOfTracks);
            if(_jobs.empty() && !_worker.workLeft()) {
                break;
            }
        }
    }
    setConfigValue(Keys::last_relive_sync, now);
    if(_progressHandler) {
        _progressHandler(0);
    }
    DEBUG_LOG(1, "Found " << _numOfTracks << " tracks");
    DEBUG_LOG(1, "refreshStations done");
}

void ReLiveDB::doRefreshStations()
{
    http::Headers headers = {
        { "User-Agent", relive::userAgent() }
    };
    std::shared_ptr<http::Response> res;
    if(_master.scheme() == "https") {
        http::SSLClient cli(_master.host().c_str(), _master.port());
        cli.enable_server_certificate_verification(false);
        res = cli.Get("/getstations/?v=11");
    }
    else {
        http::Client cli(_master.host().c_str(), _master.port());
        res = cli.Get("/getstations/?v=11");
    }
    if (res && res->status == 200) {
        try {
            auto result = json::parse(res->body);
            auto now = getTime();
            for(const auto& station : result.at("stations")) {
                int64_t stationId;
                DEBUG_LOG(3, station.at("name").get<std::string>());
                std::string apiServer;
                {
                    std::lock_guard<Mutex> lock{_mutex};
                    Station st{-1, station.at("id").get<int64_t>(), 11, station.at("name").get<std::string>(), now, 0, ""};
                    auto oldStations = storage().get_all<Station>(where(c(&Station::_name) == st._name));
                    if(oldStations.empty()) {
                        storage().begin_transaction();
                        stationId = storage().insert(st);
                        for(const auto& server : station.at("servers")) {
                            if(apiServer.empty()) {
                                apiServer = server.get<std::string>();
                            }
                            Url serverUrl{-1, stationId, server.get<std::string>(), now, Url::eStationAPI, ""};
                            storage().insert(serverUrl);
                        }
                        storage().commit();
                    }
                    else {
                        stationId = oldStations.front()._id;
                        if(oldStations.front().needsUpdate(st)) {
                            st._id = stationId;
                            storage().update(st);
                        }
                        auto apis = storage().get_all<Url>(where(c(&Url::_ownerId) == stationId and c(&Url::_type) == int(Url::eStationAPI)));
                        if(!apis.empty()) {
                            apiServer = apis.front()._url;
                        }
                    }
                }
                if(!apiServer.empty()) {
                    refreshStationInfo(ghc::net::uri(apiServer), stationId);
                }
            }
        }
        catch(const json::exception& ex) {
            ERROR_LOG(0, "JSON exception: " << ex.what());
        }
        catch(const std::system_error& ex) {
            ERROR_LOG(0, "SQLite exception: " << ex.what());
        }
    }
    else {
        ERROR_LOG(0, "Couldn't reach " << _master.host() << ":" << _master.port() << "/getstations/?v=11");
    }
}

void ReLiveDB::refreshStationInfo(const ghc::net::uri& station, int64_t stationId)
{
    std::lock_guard<Mutex> lock{_mutex};
    _jobs.push_back(_worker.submit([this](const ghc::net::uri url, int64_t station_id){ doRefreshStationInfo(url, station_id); }, station, stationId));
}

void ReLiveDB::doRefreshStationInfo(const ghc::net::uri& station, int64_t stationId)
{
    DEBUG_LOG(2, station.request_path() << "getstationinfo?v=11");
    http::Headers headers = {
        { "User-Agent", relive::userAgent() }
    };
    std::shared_ptr<http::Response> res;
    if(station.scheme() == "https") {
        http::SSLClient cli(station.host().c_str(), station.port());
        cli.enable_server_certificate_verification(false);
        res = cli.Get((station.request_path() + "getstationinfo?v=11").c_str());
    }
    else {
        http::Client cli(station.host().c_str(), station.port());
        res = cli.Get((station.request_path() + "getstationinfo?v=11").c_str());
    }
    if (res && res->status == 200) {
        try {
            auto result = json::parse(res->body);
            std::string stationName = result["stationName"].get<std::string>();
            DEBUG_LOG(2, stationName << ": " << result["streams"].size() << " streams");
            {
                auto now = getTime();
                {
                    std::lock_guard<Mutex> lock{_mutex};
                    Station st = storage().get<Station>(stationId);
                    st._protocol = result.at("version").get<int>();
                    st._lastUpdate = now;
                    storage().update(st);
                    auto oldWeb = storage().get_all<Url>(where(c(&Url::_ownerId) == stationId and c(&Url::_type) == int(Url::eWeb)));
                    Url stationWeb{-1, stationId, result.at("webSiteUrl").get<std::string>(), now, Url::eWeb, ""};
                    if(oldWeb.empty()) {
                        storage().insert(stationWeb);
                    }
                    else if(oldWeb.front().needsUpdate(stationWeb)) {
                        stationWeb._id = oldWeb.front()._id;
                        storage().update(stationWeb);
                    }
                }
                for(const auto& stream : result.at("streams")) {
                    Stream s;
                    int64_t streamId;
                    int64_t reliveId;
                    {
                        std::lock_guard<Mutex> lock{_mutex};
                        reliveId = stream.at("id").get<int64_t>();
                        s = Stream{
                            -1, reliveId, stationId, stream.at("streamName").get<std::string>(), stream.at("hostName").get<std::string>(),
                            stream.at("description").get<std::string>(), stream.at("timestamp").get<int64_t>(), stream.at("duration").get<int64_t>(),
                            stream.at("size").get<int64_t>(), stream.at("mediaDataFormat").get<std::string>(),
                            stream.at("mediaDataOffset").get<int64_t>(),
                            stream.at("checksumStreamInfoData").get<int64_t>(), stream.at("checksumChatData").get<int64_t>(), stream.at("checksumMediaData").get<int64_t>(),
                            getTime(), 0, ""
                        };
                        auto oldStream = storage().get_all<Stream>(where(c(&Stream::_reliveId) == reliveId and c(&Stream::_stationId) == stationId));
                        if(oldStream.empty()) {
                            streamId = storage().insert(s);
                            for(const auto& mediaDirect : stream.at("mediaDirectUrls")) {
                                Url mediaUrl{-1, streamId, mediaDirect.get<std::string>(), now, Url::eMedia, ""};
                                storage().insert(mediaUrl);
                            }
                            refreshStreamInfo(station, reliveId, streamId);
                        }
                        else {
                            streamId = oldStream.front()._id;
                            if(oldStream.front().needsUpdate(s)) {
                                s._id = streamId;
                                if(oldStream.front()._flags & Stream::ePlayed) {
                                    s._flags |= Stream::ePlayed;
                                }
                                storage().update(s);
                                if(oldStream.front()._streamInfoChecksum != s._streamInfoChecksum) {
                                    storage().remove_all<Track>(where(c(&Track::_streamId) == streamId));
                                    refreshStreamInfo(station, reliveId, streamId);
                                }
                            }
                        }
                    }
                }
            }
        }
        catch(const json::exception& ex) {
            ERROR_LOG(0, "JSON exception: " << ex.what());
        }
        catch(const std::system_error& ex) {
            ERROR_LOG(0, "SQLite exception: " << ex.what());
        }
    }
    else {
        ERROR_LOG(0, "Error while fetching " << station.str());
    }
}

void ReLiveDB::refreshStreamInfo(const ghc::net::uri& station, int64_t reliveId, int64_t streamId)
{
    std::lock_guard<Mutex> lock{_mutex};
    _jobs.push_back(_worker.submit([this](const ghc::net::uri s, int64_t relive_id, int64_t stream_id){ doRefreshStreamInfo(s, relive_id, stream_id); }, station, reliveId, streamId));
}

void ReLiveDB::doRefreshStreamInfo(const ghc::net::uri& station, int64_t reliveId, int64_t streamId)
{
    DEBUG_LOG(2, station.request_path() << "getstationinfo?v=11");
    http::Headers headers = {
        { "User-Agent", relive::userAgent() }
    };
    std::shared_ptr<http::Response> res;
    if(station.scheme() == "https") {
        http::SSLClient cli(station.host().c_str(), station.port());
        cli.enable_server_certificate_verification(false);
        res = cli.Get((station.request_path() + "getstreaminfo?v=11&streamid=" + std::to_string(reliveId)).c_str());
    }
    else {
        http::Client cli(station.host().c_str(), station.port());
        res = cli.Get((station.request_path() + "getstreaminfo?v=11&streamid=" + std::to_string(reliveId)).c_str());
    }
    if (res && res->status == 200) {
        try {
            auto result = json::parse(res->body);
            auto info = res->body;
            std::lock_guard<Mutex> lock{_mutex};
            storage().begin_transaction();
            for(const auto& track : result.at("tracks")) {
                int type = 0;
                auto typeStr = track["trackType"].get<std::string>();
                if(typeStr == "Music") {
                    type = 1;
                }
                else if(typeStr == "Conversation") {
                    type = 2;
                }
                else if(typeStr == "Jingle") {
                    type = 3;
                }
                else if(typeStr == "Narration") {
                    type = 4;
                }
                Track t{-1, streamId, track["trackName"].get<std::string>(), track["artistName"].get<std::string>(), type, track["time"].get<int64_t>(), getTime(), 0 };
                auto oldTrack = storage().get_all<Track>(where(c(&Track::_streamId) == streamId and c(&Track::_time) == t._time));
                if(oldTrack.empty()) {
                    storage().insert(t);
                }
                else {
                    if(oldTrack.front().needsUpdate(t)) {
                        t._id = oldTrack.front()._id;
                        storage().update(t);
                    }
                }
            }
            storage().commit();
            DEBUG_LOG(3, "    " << result["tracks"].size());
            _numOfTracks += result["tracks"].size();
        }
        catch(const json::exception& ex) {
            ERROR_LOG(0, "JSON exception: " << ex.what());
        }
        catch(const std::system_error& ex) {
            ERROR_LOG(0, "SQLite exception: " << ex.what());
        }
    }
    else {
        ERROR_LOG(0, "Error while fetching " << station.str() << " - Stream: " << streamId);
    }
}

