//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
/*
 * TODO: Switching stations should reset the stream list position back to 0, same for switching streams and the track list
 */

#include "reliveapp.hpp"

#include <GLFW/glfw3.h>
#include <imguix/imguix.h>
#include <resources/feather_icons.h>
#include <backend/hash.hpp>
#include <backend/logging.hpp>
#include <backend/system.hpp>
#include <backend/utility.hpp>
#include <ghc/options.hpp>
#include <version/version.hpp>

#include <iostream>
#include <regex>
#include <set>

using namespace std::literals;

#define RELIVE_APP_NAME "reLiveG"

namespace relive {

ReLiveApp::ReLiveApp()
    : ImGui::Application(RELIVE_APP_NAME " " RELIVE_VERSION_STRING_LONG " - \u00a9 2020 by Gulrak", "reLiveG")
    , _rdb([&](int percent) { progress(percent); })
    , _lastFetch(0)
    , _activeStation(0)
    , _activeStream(0)
    , _activeTrack(0)
{
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    setWindowSize(static_cast<unsigned>(mode->width * 0.5), static_cast<unsigned>(mode->height * 0.5));
    // LogManager::instance()->defaultLevel(1);
}

void ReLiveApp::progress(int percent)
{
    std::lock_guard<std::mutex> lock{_mutex};
    _progress = percent;
    _needsRefresh = true;
}

ImU32 ReLiveApp::colorForString(const std::string& str)
{
    auto hash = relive::hash(str, static_cast<uint32_t>(_nameColorSeed));
    float r = 0, g = 0, b = 0;
    ImGui::ColorConvertHSVtoRGB((hash & 0xffu) / 255.0f, 0.5f, ((_darkMode ? 255 : 128) - ((hash >> 8u) & 31u)) / 255.0f, r, g, b);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 255));
}

std::string ReLiveApp::generateMessage(const ChatMessage& msg)
{
    switch (msg._type) {
        case ChatMessage::eMe:
        case ChatMessage::eMode:
        case ChatMessage::eKick:
            return msg._strings.back();
        case ChatMessage::eNick:
            return "is now known as " + msg._strings.back();
        case ChatMessage::eJoin:
            return "has joined the channel";
        case ChatMessage::eLeave:
            return "has left the channel " + (msg._strings.size() > 1 ? "(" + msg._strings[1] + ")" : "");
        case ChatMessage::eQuit:
            return "has quit " + (msg._strings.size() > 1 ? "(" + msg._strings[1] + ")" : "");
        case ChatMessage::eTopic:
            return "has changed the topic to: " + msg._strings.back();
        default:
            return msg._strings.back();
    }
}

void ReLiveApp::scanForMaxNickSize()
{
    ZoneScopedN("scanForMaxNickSize");
    std::set<std::string> nicks;
    for (const auto& msg : _chat) {
        auto nick = msg.nick();
        if (!nick.empty() && !nicks.count(nick)) {
            nicks.insert(nick);
            auto l = ImGui::CalcTextSize(msg._strings.front().c_str()).x;
            if (l > _maxNickSize) {
                _maxNickSize = l;
            }
        }
    }
}

void ReLiveApp::recalcMessageSize()
{
    ZoneScopedN("recalcMessageSize");
    _messageSizes.clear();
    _messageSizes.reserve(_chat.size());
    _chatLogHeight = 0;
    _chatMessageWidth = _width - _maxNickSize - 130;
    for (const ChatMessage& msg : _chat) {
        auto text = generateMessage(msg);
        _messageSizes.push_back(ImGui::CalcTextSize(text.c_str(), nullptr, false, _chatMessageWidth));
        _chatLogHeight += _messageSizes.back().y;
    }
}

void ReLiveApp::fetchStreams(const Station& station)
{
    if (!station._streams.empty()) {
        _streams = station._streams;
        _rdb.deepFetch(_streams.front(), true);
        auto parent = _streams.front()._station;
        for (auto& stream : _streams) {
            stream._station = parent;
        }
    }
}

void ReLiveApp::fetchTracks(Stream& stream)
{
    _activeStream = stream._id;
    _rdb.deepFetch(stream);
    _tracks = stream._tracks;
    auto parent = std::make_shared<Stream>(stream);
    for (auto& track : _tracks) {
        track._stream = parent;
    }
    /*if(!_tracks.empty()) {
        _rdb.deepFetch(_tracks.front(), true);
    }*/
    _activeTrack = 0;
}

void ReLiveApp::fetchStations()
{
    _stations = _rdb.fetchStations();
    for (auto& station : _stations) {
        _rdb.deepFetch(station);
    }
}

