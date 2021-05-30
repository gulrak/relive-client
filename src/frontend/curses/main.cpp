//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
#include <backend/logging.hpp>
#include <backend/relivedb.hpp>
#include <backend/player.hpp>
#include <backend/system.hpp>
#include <ghc/cui.hpp>
#include <ghc/options.hpp>
#include <version/version.hpp>
#if defined(RELIVE_RTAUDIO_BACKEND)
#include <RtAudio.h>
#elif defined(RELIVE_MINIAUDIO_BACKEND)
#include <mackron/miniaudio.h>
#endif
#include <atomic>
#include <csignal>
#include <iostream>
#include <thread>


using namespace relive;
using namespace std::string_literals;

#define RELIVE_APP_NAME "reLiveCUI"

#include "info.hpp"

static const std::map<std::string, std::string> g_radioURLs = {
    {"SceneSat", "https://scenesat.com/listen/normal/mid.m3u"},
    {"SceneSat [port 80]", "https://scenesat.com/listen/port80/mid.m3u"},
    {"SLAY Radio", "https://www.slayradio.org/tune_in.php/128kbps/slayradio.128.m3u"}
};

static const std::map<int, std::string> g_fmenu = {
    { 1, "Stations"},
    { 2, "Streams"},
    { 3, "Tracks"},
    { 4, "Chat"},
    { 5, "Radio"},
    { 6, "Config"},
    { 7, "Info" },
    {10, "Quit"}
};

namespace {

static void sigintHandler(int sig_num) 
{ 
    std::signal(SIGINT, sigintHandler);
}

}

class StationsModel : public ghc::cui::list_view::model
{
public:
    ~StationsModel() override {}
    int size() const override
    {
        return _stations.size();
    }
    std::vector<ghc::cui::cell> line(int index, int width) const override
    {
        std::vector<ghc::cui::cell> result;
        if(index < 0) {
            result.emplace_back(ghc::cui::cell::eLeft, 30, 0, "Station Name");
            result.emplace_back(ghc::cui::cell::eRight, 7, 0, "Streams");
            result.emplace_back(ghc::cui::cell::eLeft, 40, 0, "URL");
        }
        else if (index < _stations.size()) {
            const Station& station = _stations[index];
            int attr = 0;
            if(station._id == _activeStation) {
                attr = A_BOLD;
            }
            result.emplace_back(ghc::cui::cell::eLeft, 30, attr, station._name);
            result.emplace_back(ghc::cui::cell::eRight, 7, attr, std::to_string(station._streams.size()));
            result.emplace_back(ghc::cui::cell::eLeft, 40, attr, station._webSiteUrl);
        }
        return result;
    }
    
    std::vector<Station> _stations;
    int64_t _activeStation = 0;
};

class StreamsModel : public ghc::cui::list_view::model
{
public:
    ~StreamsModel() override {}
    int size() const override
    {
        return _streams.size();
    }
    std::vector<ghc::cui::cell> line(int index, int width) const override
    {
        std::vector<ghc::cui::cell> result;
        if(index < 0) {
            result.emplace_back(ghc::cui::cell::eRight,   1, 0, " ");
            result.emplace_back(ghc::cui::cell::eCenter, 10, 0, "Date");
            result.emplace_back(ghc::cui::cell::eLeft,   35, 0, "Hosts");
            result.emplace_back(ghc::cui::cell::eLeft,   40, 0, "Title");
            result.emplace_back(ghc::cui::cell::eRight,   8, 0, "Duration");
            result.emplace_back(ghc::cui::cell::eCenter,  4, 0, "Chat");
        }
        if(index < _streams.size()) {
            const Stream& stream = _streams[index];
            int attr = 0;
            if(stream._id == _activeStream) {
                attr = A_BOLD;
            }
            result.emplace_back(ghc::cui::cell::eRight,  1, attr, stream._id == _activeStream ? ">" : " ");
            result.emplace_back(ghc::cui::cell::eLeft,  10, attr, formattedDate(stream._timestamp));
            result.emplace_back(ghc::cui::cell::eLeft,  35, attr, stream._host);
            result.emplace_back(ghc::cui::cell::eLeft,  40, attr, stream._name);
            result.emplace_back(ghc::cui::cell::eRight,  8, attr, formattedDuration(stream._duration));
            result.emplace_back(ghc::cui::cell::eCenter, 4, attr, stream._chatChecksum ? "\u2713" : "-");
        }
        return result;
    }
    
