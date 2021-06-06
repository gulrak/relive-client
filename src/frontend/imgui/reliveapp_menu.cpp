//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------

#include "reliveapp.hpp"
#include "info.hpp"

#include <resources/feather_icons.h>
#include <version/version.hpp>

namespace relive {

void ReLiveApp::renderAppMenu()
{  //-----------------------------------------------------------------------------------------
    // MENU ROOT
    //-----------------------------------------------------------------------------------------
    ImGui::SetCursorPos(ImVec2(8, 10.0f));
    ImGui::Text(ICON_FTH_MORE_VERTICAL);
    if(ImGui::IsItemClicked()) {
        ImGui::OpenPopup("main-more-menu");
    }
    bool openAboutDialog = false;
    bool openSettings = false;
    if(ImGui::BeginPopup("main-more-menu")) {
        if(ImGui::MenuItem("More info reLiveG...")) {
            openAboutDialog = true;
        }
        if(ImGui::MenuItem("Settings...")) {
            openSettings = true;
        }

        ImGui::Separator();
        if(ImGui::MenuItem("Quit")) {
            quit();
        }
        ImGui::EndPopup();
    }

    //-----------------------------------------------------------------------------------------
    // ABOUT DIALOG
    //-----------------------------------------------------------------------------------------
    if(openAboutDialog) {
        ImGui::OpenPopup("About reLiveG");
    }
    ImGui::SetNextWindowPos(ImVec2(_width *0.5f, _height *0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(700, 350));
    if(ImGui::BeginPopupModal("About reLiveG")) {
        ImGui::BeginChild("##reLiveAboutText", ImVec2(690, 290));
        if(g_info.find("@VERSION@") != std::string::npos) {
            auto pos = g_info.find("@VERSION@");
            g_info.replace(pos, 9, std::string(RELIVE_VERSION_STRING_LONG));
            pos = g_info.find("@VERLINE@");
            g_info.replace(pos, 9, std::string(strlen(RELIVE_VERSION_STRING_LONG), '-'));
        }
        ImGui::TextUnformatted(g_info.c_str(), g_info.c_str() + g_info.size());
        ImGui::EndChild();
        if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }

    //-----------------------------------------------------------------------------------------
    // SETTINGS DIALOG
    //-----------------------------------------------------------------------------------------
    static std::__1::vector<Player::Device> outputDevices;
    static std::string defaultStation;
    if(openSettings) {
        ImGui::OpenPopup("reLiveG Settings");
        outputDevices = _player.getOutputDevices();
        defaultStation = _rdb.getConfigValue(Keys::default_station, defaultStation);
    }
    ImGui::SetNextWindowPos(ImVec2(_width *0.5f, _height *0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 350));
    if(ImGui::BeginPopupModal("reLiveG Settings", nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::BeginChild("##settingstabarea", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()*2.0));
        ImGui::Spacing();
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("SettingsTabBar", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("General"))
            {
                ImGui::Spacing();
                ImGui::Indent();
                if(ImGui::Checkbox("Remember last play position", &_startAtLastPosition)) {
                    _rdb.setConfigValue(Keys::start_at_last_position, _startAtLastPosition);
                }
                if(ImGui::BeginCombo("Default station##settings", defaultStation.c_str())) {
                    for(const auto& station : _stations) {
                        ImGui::PushID(station._name.c_str());
                        if (ImGui::Selectable(station._name.c_str(), defaultStation == station._name)) {
                            if (defaultStation != station._name) {
                                _rdb.setConfigValue(Keys::default_station, station._name);
                            }
                            defaultStation = station._name;
                        }
                        ImGui::PopID();
                    }
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Debug:");
                if(ImGui::Checkbox("Show receive buffer bar", &_receiveBufferBar)) {
                    _rdb.setConfigValue(Keys::show_buffer_bar, _receiveBufferBar);
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Appearance"))
            {
                ImGui::Spacing();
                ImGui::Indent();
                if(ImGui::Checkbox("Dark mode", &_darkMode)) {
                    _style.setTheme(_darkMode ? StyleManager::DarkTheme : StyleManager::LightTheme);
                    _rdb.setConfigValue(Keys::use_dark_theme, _darkMode);
                }
                static std::string testString = "TestName";
                ImGui::PushItemWidth(200);
                ImGui::InputInt("Name coloring seed", &_nameColorSeed);
                ImGui::SameLine();
                if(ImGui::Button("Random")) {
                    _nameColorSeed = rand() & 0xffffffff;
                }
                ImGui::PushStyleColor(ImGuiCol_Text, colorForString(testString));
                ImGui::InputText("##Testname", &testString);
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::Text("Test name");
                ImGui::PopItemWidth();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Audio"))
            {
                ImGui::Spacing();
                static auto currentDeviceName = outputDevices.empty() ? "" : outputDevices.front().name;
                ImGui::Indent();
                if(ImGui::BeginCombo("Output device", _outputDevice.name.c_str())) {
                    for (const auto& device : outputDevices) {
                        ImGui::PushID(device.name.c_str());
                        if (ImGui::Selectable(device.name.c_str(), _outputDevice.name == device.name)) {
                            if (_outputDevice.name != device.name) {
                                _player.configureAudio(device.name);
                                _rdb.setConfigValue(Keys::output_device, _outputDevice.name);
                            }
                            _outputDevice = device;
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndCombo();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::SameLine(ImGui::GetWindowWidth()-140);
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            _rdb.setConfigValue(Keys::name_color_seed, _nameColorSeed);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        //ImGui::SameLine();
        //if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
}


}