void ReLiveApp::doSetup()
{
    auto& style = ImGui::GetStyle();
    auto& io = ImGui::GetIO();
    style.WindowPadding = ImVec2(2, 2);
    // ImGuiIO& io = ImGui::GetIO();
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    builder.AddRanges(io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    static ImVector<ImWchar> ranges;
    /*
    static const ImWchar glyph_ranges[] =
        {                 // 0x20, 0xffff, 0x1f600, 0x1f64f, 0 };
         0x20,   0x7f,    // Basic Latin
         0x80,   0xff,    // Latin-1 Supplement
         0x100,  0x17f,   // Latin Extended-A
         0x250,  0x2af,   // IPA Extensions
         0x2b0,  0x2ff,   // Spacing Modifier Letters
         0x370,  0x3ff,   // Greek and Coptic
         0x400,  0x4ff,   // Cyrillic
         0x1100, 0x11ff,  // Hangul Jamo
         0x1d00, 0x1d7f,  // Phonetic Extensions
         0x2000, 0x206f,  // General Punctuation
         0x20a0, 0x20cf,  // Currency Symbols
         0x2100, 0x214f,  // Letterlike Symbols
         0x2200, 0x22ff,  // Mathematical Operators
         0x30a0, 0x30ff,  // Katakana
         0x4e00, 0x9fff,  // CJK Unified Ideographs
         0xac00, 0xd7af,  // Hangul Syllables
         0};*/

    static const ImWchar dejavue_glyphes[] = {
        // start
        0x20,    0x7e,    0xa0,    0x2e9,   0x2ec,   0x2ee,   0x2f3,   0x2f3,   0x2f7,   0x2f7,   0x300,   0x34f,   0x351,   0x353,   0x357,   0x358,   0x35a,   0x35a,   0x35c,   0x362,   0x370,   0x377,   0x37a,   0x37f,   0x384,   0x38a,   0x38c,
        0x38c,   0x38e,   0x3a1,   0x3a3,   0x525,   0x531,   0x556,   0x559,   0x55f,   0x561,   0x587,   0x589,   0x58a,   0x5b0,   0x5c3,   0x5c6,   0x5c7,   0x5d0,   0x5ea,   0x5f0,   0x5f4,   0x606,   0x607,   0x609,   0x60a,   0x60c,   0x60c,
        0x615,   0x615,   0x61b,   0x61b,   0x61f,   0x61f,   0x621,   0x63a,   0x640,   0x655,   0x657,   0x657,   0x65a,   0x65a,   0x660,   0x670,   0x674,   0x674,   0x679,   0x6bf,   0x6c6,   0x6c8,   0x6cb,   0x6cc,   0x6ce,   0x6ce,   0x6d0,
        0x6d0,   0x6d5,   0x6d5,   0x6f0,   0x6f9,   0x7c0,   0x7e7,   0x7eb,   0x7f5,   0x7f8,   0x7fa,   0xe3f,   0xe3f,   0xe81,   0xe82,   0xe84,   0xe84,   0xe87,   0xe88,   0xe8a,   0xe8a,   0xe8d,   0xe8d,   0xe94,   0xe97,   0xe99,   0xe9f,
        0xea1,   0xea3,   0xea5,   0xea5,   0xea7,   0xea7,   0xeaa,   0xeab,   0xead,   0xeb9,   0xebb,   0xebd,   0xec0,   0xec4,   0xec6,   0xec6,   0xec8,   0xecd,   0xed0,   0xed9,   0xedc,   0xedd,   0x10a0,  0x10c5,  0x10d0,  0x10fc,  0x1401,
        0x1407,  0x1409,  0x141b,  0x141d,  0x1435,  0x1437,  0x144a,  0x144c,  0x1452,  0x1454,  0x14bd,  0x14c0,  0x14ea,  0x14ec,  0x1507,  0x1510,  0x153e,  0x1540,  0x1550,  0x1552,  0x156a,  0x1574,  0x1585,  0x158a,  0x1596,  0x15a0,  0x15af,
        0x15de,  0x15de,  0x15e1,  0x15e1,  0x1646,  0x1647,  0x166e,  0x1676,  0x1680,  0x169c,  0x1d00,  0x1d14,  0x1d16,  0x1d23,  0x1d26,  0x1d2e,  0x1d30,  0x1d5b,  0x1d5d,  0x1d6a,  0x1d77,  0x1d78,  0x1d7b,  0x1d7b,  0x1d7d,  0x1d7d,  0x1d85,
        0x1d85,  0x1d9b,  0x1dbf,  0x1dc4,  0x1dc9,  0x1e00,  0x1efb,  0x1f00,  0x1f15,  0x1f18,  0x1f1d,  0x1f20,  0x1f45,  0x1f48,  0x1f4d,  0x1f50,  0x1f57,  0x1f59,  0x1f59,  0x1f5b,  0x1f5b,  0x1f5d,  0x1f5d,  0x1f5f,  0x1f7d,  0x1f80,  0x1fb4,
        0x1fb6,  0x1fc4,  0x1fc6,  0x1fd3,  0x1fd6,  0x1fdb,  0x1fdd,  0x1fef,  0x1ff2,  0x1ff4,  0x1ff6,  0x1ffe,  0x2000,  0x2064,  0x206a,  0x2071,  0x2074,  0x208e,  0x2090,  0x209c,  0x20a0,  0x20b5,  0x20b8,  0x20ba,  0x20bd,  0x20bd,  0x20d0,
        0x20d1,  0x20d6,  0x20d7,  0x20db,  0x20dc,  0x20e1,  0x20e1,  0x2100,  0x2109,  0x210b,  0x2149,  0x214b,  0x214b,  0x214e,  0x214e,  0x2150,  0x2185,  0x2189,  0x2189,  0x2190,  0x2311,  0x2318,  0x2319,  0x231c,  0x2321,  0x2324,  0x2328,
        0x232b,  0x232c,  0x2373,  0x2375,  0x237a,  0x237a,  0x237d,  0x237d,  0x2387,  0x2387,  0x2394,  0x2394,  0x239b,  0x23ae,  0x23ce,  0x23cf,  0x23e3,  0x23e3,  0x23e5,  0x23e5,  0x23e8,  0x23e8,  0x2422,  0x2423,  0x2460,  0x2469,  0x2500,
        0x269c,  0x269e,  0x26b8,  0x26c0,  0x26c3,  0x26e2,  0x26e2,  0x2701,  0x2704,  0x2706,  0x2709,  0x270c,  0x2727,  0x2729,  0x274b,  0x274d,  0x274d,  0x274f,  0x2752,  0x2756,  0x2756,  0x2758,  0x275e,  0x2761,  0x2794,  0x2798,  0x27af,
        0x27b1,  0x27be,  0x27c5,  0x27c6,  0x27e0,  0x27e0,  0x27e6,  0x27eb,  0x27f0,  0x28ff,  0x2906,  0x2907,  0x290a,  0x290b,  0x2940,  0x2941,  0x2983,  0x2984,  0x29ce,  0x29d5,  0x29eb,  0x29eb,  0x29fa,  0x29fb,  0x2a00,  0x2a02,  0x2a0c,
        0x2a1c,  0x2a2f,  0x2a2f,  0x2a6a,  0x2a6b,  0x2a7d,  0x2aa0,  0x2aae,  0x2aba,  0x2af9,  0x2afa,  0x2b00,  0x2b1a,  0x2b1f,  0x2b24,  0x2b53,  0x2b54,  0x2c60,  0x2c77,  0x2c79,  0x2c7f,  0x2d00,  0x2d25,  0x2d30,  0x2d65,  0x2d6f,  0x2d6f,
        0x2e18,  0x2e18,  0x2e1f,  0x2e1f,  0x2e22,  0x2e25,  0x2e2e,  0x2e2e,  0x4dc0,  0x4dff,  0xa4d0,  0xa4ff,  0xa644,  0xa647,  0xa64c,  0xa64d,  0xa650,  0xa651,  0xa654,  0xa657,  0xa662,  0xa66e,  0xa68a,  0xa68d,  0xa694,  0xa695,  0xa698,
        0xa699,  0xa708,  0xa716,  0xa71b,  0xa71f,  0xa722,  0xa72b,  0xa730,  0xa741,  0xa746,  0xa74b,  0xa74e,  0xa753,  0xa756,  0xa757,  0xa764,  0xa767,  0xa780,  0xa783,  0xa789,  0xa78e,  0xa790,  0xa791,  0xa7a0,  0xa7aa,  0xa7f8,  0xa7ff,
        0xef00,  0xef19,  0xf000,  0xf003,  0xf400,  0xf426,  0xf428,  0xf441,  0xf6c5,  0xf6c5,  0xfb00,  0xfb06,  0xfb13,  0xfb17,  0xfb1d,  0xfb36,  0xfb38,  0xfb3c,  0xfb3e,  0xfb3e,  0xfb40,  0xfb41,  0xfb43,  0xfb44,  0xfb46,  0xfb4f,  0xfb52,
        0xfba3,  0xfbaa,  0xfbad,  0xfbd3,  0xfbdc,  0xfbde,  0xfbdf,  0xfbe4,  0xfbe9,  0xfbfc,  0xfbff,  0xfe00,  0xfe0f,  0xfe20,  0xfe23,  0xfe70,  0xfe74,  0xfe76,  0xfefc,  0xfeff,  0xfeff,  0xfff9,  0xfffd,  0x10300, 0x1031e, 0x10320, 0x10323,
        0x1d300, 0x1d356, 0x1d538, 0x1d539, 0x1d53b, 0x1d53e, 0x1d540, 0x1d544, 0x1d546, 0x1d546, 0x1d54a, 0x1d550, 0x1d552, 0x1d56b, 0x1d5a0, 0x1d5d3, 0x1d7d8, 0x1d7eb, 0x1ee00, 0x1ee03, 0x1ee05, 0x1ee1f, 0x1ee21, 0x1ee22, 0x1ee24, 0x1ee24, 0x1ee27,
        0x1ee27, 0x1ee29, 0x1ee32, 0x1ee34, 0x1ee37, 0x1ee39, 0x1ee39, 0x1ee3b, 0x1ee3b, 0x1ee61, 0x1ee62, 0x1ee64, 0x1ee64, 0x1ee67, 0x1ee6a, 0x1ee6c, 0x1ee72, 0x1ee74, 0x1ee77, 0x1ee79, 0x1ee7c, 0x1ee7e, 0x1ee7e, 0x1f030, 0x1f093, 0x1f0a0, 0x1f0ae,
        0x1f0b1, 0x1f0be, 0x1f0c1, 0x1f0cf, 0x1f0d1, 0x1f0df, 0x1f311, 0x1f318, 0x1f42d, 0x1f42e, 0x1f431, 0x1f431, 0x1f435, 0x1f435, 0x1f600, 0x1f623, 0x1f625, 0x1f62b, 0x1f62d, 0x1f640, 0x1f643, 0x1f643, 0, 0};
    builder.BuildRanges(&ranges);
    ImFontConfig font_cfg;
    font_cfg.OversampleH = font_cfg.OversampleV = 1;
    font_cfg.PixelSnapH = true;
    //_propFont = io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/Supplemental/Arial Unicode.ttf", FONT_SIZE, nullptr, glyph_ranges);
    //_propFont = addFontFromResourceTTF("DejaVuSans.ttf", FONT_SIZE, nullptr, glyph_ranges);
    _propFont = addFontFromResourceTTF("DejaVuSans.ttf", FONT_SIZE, nullptr, dejavue_glyphes);
    //_propFont = addFontFromResourceTTF("unifont.ttf", FONT_SIZE, &font_cfg, ranges.Data);
    static const ImWchar icons_ranges[] = {ICON_MIN_FTH, ICON_MAX_FTH, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    addFontFromResourceTTF("Feather.ttf", FONT_SIZE, &icons_config, icons_ranges);
    _headerFont = addFontFromResourceTTF("DejaVuSans.ttf", HEADER_FONT_SIZE);
    _monoFont = io.Fonts->AddFontDefault();
    _outputDevice.name = _rdb.getConfigValue(Keys::output_device, Player::getDynamicDefaultOutputName());
    _darkMode = _rdb.getConfigValue(Keys::use_dark_theme, _darkMode);
    _style.setTheme(_darkMode ? StyleManager::DarkTheme : StyleManager::LightTheme);
    _receiveBufferBar = _rdb.getConfigValue(Keys::show_buffer_bar, _receiveBufferBar);
    _startAtLastPosition = _rdb.getConfigValue(Keys::start_at_last_position, _startAtLastPosition);
    _nameColorSeed = _rdb.getConfigValue(Keys::name_color_seed, _nameColorSeed);
    _player.volume(_rdb.getConfigValue(Keys::player_volume, _player.volume()));
    fetchStations();
}

void ReLiveApp::doTeardown()
{
    savePosition();
}

bool ReLiveApp::selectStation(const std::string& name)
{
    DEBUG_LOG(1, "Switching to default station '" << name << "'");
    for (const auto& station : _stations) {
        DEBUG_LOG(2, "comparing to '" << station._name << "'");
        if (station._name == name) {
            DEBUG_LOG(2, "found '" << station._name << "'");
            selectStation(station);
            return true;
        }
    }
    return false;
}

void ReLiveApp::selectStation(const Station& station)
{
    _activeStation = station._id;
    _streams = station._streams;
    if (!_streams.empty()) {
        _rdb.deepFetch(_streams.front(), true);
    }
    //_tracksModel.select(0);
    _currentPage = CurrentPage::pSTREAMS;
}

void ReLiveApp::selectStream(Stream& stream, bool play)
{
    fetchTracks(stream);
    _rdb.setPlayed(stream);
    _player.setSource(stream);
    if (play) {
        _player.play();
    }
    _chat = _rdb.fetchChat(stream);
    scanForMaxNickSize();
    recalcMessageSize();
    _currentPage = CurrentPage::pTRACKS;
    _needsRefresh = true;
}

void ReLiveApp::selectTrack(Track track)
{
    _rdb.deepFetch(track);
    if (track._stream) {
        selectStream(*track._stream, false);
        _player.setSource(track);
        _player.play();
        _currentPage = CurrentPage::pTRACKS;
    }
}

void ReLiveApp::savePosition()
{
    auto dbPosition = _rdb.getConfigValue(Keys::play_position, std::string());
    std::string newPos;
    if (_player.state() != PlayerState::eENDOFSTREAM && _activeTrack) {
        newPos = _activeTrackInfo.reLiveURL(_player.playTime());
    }
    else if (_activeStation) {
        for (const auto& station : _stations) {
            if (_activeStation == station._id) {
                newPos = station.reLiveURL();
            }
        }
    }
    if(!newPos.empty() && dbPosition != newPos) {
        _rdb.setConfigValue(Keys::play_position, newPos);
    }
    _lastSavepoint = currentTime();
}

bool ReLiveApp::openURL(std::string url, bool play)
{
    auto [stationId, streamId, timeOffset] = parseUrl(url);
    if (stationId >= 0) {
        _needsRefresh = true;
        for (const auto& station : _stations) {
            DEBUG_LOG(2, "comparing to '" << station._name << "'");
            if (station._reliveId == stationId) {
                DEBUG_LOG(2, "found '" << station._name << "'");
                selectStation(station);
                if (streamId >= 0) {
                    for (auto& stream : _streams) {
                        if (stream._reliveId == streamId) {
                            selectStream(stream, false);
                            if (timeOffset >= 0) {
                                _player.seekTo(timeOffset, play);
                            }
                        }
                    }
                }
                return true;
            }
        }
    }
    return false;
}

void ReLiveApp::handleInput(ImGuiIO& io)
{
    if (io.MouseWheel) {
        _wheelAction = 500;
    }
    setClearColor(ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_WindowBg]));
    if (_lateSetup) {
        _lateSetup = false;
        _player.setSource(Player::eSCastStream, ghc::net::uri("https://sj-1.scenesat.com/scenesatmax"));
        /*
        auto defaultStation = _rdb.getConfigValue(Keys::default_station, std::string());
        auto savedPosition = _rdb.getConfigValue(Keys::play_position, std::string());
        if (savedPosition.empty() || !_startAtLastPosition || !openURL(savedPosition, false)) {
            for (const auto& station : _stations) {
                if (station._name == defaultStation) {
                    DEBUG_LOG(2, "found default station '" << station._name << "'");
                    selectStation(station);
                }
            }
        }
         */
    }
    if (currentTime() - _lastFetch > 3600) {
        DEBUG_LOG(1, "Fetching station info...");
        _rdb.refreshStations([this]() { handleRedraw(); });
        fetchStations();
        _lastFetch = currentTime();
    }
    if (currentTime() - _lastSavepoint >= 60) {
        savePosition();
    }
}

