/*
 * TODO: Chat view
 * TODO: Info view
 * TODO: Settings screen
 * TODO: Switching stations should reset the stream list position back to 0, same for switching streams and the track list
 * TODO: Detaching an audio device (CoreAudio) leads to hanging application (workaround: https://github.com/thestk/rtaudio/issues/194)
 * TODO: extended unicode ranges for asian languages needed in screen font.
 * TODO: reLive bar not always seamless
 */
#include <backend/logging.hpp>
#include <backend/relivedb.hpp>
#include <backend/player.hpp>
#include <backend/system.hpp>
#include <version/version.hpp>
#include <ghc/options.hpp>
#include <RtAudio.h>
#include <GLFW/glfw3.h>
#include <imguix/application.h>
#include <imguix/imguix.h>
#include <resources/feather_icons.h>

#include <iostream>
#include <set>

using namespace std::literals;

#define RELIVE_APP_NAME "reLiveG"

namespace relive {


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
    enum CurrentPage { pSTATIONS, pSTREAMS, pTRACKS, pCHAT };

public:
    ReLiveApp()
    : ImGui::Application("reLiveG " RELIVE_VERSION_STRING_LONG " - \u00a9 2020 by Gulrak", "reLiveG")
    , _rdb([&](int percent){ progress(percent); })
    , _lastFetch(0)
    , _activeStation(0)
    , _activeStream(0)
    , _activeTrack(0)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        setWindowSize(static_cast<unsigned>(mode->width * 0.5), static_cast<unsigned>(mode->height * 0.5));
        //LogManager::instance()->defaultLevel(1);
    }

    ~ReLiveApp()
    {

    }

    void fetchStations()
    {
        _stations = _rdb.fetchStations();
        for(auto& station : _stations) {
            _rdb.deepFetch(station);
        }
    }

    void setThemeColors()
    {
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text]                   = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.39f, 0.41f, 0.42f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.08f, 0.37f, 0.82f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.30f, 0.30f, 0.30f, 0.80f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    void doSetup() override
    {
        auto& style = ImGui::GetStyle();
        auto& io = ImGui::GetIO();
        style.WindowPadding = ImVec2(2, 2);
        setThemeColors();
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
        builder.BuildRanges(&ranges);
        ImFontConfig font_cfg;
        font_cfg.OversampleH = font_cfg.OversampleV = 1;
        font_cfg.PixelSnapH = true;
        //_propFont = io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/Supplemental/Arial Unicode.ttf", FONT_SIZE, nullptr, glyph_ranges);
        //_propFont = addFontFromResourceTTF("DejaVuSans.ttf", FONT_SIZE, nullptr, glyph_ranges);
        _propFont = addFontFromResourceTTF("DejaVuSans.ttf", FONT_SIZE);
        //_propFont = addFontFromResourceTTF("unifont.ttf", FONT_SIZE, &font_cfg, ranges.Data);
        static const ImWchar icons_ranges[] = {ICON_MIN_FTH, ICON_MAX_FTH, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        addFontFromResourceTTF("Feather.ttf", FONT_SIZE, &icons_config, icons_ranges);
        _headerFont = addFontFromResourceTTF("DejaVuSans.ttf", HEADER_FONT_SIZE);
        _monoFont = io.Fonts->AddFontDefault();
        fetchStations();
    }

    void doTeardown() override {}

    bool selectStation(const std::string& name)
    {
        DEBUG_LOG(1, "Switching to default station '" << name << "'");
        for(const auto& station : _stations) {
            DEBUG_LOG(2, "comparing to '" << station._name << "'");
            if(station._name == name) {
                DEBUG_LOG(2, "found '" << station._name << "'");
                _activeStation = station._id;
                _streams = station._streams;
                return true;
            }
        }
        return false;
    }

    void handleInput(ImGuiIO& io) override
    {
        setClearColor(ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_WindowBg]));
        if(currentTime() - _lastFetch > 3600) {
            DEBUG_LOG(1, "Fetching station info...");
            _rdb.refreshStations([this](){ handleRedraw(); });
            fetchStations();
            _lastFetch = currentTime();
        }
    }

    void handleResize(float width, float height) override
    {
        _width = width;
        _height = height;
        recalcMessageSize();
    }

    void handleRedraw()
    {
        onRefresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void renderFrame() override
    {
        renderMainWindow();
#ifndef NDEBUG
        if (_show_demo_window) {
            ImGui::ShowDemoWindow(&_show_demo_window);
        }
#endif
    }

    void renderPlayBar(ImVec2 size)
    {
        const ImU32 col1 = IM_COL32(100,100,100,255);
        const ImU32 col2 = IM_COL32(130,130,130,255);
        const ImU32 colBorder = IM_COL32(200,200,200,255);
        const ImU32 colEmpty = IM_COL32(60,60,60,255);
        const ImU32 colCursor = IM_COL32(220,220,220,255);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        auto pos = ImGui::GetCursorPos();
        auto stream = _player.currentStream();
        auto width = size.x;
        if(stream) {
            auto dt = (double)stream->_duration / width;
            double sum = 0, total = 0;
            int index = 0;
            int w = width;
            for(const auto& track : stream->_tracks) {
                auto part = track._duration / dt;
                if(part > 1.0f) {
                    // draw from sum to sum+part
                    drawList->AddRectFilled(ImVec2(pos.x + sum, pos.y), ImVec2(pos.x + sum + part, pos.y + size.y), index & 1 ? col1 : col2);
                }
                sum += part;
                ++index;
            }
            int playTimePos = _player.playTime() / dt;
            //drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), colBorder);
            drawList->AddLine(ImVec2(pos.x + playTimePos - 3.0f, pos.y), ImVec2(pos.x + playTimePos + 4.0f, pos.y), colCursor, 2.0f);
            drawList->AddLine(ImVec2(pos.x + playTimePos - 3.0f, pos.y + PLAY_BAR_HEIGHT - 0.5f), ImVec2(pos.x + playTimePos + 4.0f, pos.y + PLAY_BAR_HEIGHT - 0.5f), colCursor, 2.0f);
            drawList->AddLine(ImVec2(pos.x + playTimePos + 0.5f, pos.y + 0.5f), ImVec2(pos.x + playTimePos + 0.5f, pos.y + PLAY_BAR_HEIGHT - 1.0f), colCursor, 2.0f);
        }
        else {
            drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), colEmpty);
        }
        ImGui::InvisibleButton("playbar", size);
        if(ImGui::IsItemClicked()) {
            auto clickPos = ImGui::GetMousePos();
            auto relClick = clickPos - ImGui::GetItemRectMin();
            auto pos = (double)stream->_duration / size.x * relClick.x;
            if(pos >= 0 && pos < stream->_duration) {
                _player.seekTo(pos);
            }
        }
    }

    void renderPlayControl()
    {
        static int volume = _player.volume();
        ImGui::BeginGroup();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::PushItemWidth(PLAY_CONTROLS_WIDTH - 8);
        if(ImGui::SliderInt("##Vol", &volume, 0, 100, "")) {
            _player.volume(volume);
        }
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Indent(24);
        ImGui::Text(ICON_FTH_SKIP_BACK);
        if(ImGui::IsItemClicked()) {
            _player.prev();
        }
        ImGui::SameLine(0, 20);
        if(_player.state() == ePLAYING) {
            ImGui::Text(ICON_FTH_PAUSE);
            if(ImGui::IsItemClicked()) {
                _player.pause();
            }
        }
        else {
            ImGui::Text(ICON_FTH_PLAY);
            if(ImGui::IsItemClicked()) {
                _player.play();
            }
        }
        ImGui::SameLine(0, 20);
        ImGui::Text(ICON_FTH_SKIP_FORWARD);
        if(ImGui::IsItemClicked()) {
            _player.next();
        }
        ImGui::EndGroup();
    }

    void renderStations(ImVec2 pageSize)
    {
        ImGui::BeginTable("StationsTable", 3,  ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollFreezeTopRow, pageSize);
        ImGui::TableSetupColumn("Station Name##station", ImGuiTableColumnFlags_WidthStretch,0.5f);
        ImGui::TableSetupColumn("Streams##station", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("URL##station", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::PushFont(_headerFont);
        ImGui::TableNextRow(0, 20);
        for(int i = 0; i < 3; ++i) {
            ImGui::TableSetColumnIndex(i);
            const char* column_name = ImGui::TableGetColumnName(i);
            ImGui::PushID(i);
            ImGui::TableHeader(column_name);
            ImGui::PopID();
        }
        ImGui::PopFont();
        for (const auto& station : _stations) {
            ImGui::TableNextRow();
            bool isActive = false;
            if(station._id == _activeStation) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,255,255,255));
                isActive = true;
            }
            ImGui::TableSetColumnIndex(0);
            if(ImGui::Selectable(station._name.c_str(), station._id == _activeStation, ImGuiSelectableFlags_SpanAllColumns)) {
                _activeStation = station._id;
                _streams = station._streams;
                //_tracksModel.select(0);
                _currentPage = CurrentPage::pSTREAMS;
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%lu", station._streams.size());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", station._webSiteUrl.c_str());
            if(isActive) {
                ImGui::PopStyleColor();
            }
        }
        ImGui::EndTable();
    }

    void renderStreams(ImVec2 pageSize)
    {
        ImGui::BeginTable("StreamsTable", 6,  ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollFreezeTopRow, pageSize);
        ImGui::TableSetupColumn(" ##Streams", ImGuiTableColumnFlags_WidthFixed,20);
        ImGui::TableSetupColumn("    Date##Streams", ImGuiTableColumnFlags_WidthFixed, 90);
        ImGui::TableSetupColumn("Hosts##Streams", ImGuiTableColumnFlags_WidthStretch, 0.35f);
        ImGui::TableSetupColumn("Title##Streams", ImGuiTableColumnFlags_WidthStretch, 0.40f);
        ImGui::TableSetupColumn("Duration##Streams", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Chat##Streams", ImGuiTableColumnFlags_WidthFixed,60);
        ImGui::PushFont(_headerFont);
        ImGui::TableNextRow(0, 20);
        for(int i = 0; i < 6; ++i) {
            ImGui::TableSetColumnIndex(i);
            const char* column_name = ImGui::TableGetColumnName(i);
            ImGui::PushID(i);
            ImGui::TableHeader(column_name);
            ImGui::PopID();
        }
        ImGui::PopFont();
        static const char* playAnim[4] = { ICON_FTH_VOLUME "##strm", ICON_FTH_VOLUME_1 "##strm", ICON_FTH_VOLUME_2 "##strm", "##strm" };
        for (auto& stream : _streams) {
            ImGui::TableNextRow();
            bool isActive = false;
            if(stream._id == _activeStream) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,255,255,255));
                isActive = true;
            }
            ImGui::TableSetColumnIndex(0);
            if(ImGui::Selectable((std::string(stream._id == _activeStream ? playAnim[std::time(NULL)%3] : playAnim[3]) + std::to_string(stream._id)).c_str(), stream._id == _activeStream, ImGuiSelectableFlags_SpanAllColumns)) {
                _activeStream = stream._id;
                _rdb.deepFetch(stream);
                _tracks = stream._tracks;
                _activeTrack = 0;
                _player.setSource(stream);
                _player.play();
                _chat = _rdb.fetchChat(stream);
                scanForMaxNickSize();
                recalcMessageSize();
                _currentPage = CurrentPage::pTRACKS;
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", formattedTime(stream._timestamp).c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", stream._host.c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", stream._name.c_str());
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s", formattedDuration(stream._duration).c_str());
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("   %s", stream._chatChecksum ? ICON_FTH_CHECK : "-");
            if(isActive) {
                ImGui::PopStyleColor();
            }
        }
        ImGui::EndTable();
    }

    void renderTracks(ImVec2 pageSize)
    {
        static const char* types[] = {
            "-",
            ICON_FTH_MUSIC,
            ICON_FTH_MESSAGE_CIRCLE,
            ICON_FTH_BELL,
            ICON_FTH_MESSAGE_SQUARE
        };

        ImGui::BeginTable("TracksTable", 6,  ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollFreezeTopRow, pageSize);
        ImGui::TableSetupColumn(" ##tracks", ImGuiTableColumnFlags_WidthFixed,20);
        ImGui::TableSetupColumn("  Time##tracks", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Artist##tracks", ImGuiTableColumnFlags_WidthStretch, 0.35f);
        ImGui::TableSetupColumn("Title##tracks", ImGuiTableColumnFlags_WidthStretch, 0.40f);
        ImGui::TableSetupColumn("Duration##tracks", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Type##tracks", ImGuiTableColumnFlags_WidthFixed,60);
        ImGui::PushFont(_headerFont);
        ImGui::TableNextRow(0, 20);
        for(int i = 0; i < 6; ++i) {
            ImGui::TableSetColumnIndex(i);
            const char* column_name = ImGui::TableGetColumnName(i);
            ImGui::PushID(i);
            ImGui::TableHeader(column_name);
            ImGui::PopID();
        }
        ImGui::PopFont();
        static const char* playAnim[4] = { ICON_FTH_VOLUME "##trk", ICON_FTH_VOLUME_1 "##trk", ICON_FTH_VOLUME_2 "##strm", "##trk" };
        for (auto& track : _tracks) {
            ImGui::TableNextRow();
            bool isActive = false;
            if(track._id == _activeTrack) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,255,255,255));
                isActive = true;
            }
            ImGui::TableSetColumnIndex(0);
            if(ImGui::Selectable((std::string(track._id == _activeTrack ? playAnim[std::time(NULL)%3] : playAnim[3]) + std::to_string(track._id)).c_str(), track._id == _activeTrack, ImGuiSelectableFlags_SpanAllColumns)) {
                _rdb.deepFetch(track);
                _rdb.deepFetch(*track._stream);

                _player.setSource(track);
                _player.play();

                _currentPage = CurrentPage::pTRACKS;
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
            if(isActive) {
                ImGui::PopStyleColor();
            }
        }
        ImGui::EndTable();
    }

    void renderChat(ImVec2 pageSize)
    {
        ImGui::SetNextWindowContentSize(ImVec2(pageSize.x - 20, _chatLogHeight + ImGui::GetTextLineHeight()));
        ImGui::BeginChild("##ChatLog", ImVec2(pageSize.x - 4, pageSize.y));
        if(_forceScroll) {
            ImGui::SetScrollY(ImGui::GetScrollMaxY());
            _forceScroll = false;
        }
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        auto pos = ImGui::GetCursorScreenPos();
        auto scrollPos = ImGui::GetScrollY();
        float yoffset = 0;
        for(int i = 0; i <= _chatPosition; ++i) {
            auto msgcol = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]);
            if(yoffset >= scrollPos - 16 && yoffset < scrollPos + pageSize.y) {
                const auto& msg = _chat[i];
                drawList->AddText(ImVec2(pos.x, pos.y + yoffset), msgcol, formattedDuration(msg._time).c_str());
                if(msg.hasNick()) {
                    auto nick = msg.nick();
                    auto col = colorForString(nick);
                    if(msg._type != ChatMessage::eMessage) {
                        msgcol = col;
                    }

                    drawList->AddText(ImVec2(pos.x + 80 + _maxNickSize - ImGui::CalcTextSize(nick.c_str()).x, pos.y + yoffset), col, nick.c_str());
                }
                auto text = generateMessage(_chat[i]);
                drawList->AddText(nullptr, 0, ImVec2(pos.x + _maxNickSize + 100, pos.y + yoffset), msgcol, text.c_str(), nullptr, _chatMessageWidth);
            }
            yoffset += _messageSizes[i].y;
        }
        ImGui::EndChild();
    }

    void updatePlayRelatedInfo()
    {
        auto playTime = _player.playTime();
        if(playTime != _lastPlayPos) {
            _lastPlayPos = playTime;
            auto stream = _player.currentStream();
            if(stream) {
                // find active track
                int64_t lastId = 0;
                const Track* lastTrackInfo = nullptr;
                for(const auto& track : _tracks) {
                    if(track._time > playTime) {
                        if(_activeTrack != lastId) {
                            _activeTrack = lastId;
                            _activeTrackInfo = *lastTrackInfo;
                            _needsRefresh = true;
                        }
                        break;
                    }
                    lastId = track._id;
                    lastTrackInfo = &track;
                }
                if(_activeTrack != lastId) {
                    _activeTrack = lastId;
                    _needsRefresh = true;
                }
                int chatPos = -1;
                _chatLogHeight = 0;
                for(const auto& msg : _chat) {
                    if(msg._time <= playTime) {
                        ++chatPos;
                        _chatLogHeight += _messageSizes[chatPos].y;
                    }
                    else {
                        break;
                    }
                }
                if(_chatPosition != chatPos) {
                    _forceScroll = true;
                    _chatPosition = chatPos;
                }
            }
        }
    }

    void renderMainWindow()
    {
        updatePlayRelatedInfo();

        auto style = ImGui::GetStyle();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(_width-1, _height));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
        ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::SetCursorPos(ImVec2(8, 10.0f));
        ImGui::Text(ICON_FTH_MORE_VERTICAL);
        if(ImGui::IsItemClicked()) {
            ImGui::OpenPopup("main-more-menu");
        }
        if(ImGui::BeginPopup("main-more-menu")) {
            ImGui::MenuItem("More info reLiveG...");
            ImGui::MenuItem("Settings...");
            ImGui::Separator();
            if(ImGui::MenuItem("Quit")) {
                quit();
            }
            ImGui::EndPopup();
        }

        ImGui::PushFont(_headerFont);
        static const std::vector<std::string> menuLabels = {
            "Stations", "Streams", "Tracks", "Chat"
        };
        float spacing = 32.0f;
        float width = (menuLabels.size()-1) * spacing;
        for(const auto& label : menuLabels) {
            width += ImGui::CalcTextSize(label.c_str()).x;
        }
        auto offset = (_width - width) / 2.0f;
        ImGui::BeginGroup();
        ImGui::SetCursorPosY(20.0f);
        int idx = 0;
        for(const auto& label : menuLabels) {
            ImGui::SetCursorPosX(offset);
            if(idx == _currentPage) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 180, 250, 255));
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 100, 100, 255));
            }
            ImGui::Text("%s", label.c_str());
            if(ImGui::IsItemClicked()) {
                _currentPage = static_cast<CurrentPage>(idx);
            }
            ImGui::PopStyleColor();
            offset += ImGui::CalcTextSize(label.c_str()).x + spacing;
            ImGui::SameLine();
            ++idx;
        }
        ImGui::EndGroup();
        ImGui::PopFont();

        ImGui::SetCursorPos(ImVec2(_width - 200, 20.0f));
        ImGui::SetNextItemWidth(170);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8,3));
        ImGui::InputText(ICON_FTH_SEARCH "##search", &_searchText);
        ImGui::PopStyleVar(2);

        //ImGui::Separator();
        ImGui::SetCursorPos(ImVec2(8, TOP_PANEL_HEIGHT));

        auto pageSize = ImVec2(_width - 10, _height - TOP_PANEL_HEIGHT - BOTTOM_PANEL_HEIGHT);
        switch(_currentPage) {
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
        //ImGui::Separator();

        ImGui::SetCursorPos(ImVec2(8, _height - BOTTOM_PANEL_HEIGHT + 14));
        ImGui::BeginGroup();
        auto stream = _player.currentStream();
        if(stream) {
            ImGui::Text("%s - %s", stream->_station->_name.c_str(), stream->_name.c_str());
            auto currentPlayTime = _player.playTime();
            auto timeInfo = formattedDuration(currentPlayTime) + "/" + formattedDuration(stream->_duration);
            ImGui::SameLine(_width - PLAY_BAR_REL_END - ImGui::CalcTextSize(timeInfo.c_str()).x);
            ImGui::Text("%s", timeInfo.c_str());
            if(_activeTrackInfo._artist.empty()) {
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
            ImGui::Text("");
            ImGui::Text("");
        }
        //ImGui::SetCursorPos(ImVec2(4, _height - PLAY_BAR_HEIGHT - 4));
        if(_progress) {
            ImGui::ProgressBar(_progress / 100.0f, ImVec2(_width - PLAY_BAR_REL_END, PLAY_BAR_HEIGHT));
        }
        else {
            renderPlayBar(ImVec2(_width - PLAY_BAR_REL_END, PLAY_BAR_HEIGHT));
        }
        ImGui::EndGroup();
        //ImGui::SameLine();

        ImGui::SetCursorPos(ImVec2(_width - PLAY_CONTROLS_WIDTH, _height - BOTTOM_PANEL_HEIGHT + 14));
        renderPlayControl();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        auto pos = ImGui::GetWindowPos();
        drawList->AddRectFilledMultiColor(ImVec2(pos.x, pos.y + TOP_PANEL_HEIGHT), ImVec2(pos.x + _width, pos.y + TOP_PANEL_HEIGHT + PANEL_SHADOW),
                                          IM_COL32(0,0,0,128), IM_COL32(0,0,0,128),
                                          IM_COL32(0,0,0,0), IM_COL32(0,0,0,0));

        drawList->AddRectFilledMultiColor(ImVec2(pos.x,pos.y + _height - BOTTOM_PANEL_HEIGHT - PANEL_SHADOW), ImVec2(pos.x + _width, pos.y + _height - BOTTOM_PANEL_HEIGHT),
                                          IM_COL32(0,0,0,0), IM_COL32(0,0,0,0),
                                          IM_COL32(0,0,0,255), IM_COL32(0,0,0,255));

        ImGui::End();
        ImGui::PopStyleVar(2);
    }

private:
    void progress(int percent)
    {
        std::lock_guard<std::mutex> lock{_mutex};
        _progress = percent;
        _needsRefresh = true;
    }


    static uint32_t hashFNV1a(const char *str, uint32_t hval = 0)
    {
        auto s = (uint8_t *)str;
        while (*s) {
            hval ^= (uint32_t)*s++;
            hval += (hval<<1u) + (hval<<4u) + (hval<<7u) + (hval<<8u) + (hval<<24u);
        }
        return hval;
    }

    static ImU32 colorForString(const std::string& str)
    {
        auto hash = hashFNV1a(str.c_str());
        float r = 0, g = 0, b = 0;
        ImGui::ColorConvertHSVtoRGB((hash & 0xffu) / 255.0f, 0.5f, (255 - ((hash >> 8u) & 31u)) / 255.0f, r, g, b);
        return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 255));
    }

    static std::string generateMessage(const ChatMessage& msg)
    {
        switch(msg._type) {
            case ChatMessage::eMe:
            case ChatMessage::eMode:
            case ChatMessage::eKick:
                return msg._strings.back();
            case ChatMessage::eNick:
                return "is now known as " + msg._strings.back();
            case ChatMessage::eJoin:
                return "has joined the channel";
            case ChatMessage::eLeave:
                return "has left the channel " + (msg._strings.size()>1 ? "("+msg._strings[1]+")":"");
            case ChatMessage::eQuit:
                return "has quit " + (msg._strings.size()>1 ? "("+msg._strings[1]+")":"");
            case ChatMessage::eTopic:
                return "has changed the topic to: " + msg._strings.back();
            default:
                return msg._strings.back();
        }
    }

    void scanForMaxNickSize()
    {
        std::set<std::string> nicks;
        for(const auto& msg : _chat) {
            auto nick = msg.nick();
            if(!nick.empty() && !nicks.count(nick)) {
                nicks.insert(nick);
                auto l = ImGui::CalcTextSize(msg._strings.front().c_str()).x;
                if(l > _maxNickSize) {
                    _maxNickSize = l;
                }
            }
        }
    }

    void recalcMessageSize()
    {
        _messageSizes.clear();
        _messageSizes.reserve(_chat.size());
        _chatLogHeight = 0;
        _chatMessageWidth = _width - _maxNickSize - 130;
        for(const ChatMessage& msg : _chat) {
            auto text = generateMessage(msg);
            _messageSizes.push_back(ImGui::CalcTextSize(text.c_str(), nullptr, false, _chatMessageWidth));
            _chatLogHeight += _messageSizes.back().y;
        }
    }

    float _width = 0;
    float _height = 0;
    bool _show_demo_window = true;
    std::mutex _mutex;
    ReLiveDB _rdb;
    int64_t _lastFetch = 0;
    int _lastPlayPos = 0;
    Player _player;
    ImFont* _propFont = nullptr;
    ImFont* _headerFont = nullptr;
    ImFont* _monoFont = nullptr;
    std::atomic_bool _needsRefresh;
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
};

}


int main(int argc, char* argv[])
{
    using namespace relive;

    try {
        setAppName(RELIVE_APP_NAME);
        LogManager::setOutputFile(relive::dataPath() + "/" + appName() + ".log");
        LogManager::instance()->defaultLevel(3);
        if(isInstanceRunning()) {
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
          RtAudio audio;
          audio.showWarnings(false);
          unsigned int devices = audio.getDeviceCount();
          RtAudio::DeviceInfo info;
          for(unsigned int i=0; i<devices; ++i) {
              info = audio.getDeviceInfo( i );
              if(info.probed && info.outputChannels >= 2) {
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
          exit(0);
        });
        parser.onOpt({"-s?", "--default-station?"}, "[<name>]\tSet the default station to switch to on startup, only significant part of the name is needed. Without a parameter, this resets to starting on station screen.", [&](std::string str){
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
        parser.parse();

        ReLiveApp app;
        app.run();
    }
    catch(std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        exit(1);
    }        return 0;
}

