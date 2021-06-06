//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
#pragma once

#include <backend/player.hpp>
#include <backend/relivedb.hpp>
#include <imguix/application.h>

#include "stylemanager.h"

namespace relive
{

class ReLiveApp : public ImGui::Application
{
    const float FONT_SIZE = 16;
    const float HEADER_FONT_SIZE = 18;
    const float TOP_PANEL_HEIGHT = 64;
    const float BOTTOM_PANEL_HEIGHT = 100;
    const float PANEL_SHADOW = 24;
    const float PLAY_BAR_HEIGHT = 24;
    const float PLAY_CONTROLS_WIDTH = 150;
    const float PLAY_BAR_REL_END = PLAY_CONTROLS_WIDTH + 24;
    enum CurrentPage { pSTATIONS, pSTREAMS, pTRACKS, pCHAT, pSETTINGS };

public:
    ReLiveApp();
    ~ReLiveApp() override = default;

    void doSetup() override;
    void doTeardown() override;

    void fetchStations();
    void fetchStreams(const Station& station);
    void fetchTracks(Stream& stream);

    bool selectStation(const std::string& name);
    void selectStation(const Station& station);
    void selectStream(Stream& stream, bool play = true);
    void selectTrack(Track track);

    void savePosition();
    bool openURL(std::string url, bool play);

    void handleInput(ImGuiIO& io) override;
    void handleResize(float width, float height) override;
    void handleRedraw();

    void renderFrame() override;
    void renderPlayBar(ImVec2 size);
    void renderPlayControl();
    void renderStations(ImVec2 pageSize);
    void renderStreams(ImVec2 pageSize);
    void renderTracks(ImVec2 pageSize);
    void renderChat(ImVec2 pageSize);
    void renderMainWindow();

    void updatePlayRelatedInfo();

private:
    void progress(int percent);
    ImU32 colorForString(const std::string& str);
    static std::string generateMessage(const ChatMessage& msg);
    void scanForMaxNickSize();
    void recalcMessageSize();
    void renderAppMenu();

    float _width = 0;
    float _height = 0;
    bool _show_demo_window = true;
    bool _lateSetup = true;
    std::mutex _mutex;
    ReLiveDB _rdb;
    int64_t _lastFetch = 0;
    int64_t _lastSavepoint = 0;
    int _lastPlayPos = 0;
    int _wheelAction = 0;
    Player _player;
    StyleManager _style;
    ImFont* _propFont = nullptr;
    ImFont* _headerFont = nullptr;
    ImFont* _monoFont = nullptr;
    std::atomic_bool _needsRefresh = true;
    CurrentPage _currentPage = pSTATIONS;
    std::vector<Station> _stations;
    int64_t _activeStation = 0;
    std::vector<Stream> _streams;
    int64_t _activeStream = 0;
    std::vector<Track> _tracks;
    int64_t _activeTrack = 0;
    Track _activeTrackInfo;
    std::vector<ChatMessage> _chat;
    std::vector<ImVec2> _messageSizes;
    float _maxNickSize = 0;
    float _chatLogHeight = 0;
    float _chatMessageWidth = 0;
    int _chatPosition = -1;
    bool _forceScroll = false;
    int _progress = 0;
    std::string _searchText;
    bool _darkMode = true;
    bool _receiveBufferBar = false;
    bool _startAtLastPosition = true;
    int _nameColorSeed = 31337;
    Player::Device _outputDevice;
};


}