void ReLiveApp::handleResize(float width, float height)
{
    _width = width;
    _height = height;
    recalcMessageSize();
}

void ReLiveApp::renderFrame()
{
    ZoneScopedN("renderFrame");
    renderMainWindow();
#ifndef NDEBUG
    if (_show_demo_window) {
        ImGui::ShowDemoWindow(&_show_demo_window);
    }
#endif
    _needsRefresh = false;
    if (_wheelAction > 0) {
        --_wheelAction;
    }
}

void ReLiveApp::handleRedraw()
{
    onRefresh();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void ReLiveApp::renderPlayBar(ImVec2 size)
{
    ZoneScopedN("renderPlayBar");
    const ImU32 col1 = _style.getColor(reLiveCol_PlaybarStripe0);
    const ImU32 col2 = _style.getColor(reLiveCol_PlaybarStripe1);
    const ImU32 colBorder = _style.getColor(reLiveCol_PlaybarBorder);
    const ImU32 colEmpty = _style.getColor(reLiveCol_PlaybarEmpty);
    const ImU32 colCursor = _style.getColor(reLiveCol_PlaybarCursor);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    auto pos = ImGui::GetCursorPos();
    auto stream = _player.currentStream();
    auto width = size.x;
    if (stream) {
        auto dt = (double)stream->_duration / width;
        double sum = 0, total = 0;
        int index = 0;
        int w = width;
        for (const auto& track : stream->_tracks) {
            auto part = track._duration / dt;
            if(sum + part > width) {
                part = width - sum;
            }
            if (part > 0.1f) {
                // draw from sum to sum+part
                drawList->AddRectFilled(ImVec2(pos.x + sum, pos.y), ImVec2(pos.x + sum + part, pos.y + size.y), index & 1 ? col1 : col2);
            }
            if(part > 0.0f) {
                sum += part;
            }
            ++index;
        }
        int playTimePos = _player.playTime() / dt;
        // drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), colBorder);
        drawList->AddLine(ImVec2(pos.x + playTimePos - 2.5f, pos.y), ImVec2(pos.x + playTimePos + 4.0f, pos.y), colCursor, 2.0f);
        drawList->AddLine(ImVec2(pos.x + playTimePos - 2.5f, pos.y + PLAY_BAR_HEIGHT - 0.5f), ImVec2(pos.x + playTimePos + 4.0f, pos.y + PLAY_BAR_HEIGHT - 0.5f), colCursor, 2.0f);
        drawList->AddLine(ImVec2(pos.x + playTimePos + 0.5f, pos.y + 0.5f), ImVec2(pos.x + playTimePos + 0.5f, pos.y + PLAY_BAR_HEIGHT - 1.0f), colCursor, 2.0f);
    }
    else {
        drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), colEmpty);
    }
    if (_receiveBufferBar) {
        // render buffer gauges
        drawList->AddRectFilled(ImVec2(pos.x, pos.y + PLAY_BAR_HEIGHT + 0.5), ImVec2(pos.x + size.x * _player.receiveBufferQuote(), pos.y + PLAY_BAR_HEIGHT + 4), col1);
        drawList->AddRectFilled(ImVec2(pos.x, pos.y + PLAY_BAR_HEIGHT + 4.5), ImVec2(pos.x + size.x * _player.decodeBufferQuote(), pos.y + PLAY_BAR_HEIGHT + 8), col1);
    }
    ImGui::InvisibleButton("playbar", size);
    if (ImGui::IsItemClicked()) {
        auto clickPos = ImGui::GetMousePos();
        auto relClick = clickPos - ImGui::GetItemRectMin();
        auto playpos = (double)stream->_duration / size.x * relClick.x;
        if (playpos >= 0 && playpos < stream->_duration) {
            _player.seekTo(playpos);
        }
    }
}