    std::vector<Stream> _streams;
    int64_t _activeStream = 0;
};

class TracksModel : public ghc::cui::list_view::model
{
public:
    ~TracksModel() override {}
    int size() const override
    {
        return _tracks.size();
    }
    std::vector<ghc::cui::cell> line(int index, int width) const override
    {
        std::vector<ghc::cui::cell> result;
        static const char* types[] = {
            "-",
            "\u266B",
            "\u263A",
            "\u266A",
            "\u263A"
        };
        if(index < 0) {
            result.emplace_back(ghc::cui::cell::eRight,  1, 0, " ");
            result.emplace_back(ghc::cui::cell::eCenter, 8, 0, "Time");
            result.emplace_back(ghc::cui::cell::eLeft,  30, 0, "Artist");
            result.emplace_back(ghc::cui::cell::eLeft,  45, 0, "Title");
            result.emplace_back(ghc::cui::cell::eRight,  8, 0, "Duration");
            result.emplace_back(ghc::cui::cell::eCenter, 4, 0, "Type");
        }
        if(index < _tracks.size()) {
            const Track& track = _tracks[index];
            int attr = 0;
            if(track._id == _activeTrack) {
                attr = A_BOLD;
            }
            auto type = track._type > 0 && track._type <= 4 ? types[track._type] : "-";
            result.emplace_back(ghc::cui::cell::eRight,  1, attr, track._id == _activeTrack ? ">" : " ");
            result.emplace_back(ghc::cui::cell::eRight,  8, attr, formattedDuration(track._time));
            result.emplace_back(ghc::cui::cell::eLeft,  30, attr, track._artist);
            result.emplace_back(ghc::cui::cell::eLeft,  45, attr, track._name);
            result.emplace_back(ghc::cui::cell::eRight,  8, attr, formattedDuration(track._duration));
            result.emplace_back(ghc::cui::cell::eCenter, 4, attr, type);
        }
        return result;
    }
    
    std::shared_ptr<Stream> _stream;
    std::vector<Track> _tracks;
    int64_t _activeTrack = 0;
};

class ChatModel : public ghc::cui::log_view::model
{
public:
    ~ChatModel() override {}
    int size() const override
    {
        return _chat.size();
    }
    std::vector<ghc::cui::cell> line(int index, int width) const override
    {
        std::vector<ghc::cui::cell> result;
        if(index >= 0 && index < _chat.size()) {
            const auto& msg = _chat[index];
            result.emplace_back(ghc::cui::cell::eLeft, 10, 0, "[" + formattedDuration(msg._time) + "]");
            switch(msg._type) {
                case ChatMessage::eMe:
                case ChatMessage::eMode:
                case ChatMessage::eKick:
                    result.emplace_back(ghc::cui::cell::eRight, _nickLen, A_BOLD, "*"+msg._strings.front());
                    result.emplace_back(ghc::cui::cell::eLeft, 0, A_BOLD,  msg._strings.back());
                    break;
                case ChatMessage::eNick:
                    result.emplace_back(ghc::cui::cell::eRight, _nickLen, A_BOLD, "*"+msg._strings.front());
                    result.emplace_back(ghc::cui::cell::eLeft, 0, A_BOLD, "is now known as " + msg._strings.back());
                    break;
                case ChatMessage::eJoin:
                    result.emplace_back(ghc::cui::cell::eRight, _nickLen, A_BOLD, "*"+msg._strings.front());
                    result.emplace_back(ghc::cui::cell::eLeft, 0, A_BOLD, "has joined the channel");
                    break;
                case ChatMessage::eLeave:
                    result.emplace_back(ghc::cui::cell::eRight, _nickLen, A_BOLD, "*"+msg._strings.front());
                    result.emplace_back(ghc::cui::cell::eLeft, 0, A_BOLD, "has left the channel " + (msg._strings.size()>1 ? "("+msg._strings[1]+")":""));
                    break;
                case ChatMessage::eQuit:
                    result.emplace_back(ghc::cui::cell::eRight, _nickLen, A_BOLD, "*"+msg._strings.front());
                    result.emplace_back(ghc::cui::cell::eLeft, 0, A_BOLD, "has quit " + (msg._strings.size()>1 ? "("+msg._strings[1]+")":""));
                    break;
                case ChatMessage::eTopic:
                    result.emplace_back(ghc::cui::cell::eRight, _nickLen, A_BOLD, "*"+msg._strings.front());
                    result.emplace_back(ghc::cui::cell::eLeft, 0, A_BOLD, "has changed the topic to: " + msg._strings.back());
                    break;
                default:
                    if(msg._strings.size() == 1) {
                        result.emplace_back(ghc::cui::cell::eRight, _nickLen, 0, "");
                    }
                    else {
                        result.emplace_back(ghc::cui::cell::eRight, _nickLen, 0, msg._strings.front()+":");
                    }
                    result.emplace_back(ghc::cui::cell::eLeft, 0, 0, msg._strings.back());
                    break;
            }
        }
        return result;
    }
    void rescan()
    {
        int maxNick = 8;
        for(const auto& msg : _chat) {
            if(!msg._strings.empty() && msg._type != ChatMessage::eUnknown && msg._strings.front().size() > maxNick) {
                auto l = ghc::cui::detail::utf8Length(msg._strings.front());
                if(l > maxNick) {
                    maxNick = l;
                }
            }
        }
        _nickLen = maxNick + 1;
    }
    std::vector<ChatMessage> _chat;
    int _nickLen = 10;
};

