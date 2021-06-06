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
    ImGui::SetNextWindowSize(ImVec2(600, 350));
    if(ImGui::BeginPopupModal("About reLiveG")) {
        ImGui::BeginChild("##reLiveAboutText", ImVec2(590, 290));
        if(g_info.find("@VERSION@") != std::string::npos) {
            auto pos = g_info.find("@VERSION@");
            g_info.replace(pos, 9, std::string(RELIVE_VERSION_STRING_LONG));
            pos = g_info.find("@VERLINE@");
            g_info.replace(pos, 9, std::string(strlen(RELIVE_VERSION_STRING_LONG), '-'));
        }
        ImGui::TextWrapped("%s", g_info.c_str());
        ImGui::EndChild();
        if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }

    //-----------------------------------------------------------------------------------------
    // SETTINGS DIALOG
    //-----------------------------------------------------------------------------------------
    static std::__1::vector<Player::Device> outputDevices;
    if(openSettings) {
        ImGui::OpenPopup("reLiveG Settings");
        outputDevices = _player.getOutputDevices();
    }
    ImGui::SetNextWindowPos(ImVec2(_width *0.5f, _height *0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 350));
    if(ImGui::BeginPopupModal("reLiveG Settings", nullptr)) {
        ImGui::BeginChild("##settingstabarea", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()*2.0));
        ImGui::Spacing();
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("SettingsTabBar", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("General"))
            {
                ImGui::Text("This is the Avocado tab!\nblah blah blah blah blah");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Appearance"))
            {
                if(ImGui::Checkbox("Dark mode", &_darkMode)) {
                    _style.setTheme(_darkMode ? StyleManager::DarkTheme : StyleManager::LightTheme);
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Audio"))
            {
                static auto currentDeviceName = outputDevices.empty() ? "" : outputDevices.front().name;
                if(ImGui::BeginCombo("Output device", _outputDevice.name.c_str())) {
                    for (const auto& device : outputDevices) {
                        ImGui::PushID(device.name.c_str());
                        if (ImGui::Selectable(device.name.c_str(), _outputDevice.name == device.name)) {
                            if (_outputDevice.name != device.name) {
                                _player.configureAudio(device.name);
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
        if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::SetItemDefaultFocus();
        //ImGui::SameLine();
        //if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
}


}