void ReLiveApp::renderPlayControl()
{
    ZoneScopedN("renderPlayControl");
    static int volume = _player.volume();
    ImGui::BeginGroup();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::PushItemWidth(PLAY_CONTROLS_WIDTH - 8);
    if (ImGui::SliderInt("##Vol", &volume, 0, 100, "")) {
        _player.volume(volume);
        _rdb.setConfigValue(Keys::player_volume, volume);
    }
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Indent(24);
    ImGui::Text(ICON_FTH_SKIP_BACK);
    if (ImGui::IsItemClicked()) {
        _player.prev();
    }
    ImGui::SameLine(0, 20);
    if (_player.state() == ePLAYING) {
        ImGui::Text(ICON_FTH_PAUSE);
        if (ImGui::IsItemClicked()) {
            _player.pause();
        }
    }
    else {
        ImGui::Text(ICON_FTH_PLAY);
        if (ImGui::IsItemClicked()) {
            _player.play();
        }
    }
    ImGui::SameLine(0, 20);
    ImGui::Text(ICON_FTH_SKIP_FORWARD);
    if (ImGui::IsItemClicked()) {
        _player.next();
    }
    ImGui::EndGroup();
}

void ReLiveApp::renderTopPanelShadow()
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    auto pos = ImGui::GetWindowPos();
    drawList->AddRectFilledMultiColor(ImVec2(pos.x, pos.y), ImVec2(pos.x + _width, pos.y + PANEL_SHADOW), IM_COL32(0, 0, 0, 100), IM_COL32(0, 0, 0, 100), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
}