class ReLiveCUI : public ghc::cui::application
{
public:
    enum ActiveMainView { eNone, eStationList, eStreamList, eTrackList, eChat, eRadio, eInfo, eConfig };
    ReLiveCUI(int argc, char* argv[], ghc::options& parser)
        : ghc::cui::application(argc, argv)
        , _rdb([&](int percent){ progress(percent); })
        , _lastFetch(0)
        , _activeMain(eNone)
        , _needsRefresh(false)
        , _progress(0)
        , _parser(parser)
    {
        std::signal(SIGINT, sigintHandler);
        _title = " Stations List ";
        calculatePlayBar();
        fetchStations();
        auto defaultStation = _rdb.getConfigValue(Keys::default_station, std::string());
        if(!selectStation(defaultStation)) {
            updateMainWindow(eStationList);
        }
        drawPlayer();
        drawBorders();
        ::refresh();
    }

    bool validTerminal()
    {
        return width() >= 40 && height() >= 10;
    }
    
    bool selectStation(const std::string& name)
    {
        DEBUG_LOG(1, "Switching to default station '" << name << "'");
        for(const auto& station : _stationsModel._stations) {
            DEBUG_LOG(2, "comparing to '" << station._name << "'");
            if(station._name == name) {
                DEBUG_LOG(2, "found '" << station._name << "'");
                _stationsModel._activeStation = station._id;
                _streamsModel._streams = station._streams;
                _tracksModel.select(0);
                updateMainWindow(eStreamList);
                return true;
            }
        }
        return false;
    }
    
    void on_redraw() override
    {
        _main->redraw();
        drawPlayer();
        drawBorders();
    }

    void on_idle() override
    {
        static int lastPlayPos = 0;
        if(currentTime() - _lastFetch > 3600) {
            _rdb.refreshStations([this](){ yield(); });
            fetchStations();
            updateMainWindow(_activeMain, true);
            _lastFetch = currentTime();
        }
        auto playTime = _player.playTime();
        if(playTime != lastPlayPos) {
            lastPlayPos = playTime;
            auto stream = _player.currentStream();
            if(stream) {
                if(_tracksModel._stream && stream->_id == _tracksModel._stream->_id) {
                    // find active track
                    int64_t lastId = 0;
                    for(const auto& track : _tracksModel._tracks) {
                        if(track._time > playTime) {
                            if(_tracksModel._activeTrack != lastId) {
                                _tracksModel._activeTrack = lastId;
                                _needsRefresh = true;
                            }
                            break;
                        }
                        lastId = track._id;
                    }
                    if(_tracksModel._activeTrack != lastId) {
                        _tracksModel._activeTrack = lastId;
                        _needsRefresh = true;
                    }
                }
                else {
                    if(_tracksModel._activeTrack != 0) {
                        _tracksModel._activeTrack = 0;
                        _needsRefresh = true;
                    }
                }
                int chatPos = -1;
                for(const auto& msg : _chatModel._chat) {
                    if(msg._time <= playTime) {
                        ++chatPos;
                    }
                    else {
                        break;
                    }
                }
                _chatModel.position(chatPos);
                if(_activeMain == eChat) {
                    _needsRefresh = true;
                }
            }
        }
        drawPlayer();
        if(_needsRefresh || _progress) {
            _main->redraw();
            drawBorders();
        }
    }
    
