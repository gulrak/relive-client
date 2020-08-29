//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
#pragma once

#include "rldata.hpp"
#include <ghc/uri.hpp>
#include <pearce/threadpool.hpp>
#include <atomic>
#include <list>
#include <sstream>

namespace relive
{
    
struct Keys
{
    static std::string version;             // backend version that wrote to the db last
    static std::string relive_root_server;  // url of the relive api root server
    static std::string last_relive_sync;    // unix timestamp of the last sync with relive
    static std::string default_station;     // default station to switch to, start with stations view if unset or empty
};

class ReLiveDB
{
public:
    ReLiveDB(std::function<void(int)> progressHandler = std::function<void(int)>(), const ghc::net::uri& master = ghc::net::uri("https://api.relive.nu"));
    ~ReLiveDB();

    template<typename T>
    void setConfigValue(const std::string& key, const T& val)
    {
        std::ostringstream os;
        os << val;
        setConfigValueString(key, os.str());
    }

    template<typename T>
    T getConfigValue(const std::string& key, const T& defaultVal = T())
    {
        std::ostringstream os;
        os << defaultVal;
        std::istringstream is(getConfigValueString(key, os.str()));
        T val;
        is >> val;
        return val;
    }

    void setPlayed(Stream& stream);
    
    void refreshStations(std::function<void()> yield = std::function<void()>(), bool force = false);
    
    std::vector<Station> fetchStations();
    void deepFetch(Station& station, bool withoutStreams = false);
    void deepFetch(Stream& stream, bool parentsOnly = false);
    void deepFetch(Track& track);

    std::vector<Station> findStations(const std::string& pattern);
    std::vector<Stream> findStreams(const std::string& pattern);
    std::vector<Track> findTracks(const std::string& pattern);
    
    std::vector<ChatMessage> fetchChat(const Stream& stream);
    
private:
    void setConfigValueString(const std::string& key, const std::string& value);
    std::string getConfigValueString(const std::string& key, const std::string& defaultValue);
    void refreshStationInfo(const ghc::net::uri& station, int64_t stationId);
    void refreshStreamInfo(const ghc::net::uri& station, int64_t reliveId, int64_t streamId);
    void doRefreshStations();
    void doRefreshStationInfo(const ghc::net::uri& station, int64_t stid);
    void doRefreshStreamInfo(const ghc::net::uri& station, int64_t reliveId, int64_t streamId);
    pearce::ThreadPool _worker;
    std::function<void(int)> _progressHandler;
    std::list<pearce::ThreadPool::TaskFuture<void>> _jobs;
    ghc::net::uri _master;
    using Mutex = std::recursive_mutex;
    Mutex _mutex;
    int64_t _numOfTracks = 0;
    std::atomic_bool _busy;
};

template<>
inline std::string ReLiveDB::getConfigValue<std::string>(const std::string& key, const std::string& defaultVal)
{
    return getConfigValueString(key, defaultVal);
}

}