void ReLiveApp::renderBottomPanelShadow()
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    auto pos = ImGui::GetWindowPos();
    auto height = ImGui::GetWindowHeight();
    drawList->AddRectFilledMultiColor(ImVec2(pos.x, pos.y + height - PANEL_SHADOW), ImVec2(pos.x + _width, pos.y + height), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 100),
                                      IM_COL32(0, 0, 0, 100));
}

void ReLiveApp::renderStations(ImVec2 pageSize)
{
    ZoneScopedN("renderStations");
    setWindowTitle(RELIVE_APP_NAME " " RELIVE_VERSION_STRING_LONG " - \u00a9 2020 by Gulrak");
    ImGui::BeginTable("StationsTable", 3, ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY /*| ImGuiTableFlags_ScrollFreezeTopRow*/, pageSize);
    ImGui::TableSetupColumn("Station Name##station", ImGuiTableColumnFlags_WidthStretch, 0.5f);
    ImGui::TableSetupColumn("Streams##station", ImGuiTableColumnFlags_WidthFixed, 80);
    ImGui::TableSetupColumn("URL##station", ImGuiTableColumnFlags_WidthStretch, 0.5f);
    ImGui::TableSetupScrollFreeze(3, 1);

    ImGui::PushFont(_headerFont);
    ImGui::TableHeadersRow();
    renderTopPanelShadow();
    ImGui::PopFont();
    for (const auto& station : _stations) {
        ImGui::TableNextRow();
        bool isActive = false;
        if (station._id == _activeStation) {
            _style.pushColor(ImGuiCol_Text, reLiveCol_TableActiveLine);
            isActive = true;
        }
        ImGui::TableSetColumnIndex(0);
        if (ImGui::Selectable(station._name.c_str(), station._id == _activeStation, ImGuiSelectableFlags_SpanAllColumns)) {
            selectStation(station);
        }
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%lu", station._streams.size());
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%s", station._webSiteUrl.c_str());
        if (isActive) {
            ImGui::PopStyleColor();
        }
    }
    renderBottomPanelShadow();
    ImGui::EndTable();
}