    void on_mouse(const ::MEVENT& event) override
    {
    }
    
    void on_resize(int w, int h) override
    {
        DEBUG_LOG(2, "Window resized: " << w << "x" << h);
        updateMainWindow(_activeMain, true);
        calculatePlayBar();
        drawPlayer();
        drawBorders();
    }
    
    void on_event(int event) override
    {
        switch(event) {
            case 10: {
                ::flushinp();
                auto listview = std::dynamic_pointer_cast<ghc::cui::list_view>(_main);
                if(listview) {
                    int selected = std::dynamic_pointer_cast<ghc::cui::list_view>(_main)->selected();
                    switch (_activeMain) {
                        case eStationList: {
                            if(selected >= 0 && selected < _stationsModel._stations.size()) {
                                auto& station = _stationsModel._stations[selected];
                                _stationsModel._activeStation = station._id;
                                _streamsModel._streams = station._streams;
                                _tracksModel.select(0);
                                updateMainWindow(eStreamList);
                            }
                            break;
                        }
                        case eStreamList: {
                            if(selected >= 0 && selected < _streamsModel._streams.size()) {
                                auto stream = _streamsModel._streams[selected];
                                _streamsModel._activeStream = stream._id;
                                _rdb.deepFetch(stream);
                                _tracksModel._stream = std::make_shared<Stream>(stream);
                                _tracksModel._tracks = stream._tracks;
                                _tracksModel.select(0);
                                _tracksModel._activeTrack = 0;
                                updateMainWindow(eTrackList);
                                // play
                                _rdb.setPlayed(stream);
                                _player.setSource(stream);
                                _player.play();
                                _chatModel._chat = _rdb.fetchChat(stream);
                                _chatModel.rescan();
                            }
                            break;
                        }
                        case eTrackList: {
                            if(selected >= 0 && selected < _tracksModel._tracks.size()) {
                                auto track = _tracksModel._tracks[selected];
                                _rdb.deepFetch(track);
                                _rdb.deepFetch(*track._stream);
                                // play
                                _rdb.setPlayed(*track._stream);
                                _player.setSource(track);
                                _player.play();
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
                break;
            }
            case ' ': { // PLAY/PAUSE
                PlayerState state = _player.state();
                if((state == ePAUSED || state == eENDOFSTREAM) && _player.hasSource()) {
                    _player.play();
                }
                else {
                    _player.pause();
                }
                break;
            }
            case 'v': // volume down
                _player.volume(_player.volume()-5);
                break;
            case 'V': // volume up
                _player.volume(_player.volume()+5);
                break;
            case KEY_F(1): // switch to station list
                updateMainWindow(eStationList);
                break;
            case KEY_F(2): // switch to stream list
                updateMainWindow(eStreamList);
                break;
            case KEY_F(3): // switch to track list
                updateMainWindow(eTrackList);
                break;
            case KEY_F(4): // switch to chat
                updateMainWindow(eChat);
                break;
            case KEY_F(5): // switch to config page
                updateMainWindow(eRadio);
                break;
            case KEY_F(6): // switch to config page
                updateMainWindow(eConfig);
                break;
            case KEY_F(7): // switch to info page
                updateMainWindow(eInfo);
                break;
            case KEY_F(10):
                quit();
                break;
        }
        calculatePlayBar();
        drawPlayer();
        drawBorders();
    }
    
    void drawPlayer()
    {
        static const std::string states[] = { "Paused:", "Playing:", "End of:", "Ending: ", "Error: " };
        if(validTerminal()) {
            print(1, height() - 5, std::string(width() - 2, ' '));
            print(1, height() - 4, std::string(width() - 2, ' '));
            auto stream = _player.currentStream();
            auto state = states[static_cast<int>(_player.state())];
            print(1, height() - 5, _player.hasSource() ? state : "");
            int playPos = -1;
            if(stream) {
                auto space = width() - 4 - state.size();
                if(space > 0) {
                    auto title = "[" + formattedDate(stream->_timestamp) + "] " + stream->_host + ": " + stream->_name;
                    title = title.substr(0, width() - 4 - state.size());
                    print(2 + state.size(), height() - 5, title);
                }
                auto t = " " + formattedDuration(_player.playTime()) + "/" + formattedDuration(stream->_duration);
                print(width() - 1 - t.size(), height() - 5, t);
                auto dt = (double)stream->_duration / (width() - 2);
                auto pt = _player.playTime();
                playPos = int(pt / dt + 0.5);
                if(playPos >= width() - 2) playPos = width() - 2;
                auto trackIndex = stream->trackIndexForTime(pt);
                if(trackIndex < stream->_tracks.size()) {
                    const auto& track = stream->_tracks[trackIndex];
                    print(1, height() - 4, track._artist + ": " + track._name);
                }
            }
            if(_progress > 0) {
                auto progstr = std::string("Updating metadata... (") + std::to_string(_progress) + "% done)";
                if(progstr.length() < width() - 2) {
                    progstr += std::string(width() - 2 - progstr.length(), ' ');
                }
                else {
                    progstr = progstr.substr(0, width() - 2);
                }
                auto pw = _progress * (width()-2) / 100;
                print(1, height() - 3, progstr.substr(0, pw), A_REVERSE);
                print(pw + 1, height() - 3, progstr.substr(pw));
            }
            else {
                print(1, height() - 3, _playBar);
            }
            if(playPos >= 0) {
                print(1+playPos, height() - 3, "\u2588");
            }
        }
    }
    
    void drawMenu()
    {
        if(validTerminal()) {
            int pos = 0;
            int size = 1; // for additional digit in F10
            for(const auto& m : g_fmenu) {
                size += 4 + m.second.size();
            }
            std::string space;
            if(size <= width()) {
                space = " ";
            }
            for(const auto& m : g_fmenu) {
                if(m.first != 10) {
                    print(pos, height() - 1, "F" + std::to_string(m.first));
                    print(pos+2, height() - 1, space + m.second + space, A_REVERSE);
                    pos += 2 + m.second.size() + space.size()*2;
                }
                else {
                    int right = width() - 3 - m.second.size() - space.size()*2;
                    print(right, height() - 1, "F" + std::to_string(m.first));
                    print(right+3, height() - 1, space + m.second + space, A_REVERSE);
                }
            }
        }
    }
    
    void drawBorders()
    {
        if(validTerminal()) {
            drawBox(0, 0, width(), height()-1);
            drawHLine(0, height() - 6, width(), ACS_LTEE, ACS_RTEE);
            std::string appName = relive::appName() + "-v" + RELIVE_VERSION_STRING_SHORT;
            print(width() - appName.length() - 1, 0, appName);
            if(!_title.empty()) {
                print((width() - _title.length())/2, 0, " "+_title+" ", A_REVERSE);
            }
            drawMenu();
        }
        else {
            print(0, 0, "Oops: at least 40x10 terminal size needed!");
        }
    }

    void fetchStations()
    {
        _stationsModel._stations = _rdb.fetchStations();
        for(auto& station : _stationsModel._stations) {
            _rdb.deepFetch(station);
        }
    }
    
    void updateMainWindow(ActiveMainView view, bool force = false)
    {
        if(validTerminal()) {
            if(force || _activeMain != view) {
                _activeMain = view;
                switch(_activeMain) {
                    case eStationList:
                        _main.reset();
                        _main = create_window<ghc::cui::list_view>(1, 1, width()-2, height()-7, _stationsModel);
                        _title = "Stations";
                        break;
                    case eStreamList:
                        _main.reset();
                        _main = create_window<ghc::cui::list_view>(1, 1, width()-2, height()-7, _streamsModel);
                        _title = "Streams";
                        break;
                    case eTrackList:
                        _main.reset();
                        _main = create_window<ghc::cui::list_view>(1, 1, width()-2, height()-7, _tracksModel);
                        _title = "Tracks";
                        break;
                    case eChat:
                        _main.reset();
                        _main = create_window<ghc::cui::log_view>(1, 1, width()-2, height()-7, _chatModel);
                        _title = "Chat";
                        break;
                    case eRadio: {
                        static const std::string notImplemented = "\n\nSorry, radio functionality is not implemented yet.";
                        _main.reset();
                        _main = create_window<ghc::cui::text_view>(1, 1, width()-2, height()-7, notImplemented, true);
                        _title = "Radio";
                        break;
                    }
                    case eConfig: {
                        static std::string configText;
                        _main.reset();
                        std::ostringstream os;
                        _parser.usage(os);
                        configText = "Sorry, no config ui yet, please use command line:\n\n" + os.str();
                        _main = create_window<ghc::cui::text_view>(1, 1, width()-2, height()-7, configText, true);
                        _title = "Config";
                        break;
                    }
                    case eInfo:
                        _main.reset();
                        if(g_info.find("@VERSION@") != std::string::npos) {
                            auto pos = g_info.find("@VERSION@");
                            g_info.replace(pos, 9, std::string(RELIVE_VERSION_STRING_LONG));
                            pos = g_info.find("@VERLINE@");
                            g_info.replace(pos, 9, std::string(std::strlen(RELIVE_VERSION_STRING_LONG), '-'));
                        }
                        _main = create_window<ghc::cui::text_view>(1, 1, width()-2, height()-7, g_info);
                        _title = "Info";
                        break;
                    default:
                        break;
                }
            }
            drawBorders();
            _main->redraw();
        }
        else {
            if(force || _activeMain != view) {
                static const std::string dummy;
                _activeMain = view;
                switch(_activeMain) {
                    case eStationList:
                        _main.reset();
                        _main = create_window<ghc::cui::list_view>(1, 1, width(), height(), _stationsModel);
                        _title = "Stations";
                        break;
                    case eStreamList:
                        _main.reset();
                        _main = create_window<ghc::cui::list_view>(1, 1, width(), height(), _streamsModel);
                        _title = "Streams";
                        break;
                    case eTrackList:
                        _main.reset();
                        _main = create_window<ghc::cui::list_view>(1, 1, width(), height(), _tracksModel);
                        _title = "Tracks";
                        break;
                    case eChat:
                        _main.reset();
                        _main = create_window<ghc::cui::log_view>(1, 1, width(), height(), _chatModel);
                        _title = "Chat";
                        break;
                    case eRadio:
                        _main.reset();
                        _main = create_window<ghc::cui::text_view>(1, 1, width(), height(), dummy, true);
                        _title = "Radio";
                        break;
                    case eConfig:
                        _main.reset();
                        _main = create_window<ghc::cui::text_view>(1, 1, width(), height(), dummy, true);
                        _title = "Config";
                        break;
                    case eInfo:
                        _main.reset();
                        _main = create_window<ghc::cui::text_view>(1, 1, width(), height(), g_info);
                        _title = "Info";
                        break;
                    default:
                        break;
                }
            }
        }
    }
    
    
    void calculatePlayBar()
    {
        auto stream = _player.currentStream();
        _playBar.clear();
        _playBar.reserve(width() * 3);
        if(stream) {
            auto dt = (double)stream->_duration / (width() - 2);
            double sum = 0, total = 0;
            int index = 0;
            int w = width() - 2;
            for(const auto& track : stream->_tracks) {
                sum += track._duration / dt;
                if(sum > 0.5) {
                    total += sum;
                    while(w && sum > 0.5) {
                        sum = sum - 1;
                        --w;
                        _playBar += (index&1) ? "\u2592" : "\u2591";
                    }
                }
                ++index;
            }
            /*
            for(int i = 0; i < width() - 2; ++i) {
                auto timePos = dt * i;
                int index = stream->trackIndexForTime(timePos);
                _playBar += (index&1) ? "\u2592" : "\u2591";
            }*/
        }
        else {
            for(int i = 0; i < width() - 2; ++i) {
                _playBar.append("\u2591");
            }
        }
    }
    
private:
    void progress(int percent)
    {
        std::lock_guard<std::mutex> lock{_mutex};
        _progress = percent;
        _needsRefresh = true;
    }
    std::mutex _mutex;
    ReLiveDB _rdb;
    int64_t _lastFetch;
    Player _player;
    ghc::cui::window_ptr _main;
    StationsModel _stationsModel;
    StreamsModel _streamsModel;
    TracksModel _tracksModel;
    ChatModel _chatModel;
    std::string _title;
    ActiveMainView _activeMain;
    std::atomic_bool _needsRefresh;
    std::string _playBar;
    int _progress;
    ghc::options& _parser;
};

extern char **environ;

int main(int argc, char* argv[])
{
    try {
        relive::setAppName(RELIVE_APP_NAME);
        relive::LogManager::setOutputFile(relive::dataPath() + "/" + appName() + ".log");
        relive::LogManager::instance()->defaultLevel(3);
        if(relive::isInstanceRunning()) {
            throw std::runtime_error("Instance already running.");
        }

        ghc::options parser(argc, argv);
        parser.onOpt({"-?", "-h", "--help"}, "Output this help text", [&](const std::string&){
            parser.usage(std::cout);
            exit(0);
        });
        parser.onOpt({"-v", "--version"}, "Show program version and exit.", [&](const std::string&){
            std::cout << "reLiveCUI " << RELIVE_VERSION_STRING_LONG << std::endl;
            exit(0);
        });
        parser.onOpt({"-l", "--list-devices"}, "Dump a list of found and supported output devices and exit.", [&](const std::string&){
#ifdef RELIVE_RTAUDIO_BACKEND
            RtAudio audio;
            audio.showWarnings(false);
            unsigned int devices = audio.getDeviceCount();
            RtAudio::DeviceInfo info;
            for(unsigned int i=0; i<devices; ++i) {
                info = audio.getDeviceInfo( i );
                if(info.probed == true && info.outputChannels >= 2) {
                    std::cout << "'" << info.name << "', ";
                    std::cout << " maximum output channels = " << info.outputChannels << ", [";
                    std::string rates;
                    for(auto rate : info.sampleRates) {
                        if(!rates.empty()) {
                            rates += ", ";
                        }
                        rates += std::to_string(rate) + "Hz";
                    }
                    std::cout << rates << "]" << std::endl;
                }
            }
#elif defined(RELIVE_MINIAUDIO_BACKEND)
          ma_context context;
          if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
              // Error.
          }

          ma_device_info* pPlaybackInfos;
          ma_uint32 playbackCount;
          ma_device_info* pCaptureInfos;
          ma_uint32 captureCount;
          if (ma_context_get_devices(&context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS) {
              // Error.
          }

          // Loop over each device info and do something with it. Here we just print the name with their index. You may want
          // to give the user the opportunity to choose which device they'd prefer.
          for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1) {
              printf("%d - %s[%s] %s\n", iDevice, (pPlaybackInfos[iDevice].isDefault ? "*":" "), pPlaybackInfos[iDevice].id.coreaudio, pPlaybackInfos[iDevice].name);
          }
#endif
            exit(0);
        });
        parser.onOpt({"-s?", "--default-station?"}, "[<name>]\tSet the default station to switch to on startup, only significant part of the name is needed. Without a parameter, this resets to starting on station screen.", [&](const std::string& str){
            ReLiveDB db;
            if(str.empty()) {
                db.setConfigValue(Keys::default_station, "");
                std::cout << "Selected starting with stations screen." << std::endl;
            }
            else {
                auto stations = db.findStations("%"+str+"%");
                if(stations.empty()) {
                    std::cerr << "Sorry, no station matches the given name: '" << str << "'" << std::endl;
                }
                else if (stations.size() > 1) {
                    std::cerr << "Sorry, more than one station matches the given name: '" << str << "'" << std::endl;
                    for(const auto station : stations) {
                        std::cerr << "    '" << station._name << "'" << std::endl;
                    }
                }
                else {
                    db.setConfigValue(Keys::default_station, stations.front()._name);
                    std::cout << "Set '" << stations.front()._name << "' as the new default station." << std::endl;
                }
            }
            exit(0);
        });
#ifndef NDEBUG
        parser.onOpt({"-u!", "--unicode!"}, "<unicode string>\tCalculate display width of a unicode string.", [&](std::string str){
            ::setlocale(LC_ALL, "");
            std::cout << "Unicode string `" << str << "` is " << ghc::cui::detail::utf8Length(str) << " cols wide." << std::endl;
            exit(0);
        });
#endif
        parser.parse();

        
        //for(char **env = environ; *env; ++env) {
        //    DEBUG_LOG("main", 4, "Env: " << *env);
        //}
        ReLiveCUI app(argc, argv, parser);
        app.run();
    }
    catch(std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        exit(1);
    }
    std::cout << "Bye bye." << std::endl;
#if 0
    //using namespace std::chrono_literals;
    Player player;
    //player.setSource(Player::eSCastStream, ghc::net::uri("http://oscar.scenesat.com:8000/scenesat"));
    player.setSource(Player::eMediaStream, ghc::net::uri("http://files.slayradio.org/download.php/Boz_-_BBB_20190701.mp3"), 186718208);
    //player.setSource(Player::eReLiveStream, ghc::net::uri("http://hosted.relive.nu/scenesat/getstreamdata/?v=11&streamid=1"), 331354743);
    player.play();
    std::this_thread::sleep_for(60s);
    player.pause();
#endif
    return 0;
}