void ReLiveApp::renderStreams(ImVec2 pageSize)
{
    ZoneScopedN("renderStreams");
    std::string station = _activeStation && !_streams.empty() ? _streams.front()._station->_name : "reLive - <no station selected>";
    setWindowTitle(station);
    ImGui::BeginTable("StreamsTable", 6, ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY /*| ImGuiTableFlags_ScrollFreezeTopRow*/, pageSize);
    ImGui::TableSetupColumn(" ##Streams", ImGuiTableColumnFlags_WidthFixed, 20);
    ImGui::TableSetupColumn("    Date##Streams", ImGuiTableColumnFlags_WidthFixed, 90);
    ImGui::TableSetupColumn("Hosts##Streams", ImGuiTableColumnFlags_WidthStretch, 0.35f);
    ImGui::TableSetupColumn("Title##Streams", ImGuiTableColumnFlags_WidthStretch, 0.40f);
    ImGui::TableSetupColumn("Duration##Streams", ImGuiTableColumnFlags_WidthFixed, 80);
    ImGui::TableSetupColumn("Chat##Streams", ImGuiTableColumnFlags_WidthFixed, 60);
    ImGui::TableSetupScrollFreeze(6, 1);
    ImGui::PushFont(_headerFont);
    ImGui::TableHeadersRow();
    renderTopPanelShadow();
    ImGui::PopFont();
    static const char* playAnim[4] = {ICON_FTH_VOLUME "##strm", ICON_FTH_VOLUME_1 "##strm", ICON_FTH_VOLUME_2 "##strm", "##strm"};
    if(!_streams.empty() && _streams.front()._station && !_streams.front()._station->_liveStream.empty()) {
        ImGui::TableNextRow();
        _style.pushColor(ImGuiCol_Text, reLiveCol_TableUnplayed);
        ImGui::TableSetColumnIndex(0);
        if (ImGui::Selectable((std::string(false ? playAnim[std::time(nullptr) % 3] : playAnim[3]) + std::to_string(-42)).c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
            //selectStream(stream);
            //savePosition();
        }
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("- - NOW - -");
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("");
        ImGui::TableSetColumnIndex(3);
        ImGui::Text("LIVE STREAM");
        ImGui::TableSetColumnIndex(4);
        ImGui::Text("");
        ImGui::TableSetColumnIndex(5);
        ImGui::Text("   -");
        ImGui::PopStyleColor();
    }
    for (auto& stream : _streams) {
        ImGui::TableNextRow();
        bool isActive = false;
        static int64_t lastActiveStream = 0;
        if (stream._id == _activeStream) {
            if (lastActiveStream != _activeStream /*!_wheelAction*/) {
                ImGui::SetScrollHereY();
                lastActiveStream = _activeStream;
            }
            _style.pushColor(ImGuiCol_Text, reLiveCol_TableActiveLine);
        }
        else if (!(stream._flags & Stream::ePlayed)) {
            _style.pushColor(ImGuiCol_Text, reLiveCol_TableUnplayed);
        }
        else {
            _style.pushColor(ImGuiCol_Text, reLiveCol_TablePlayed);
        }
        ImGui::TableSetColumnIndex(0);
        if (ImGui::Selectable((std::string(stream._id == _activeStream ? playAnim[std::time(nullptr) % 3] : playAnim[3]) + std::to_string(stream._id)).c_str(), stream._id == _activeStream, ImGuiSelectableFlags_SpanAllColumns)) {
            selectStream(stream);
            savePosition();
        }
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", formattedDate(stream._timestamp).c_str());
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%s", stream._host.c_str());
        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%s", stream._name.c_str());
        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%s", formattedDuration(stream._duration).c_str());
        ImGui::TableSetColumnIndex(5);
        ImGui::Text("   %s", stream._chatChecksum ? ICON_FTH_CHECK : "-");
        ImGui::PopStyleColor();
    }
    renderBottomPanelShadow();
    ImGui::EndTable();
}

void ReLiveApp::renderTracks(ImVec2 pageSize)
{
    ZoneScopedN("renderTracks");
    static const char* types[] = {"-", ICON_FTH_MUSIC, ICON_FTH_MESSAGE_CIRCLE, ICON_FTH_BELL, ICON_FTH_MESSAGE_SQUARE};

    std::string stream = !_tracks.empty() ? _tracks.front()._stream->_station->_name + ": " + _tracks.front()._stream->_name + " [" + formattedDate(_tracks.front()._stream->_timestamp) + "]" : "reLive - <no stream selected>";
    setWindowTitle(stream);
    ImGui::BeginTable("TracksTable", 6, ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY /*| ImGuiTableFlags_ScrollFreezeTopRow*/, pageSize);
    ImGui::TableSetupColumn(" ##tracks", ImGuiTableColumnFlags_WidthFixed, 20);
    ImGui::TableSetupColumn("  Time##tracks", ImGuiTableColumnFlags_WidthFixed, 80);
    ImGui::TableSetupColumn("Artist##tracks", ImGuiTableColumnFlags_WidthStretch, 0.35f);
    ImGui::TableSetupColumn("Title##tracks", ImGuiTableColumnFlags_WidthStretch, 0.40f);
    ImGui::TableSetupColumn("Duration##tracks", ImGuiTableColumnFlags_WidthFixed, 80);
    ImGui::TableSetupColumn("Type##tracks", ImGuiTableColumnFlags_WidthFixed, 60);
    ImGui::TableSetupScrollFreeze(6, 1);
    ImGui::PushFont(_headerFont);
    ImGui::TableHeadersRow();
    renderTopPanelShadow();
    ImGui::PopFont();
    static const char* playAnim[4] = {ICON_FTH_VOLUME "##trk", ICON_FTH_VOLUME_1 "##trk", ICON_FTH_VOLUME_2 "##strm", "##trk"};
    for (auto& track : _tracks) {
        ImGui::TableNextRow();
        bool isActive = false;
        if (track._id == _activeTrack) {
            static int64_t lastActiveTrack = 0;
            if (lastActiveTrack != _activeTrack && !_wheelAction) {
                ImGui::SetScrollHereY();
                lastActiveTrack = _activeTrack;
            }
            _style.pushColor(ImGuiCol_Text, reLiveCol_TableActiveLine);
            isActive = true;
        }
        ImGui::TableSetColumnIndex(0);
        if (ImGui::Selectable((std::string(track._id == _activeTrack ? playAnim[std::time(nullptr) % 3] : playAnim[3]) + std::to_string(track._id)).c_str(), track._id == _activeTrack, ImGuiSelectableFlags_SpanAllColumns)) {
            _player.setSource(track);
            _player.play();
            _currentPage = CurrentPage::pTRACKS;
            savePosition();
        }
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::Selectable("Copy goto.relive.nu Link")) {
                ImGui::SetClipboardText(("https://goto.relive.nu/" + track.reLiveURL()).c_str());
            }
            if (ImGui::Selectable("Copy Link")) {
                ImGui::SetClipboardText(("relive:" + track.reLiveURL()).c_str());
            }
            ImGui::EndPopup();
        }
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", formattedDuration(track._time).c_str());
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%s", track._artist.c_str());
        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%s", track._name.c_str());
        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%s", formattedDuration(track._duration).c_str());
        ImGui::TableSetColumnIndex(5);
        auto type = track._type > 0 && track._type <= 4 ? types[track._type] : "-";
        ImGui::Text("   %s", type);
        if (isActive) {
            ImGui::PopStyleColor();
        }
    }
    renderBottomPanelShadow();
    ImGui::EndTable();
}

void ReLiveApp::renderChat(ImVec2 pageSize)
{
    ZoneScopedN("renderChat");
    std::string stream = !_tracks.empty() ? _tracks.front()._stream->_station->_name + ": " + _tracks.front()._stream->_name + " [" + formattedDate(_tracks.front()._stream->_timestamp) + "]" : "reLive - <no stream selected>";
    setWindowTitle(stream);
    ImGui::SetNextWindowContentSize(ImVec2(pageSize.x - 20, _chatLogHeight + ImGui::GetTextLineHeight()));
    ImGui::BeginChild("##ChatLog", ImVec2(pageSize.x - 4, pageSize.y));
    if (_forceScroll) {
        ImGui::SetScrollY(ImGui::GetScrollMaxY());
        _forceScroll = false;
    }
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    auto pos = ImGui::GetCursorScreenPos();
    auto scrollPos = ImGui::GetScrollY();
    float yoffset = 0;
    for (int i = 0; i <= _chatPosition; ++i) {
        auto msgcol = _style.getColor(ImGuiCol_Text);
        if (yoffset >= scrollPos - 16 && yoffset < scrollPos + pageSize.y) {
            const auto& msg = _chat[i];
            drawList->AddText(ImVec2(pos.x, pos.y + yoffset), msgcol, formattedDuration(msg._time).c_str());
            if (msg.hasNick()) {
                auto nick = msg.nick();
                auto col = colorForString(nick);
                if (msg._type != ChatMessage::eMessage) {
                    msgcol = col;
                }

                drawList->AddText(ImVec2(pos.x + 80 + _maxNickSize - ImGui::CalcTextSize(nick.c_str()).x, pos.y + yoffset), col, nick.c_str());
            }
            auto text = generateMessage(_chat[i]);
            drawList->AddText(nullptr, 0, ImVec2(pos.x + _maxNickSize + 100, pos.y + yoffset), msgcol, text.c_str(), nullptr, _chatMessageWidth);
        }
        yoffset += _messageSizes[i].y;
    }
    renderTopPanelShadow();
    renderBottomPanelShadow();
    ImGui::EndChild();
}

void ReLiveApp::updatePlayRelatedInfo()
{
    auto playTime = _player.playTime();
    if (playTime != _lastPlayPos) {
        ZoneScopedN("updatePlayRelatedInfos");
        _lastPlayPos = playTime;
        auto stream = _player.currentStream();
        if (stream) {
            // find active track
            int64_t lastId = 0;
            const Track* lastTrackInfo = nullptr;
            for (const auto& track : _tracks) {
                if (track._time > playTime) {
                    if (_activeTrack != lastId) {
                        _activeTrack = lastId;
                        _activeTrackInfo = *lastTrackInfo;
                        _needsRefresh = true;
                    }
                    break;
                }
                lastId = track._id;
                lastTrackInfo = &track;
            }
            if (_activeTrack != lastId) {
                _activeTrack = lastId;
                _activeTrackInfo = *lastTrackInfo;
                _needsRefresh = true;
            }
            int chatPos = -1;
            _chatLogHeight = 0;
            for (const auto& msg : _chat) {
                if (msg._time <= playTime) {
                    ++chatPos;
                    _chatLogHeight += _messageSizes[chatPos].y;
                }
                else {
                    break;
                }
            }
            if (_chatPosition != chatPos) {
                _forceScroll = true;
                _chatPosition = chatPos;
            }
        }
    }
}

void ReLiveApp::renderMainWindow()
{
    ZoneScopedN("renderMainWindow");
    updatePlayRelatedInfo();

    auto style = ImGui::GetStyle();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(_width, _height));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
    ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Implemented in reliveapp_menu.cpp
    renderAppMenu();

    //-----------------------------------------------------------------------------------------
    // MAIN TABS
    //-----------------------------------------------------------------------------------------
    ImGui::PushFont(_headerFont);
    static const std::vector<std::string> menuLabels = {"Stations", "Streams", "Tracks", "Chat"};
    float spacing = 32.0f;
    float width = (menuLabels.size() - 1) * spacing;
    for (const auto& label : menuLabels) {
        width += ImGui::CalcTextSize(label.c_str()).x;
    }
    auto offset = (_width - width) / 2.0f;
    ImGui::BeginGroup();
    ImGui::SetCursorPosY(20.0f);
    int idx = 0;
    for (const auto& label : menuLabels) {
        ImGui::SetCursorPosX(offset);
        if (idx == _currentPage) {
            _style.pushColor(ImGuiCol_Text, reLiveCol_MainMenuTextSelected);
        }
        else {
            _style.pushColor(ImGuiCol_Text, reLiveCol_MainMenuText);
        }
        //-----------------------------------------------------------------
        ImGui::Text("%s", label.c_str());
        if (ImGui::IsItemClicked()) {
            _currentPage = static_cast<CurrentPage>(idx);
        }
        ImGui::PopStyleColor();
        offset += ImGui::CalcTextSize(label.c_str()).x + spacing;
        ImGui::SameLine();
        ++idx;
    }
    ImGui::EndGroup();
    ImGui::PopFont();

    //-----------------------------------------------------------------------------------------
    // SEARCH BAR
    //-----------------------------------------------------------------------------------------
    ImGui::SetCursorPos(ImVec2(_width - 200, 20.0f));
    ImGui::SetNextItemWidth(170);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 3));
    if (ImGui::InputText(ICON_FTH_SEARCH "##search", &_searchText, ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (openURL(_searchText, true)) {
            _searchText = "";
        }
    }
    ImGui::PopStyleVar(2);
    {
        //-----------------------------------------------------------------------------------------
        // SEARCH RESULTS
        //-----------------------------------------------------------------------------------------
        auto id = ImGui::GetItemID();
        static bool isOpen = false;
        static std::vector<ReLiveDB::FindTracksInfo> foundTracks;
        static std::vector<Stream> foundStreams;
        static float maxResultWidth = 0;
        bool isDeactivated = ImGui::IsItemDeactivated();
        if (ImGui::IsItemActivated())
            isOpen = true;
        float popupOffset = std::min(maxResultWidth > 170 ? maxResultWidth - 170 : 0, ImGui::GetItemRectMin().x * 0.9f);
        ImVec2 textPos{ImGui::GetItemRectMin().x - popupOffset, ImGui::GetItemRectMax().y};
        if (isOpen) {
            ImGui::OpenPopup("##SearchBar");
        }
        auto textInputState = ImGui::GetInputTextState((ImGuiID)id);
        if (isOpen && textInputState && textInputState->CurLenW >= 3) {
            ImGui::SetNextWindowPos(textPos, ImGuiCond_Always);
            if (ImGui::BeginPopup("##SearchBar", ImGuiWindowFlags_ChildWindow)) {
                ImGui::PushAllowKeyboardFocus(false);

                int numResults = 0;
                static std::string lastSearch;
                std::string inputStr;
                inputStr.resize(textInputState->TextW.size());

                ImTextStrToUtf8(inputStr.data(), (int)inputStr.size(), textInputState->TextW.Data, textInputState->TextW.Data + textInputState->TextW.Size);
                inputStr.resize((size_t)textInputState->CurLenW);

                maxResultWidth = 0;
                if (inputStr != lastSearch) {
                    if (starts_with(inputStr, "t:") || starts_with(inputStr, "m:")) {
                        inputStr.erase(0, 2);
                        foundStreams.clear();
                        foundTracks = _rdb.findTracksInfo("%" + inputStr + "%", ReLiveDB::eTracks);
                    }
                    else if (starts_with(inputStr, "j:")) {
                        inputStr.erase(0, 2);
                        foundStreams.clear();
                        foundTracks = _rdb.findTracksInfo("%" + inputStr + "%", ReLiveDB::eJingle);
                    }
                    else if (starts_with(inputStr, "n:") || starts_with(inputStr, "c:")) {
                        inputStr.erase(0, 2);
                        foundStreams.clear();
                        foundTracks = _rdb.findTracksInfo("%" + inputStr + "%", ReLiveDB::eNarration);
                    }
                    else if (starts_with(inputStr, "s:")) {
                        inputStr.erase(0, 2);
                        foundStreams = _rdb.findStreams("%" + inputStr + "%");
                        foundTracks.clear();
                    }
                    else {
                        foundStreams = _rdb.findStreams("%" + inputStr + "%");
                        foundTracks = _rdb.findTracksInfo("%" + inputStr + "%");
                    }
                }
                if (!foundStreams.empty()) {
                    ImGui::Spacing();
                    ImGui::Text("Matching Streams:");
                    ImGui::Spacing();
                    for (auto&& stream : foundStreams) {
                        std::string info = "        " + formattedDate(stream._timestamp) + " - " + stream._host + " - " + stream._name + "##serachitem" + std::to_string(numResults);
                        auto textWidth = ImGui::CalcTextSize(info.c_str(), nullptr, true).x;
                        if (textWidth > maxResultWidth) {
                            maxResultWidth = textWidth;
                        }
                        if (ImGui::Selectable(info.c_str()) || ImGui::IsItemClicked()) {
                            selectStream(stream);
                            isOpen = false;
                        }
                        ImGui::Spacing();
                        ++numResults;
                        // if (numHints > 16) {
                        //     ImGui::Text("...");
                        //     break;
                        // }
                    }
                    ImGui::Separator();
                }
                if (!foundTracks.empty()) {
                    ImGui::Spacing();
                    ImGui::Text("Matching Tracks:");
                    ImGui::Spacing();
                    for (auto&& track : foundTracks) {
                        std::string info = "        " + formattedDate(track._timestamp) + " - " + track._streamName + ":\n        " + track._artist + " - " + track._trackName + "##serachitem" + std::to_string(numResults);
                        auto textWidth = ImGui::CalcTextSize(info.c_str(), nullptr, true).x;
                        if (textWidth > maxResultWidth) {
                            maxResultWidth = textWidth;
                        }
                        if (ImGui::Selectable(info.c_str()) || ImGui::IsItemClicked()) {
                            auto trackPtr = _rdb.fetchTrack(track._trackId);
                            if (trackPtr) {
                                selectTrack(*trackPtr);
                            }
                            isOpen = false;
                        }
                        ImGui::Spacing();
                        ++numResults;
                        // if (numHints > 16) {
                        //     ImGui::Text("...");
                        //     break;
                        // }
                    }
                }

                ImGui::PopAllowKeyboardFocus();
                ImGui::EndPopup();
            }
            if (isDeactivated) {
                isOpen = false;
            }
        }
    }
    //-----------------------------------------------------------------

    // ImGui::Separator();
    ImGui::SetCursorPos(ImVec2(8, TOP_PANEL_HEIGHT));

    auto pageSize = ImVec2(_width - 10, _height - TOP_PANEL_HEIGHT - BOTTOM_PANEL_HEIGHT);
    switch (_currentPage) {
        case pSTATIONS:
            renderStations(pageSize);
            break;
        case pSTREAMS:
            renderStreams(pageSize);
            break;
        case pTRACKS:
            renderTracks(pageSize);
            break;
        case pCHAT:
            renderChat(pageSize);
            break;
        default:
            break;
    }
    // ImGui::Separator();

    ImGui::SetCursorPos(ImVec2(8, _height - BOTTOM_PANEL_HEIGHT + 14));
    ImGui::BeginGroup();
    auto stream = _player.currentStream();
    if (stream) {
        ImGui::Text("%s - %s", stream->_station->_name.c_str(), stream->_name.c_str());
        auto currentPlayTime = _player.playTime();
        auto timeInfo = formattedDuration(currentPlayTime) + "/" + formattedDuration(stream->_duration);
        ImGui::SameLine(_width - PLAY_BAR_REL_END - ImGui::CalcTextSize(timeInfo.c_str()).x);
        ImGui::Text("%s", timeInfo.c_str());
        if (_activeTrackInfo._artist.empty()) {
            ImGui::Text("%s", _activeTrackInfo._name.c_str());
        }
        else {
            ImGui::Text("%s: %s", _activeTrackInfo._artist.c_str(), _activeTrackInfo._name.c_str());
            timeInfo = formattedDuration(currentPlayTime - _activeTrackInfo._time) + "/" + formattedDuration(_activeTrackInfo._duration);
            ImGui::SameLine(_width - PLAY_BAR_REL_END - ImGui::CalcTextSize(timeInfo.c_str()).x);
            ImGui::Text("%s", timeInfo.c_str());
        }
    }
    else {
        ImGui::Text(" ");
        ImGui::Text(" ");
    }
    // ImGui::SetCursorPos(ImVec2(4, _height - PLAY_BAR_HEIGHT - 4));
    if (_progress) {
        ImGui::ProgressBar(_progress / 100.0f, ImVec2(_width - PLAY_BAR_REL_END, PLAY_BAR_HEIGHT));
    }
    else {
        renderPlayBar(ImVec2(_width - PLAY_BAR_REL_END, PLAY_BAR_HEIGHT));
    }
    ImGui::EndGroup();
    // ImGui::SameLine();

    ImGui::SetCursorPos(ImVec2(_width - PLAY_CONTROLS_WIDTH, _height - BOTTOM_PANEL_HEIGHT + 14));
    renderPlayControl();

    ImGui::End();
    ImGui::PopStyleVar(2);
}

}  // namespace relive

int main(int argc, char* argv[])
{
    using namespace relive;

    try {
        setAppName(RELIVE_APP_NAME);
        LogManager::setOutputFile(relive::dataPath() + "/" + appName() + ".log");
        LogManager::instance()->defaultLevel(3);
        if (isInstanceRunning()) {
            throw std::runtime_error("Instance already running.");
        }
        ghc::options parser(argc, argv);
        parser.onOpt({"-?", "-h", "--help"}, "Output this help text", [&](const std::string&) {
            parser.usage(std::cout);
            exit(0);
        });
        parser.onOpt({"-v", "--version"}, "Show program version and exit.", [&](const std::string&) {
            std::cout << "reLiveCUI " << RELIVE_VERSION_STRING_LONG << std::endl;
            exit(0);
        });
        parser.onOpt({"-l", "--list-devices"}, "Dump a list of found and supported output devices and exit.", [&](const std::string&) {
            Player p;
            auto devices = p.getOutputDevices();
            for (const auto& device : devices) {
                std::cout << device.name << std::endl;
            }
            exit(0);
        });
        parser.onOpt({"-s?", "--default-station?"}, "[<name>]\tSet the default station to switch to on startup, only significant part of the name is needed. Without a parameter, this resets to starting on station screen.", [&](std::string str) {
            ReLiveDB db;
            if (str.empty()) {
                db.setConfigValue(Keys::default_station, "");
                std::cout << "Selected starting with stations screen." << std::endl;
            }
            else {
                auto stations = db.findStations("%" + str + "%");
                if (stations.empty()) {
                    std::cerr << "Sorry, no station matches the given name: '" << str << "'" << std::endl;
                }
                else if (stations.size() > 1) {
                    std::cerr << "Sorry, more than one station matches the given name: '" << str << "'" << std::endl;
                    for (const auto& station : stations) {
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
        parser.parse();

        ReLiveApp app;
        app.run();
    }
    catch (std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        exit(1);
    }
    return 0;
}
