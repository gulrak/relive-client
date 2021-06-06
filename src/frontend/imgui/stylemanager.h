//---------------------------------------------------------------------------------------
// SPDX-License-Identifier: BSD-3-Clause
// relive-client - A C++ implementation of the reLive protocol and an sqlite backend
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//---------------------------------------------------------------------------------------
#pragma once

#include <imgui/imgui.h>
#include <vector>

enum reLiveColor : int {
    reLiveCol_custom_start = ImGuiCol_COUNT,
    reLiveCol_MainMenuText,     // top main menu text
    reLiveCol_MainMenuTextSelected, // top main menu text selected
    reLiveCol_PlaybarStripe0,   // reLive bar stripe color 0
    reLiveCol_PlaybarStripe1,   // relive bar stripe color 1
    reLiveCol_PlaybarBorder,    // relive bar border
    reLiveCol_PlaybarEmpty,     // relive bar fill color if no stream
    reLiveCol_PlaybarCursor,    // relive bar cursor color
    reLiveCol_TableActiveLine,  // table row text color when active/selected
    reLiveCol_TableUnplayed,    // table row of unplayed streams or stations with unplayed streams
    reLiveCol_TablePlayed,      // table row of played streams or stations with only played streams
    reLiveCol_max_color
};

class StyleManager
{
public:
    enum Predefined { DarkTheme, LightTheme };
    StyleManager() = default;

    ImU32 getColor(int colIdx)
    {
        if(colIdx < reLiveCol_custom_start) {
            return ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[colIdx]);
        }
        return _colors[colIdx - reLiveCol_custom_start];
    }

    const char* getColorName(int colIdx) const
    {
        if(colIdx < reLiveCol_custom_start) {
            return ImGui::GetStyleColorName(colIdx);
        }
        switch (colIdx) {
            case reLiveCol_MainMenuText:
                return "MainMenuText";
            case reLiveCol_MainMenuTextSelected:
                return "MainMenuTextSelected";
            case reLiveCol_PlaybarStripe0:
                return "reLiveBarStripe0";
            case reLiveCol_PlaybarStripe1:
                return "reLiveBarStripe1";
            case reLiveCol_PlaybarBorder:
                return "reLiveBarBorder";
            case reLiveCol_PlaybarEmpty:
                return "reLiveBarEmpty";
            case reLiveCol_PlaybarCursor:
                return "reLiveBarCursor";
            case reLiveCol_TableActiveLine:
                return "TableActiveLine";
            case reLiveCol_TableUnplayed:
                return "TableUnplayed";
            case reLiveCol_TablePlayed:
                return "TablePlayed";
            default:
                return "";
        }
    }

    void setColor(int colIdx, ImU32 color)
    {
        if(colIdx < reLiveCol_custom_start) {
            ImGui::GetStyle().Colors[colIdx] = ImGui::ColorConvertU32ToFloat4(color);
        }
        else {
            _colors[colIdx - reLiveCol_custom_start] = color;
        }
    }

    void setColor(int colIdx, const ImVec4& color)
    {
        if(colIdx < reLiveCol_custom_start) {
            ImGui::GetStyle().Colors[colIdx] = color;
        }
        else {
            _colors[colIdx - reLiveCol_custom_start] = ImGui::ColorConvertFloat4ToU32(color);
        }
    }

    ImVec4 getColorIV4(int colIdx)
    {
        if(colIdx < reLiveCol_custom_start) {
            return ImGui::GetStyle().Colors[colIdx];
        }
        else {
            return ImGui::ColorConvertU32ToFloat4(_colors[colIdx - reLiveCol_custom_start]);
        }
    }

    void pushColor(int colIdx, int colIdx2)
    {
        assert(colIdx < colIdx2);
        ImGui::PushStyleColor(colIdx, getColor(colIdx2));
    }

    void setTheme(Predefined theme)
    {
        _colors.resize(reLiveCol_max_color - reLiveCol_custom_start);
        switch (theme) {
            default:
            case DarkTheme: {
                setColor(ImGuiCol_Text, ImVec4(0.80f, 0.80f, 0.80f, 1.00f));
                setColor(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
                setColor(ImGuiCol_WindowBg, ImVec4(0.16f, 0.17f, 0.18f, 1.00f));
                setColor(ImGuiCol_ChildBg, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
                setColor(ImGuiCol_PopupBg, ImVec4(0.25f, 0.25f, 0.25f, 1.00f));
                setColor(ImGuiCol_Border, ImVec4(0.43f, 0.43f, 0.50f, 0.50f));
                setColor(ImGuiCol_BorderShadow, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
                setColor(ImGuiCol_FrameBg, ImVec4(0.39f, 0.41f, 0.42f, 1.00f));
                setColor(ImGuiCol_FrameBgHovered, ImVec4(0.26f, 0.59f, 0.98f, 0.40f));
                setColor(ImGuiCol_FrameBgActive, ImVec4(0.26f, 0.59f, 0.98f, 0.67f));
                setColor(ImGuiCol_TitleBg, ImVec4(0.04f, 0.04f, 0.04f, 1.00f));
                setColor(ImGuiCol_TitleBgActive, ImVec4(0.16f, 0.29f, 0.48f, 1.00f));
                setColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.00f, 0.00f, 0.00f, 0.51f));
                setColor(ImGuiCol_MenuBarBg, ImVec4(0.14f, 0.14f, 0.14f, 1.00f));
                setColor(ImGuiCol_ScrollbarBg, ImVec4(0.16f, 0.17f, 0.18f, 1.00f));
                setColor(ImGuiCol_ScrollbarGrab, ImVec4(0.31f, 0.31f, 0.31f, 1.00f));
                setColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.41f, 0.41f, 0.41f, 1.00f));
                setColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.51f, 0.51f, 0.51f, 1.00f));
                setColor(ImGuiCol_CheckMark, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
                setColor(ImGuiCol_SliderGrab, ImVec4(0.24f, 0.52f, 0.88f, 1.00f));
                setColor(ImGuiCol_SliderGrabActive, ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
                setColor(ImGuiCol_Button, ImVec4(0.08f, 0.37f, 0.82f, 1.00f));
                setColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
                setColor(ImGuiCol_ButtonActive, ImVec4(0.06f, 0.53f, 0.98f, 1.00f));
                setColor(ImGuiCol_Header, ImVec4(0.26f, 0.59f, 0.98f, 0.31f));
                setColor(ImGuiCol_HeaderHovered, ImVec4(0.30f, 0.30f, 0.30f, 0.80f));
                setColor(ImGuiCol_HeaderActive, ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
                setColor(ImGuiCol_Separator, ImVec4(0.43f, 0.43f, 0.50f, 0.50f));
                setColor(ImGuiCol_SeparatorHovered, ImVec4(0.10f, 0.40f, 0.75f, 0.78f));
                setColor(ImGuiCol_SeparatorActive, ImVec4(0.10f, 0.40f, 0.75f, 1.00f));
                setColor(ImGuiCol_ResizeGrip, ImVec4(0.26f, 0.59f, 0.98f, 0.25f));
                setColor(ImGuiCol_ResizeGripHovered, ImVec4(0.26f, 0.59f, 0.98f, 0.67f));
                setColor(ImGuiCol_ResizeGripActive, ImVec4(0.26f, 0.59f, 0.98f, 0.95f));
                setColor(ImGuiCol_Tab, ImVec4(0.18f, 0.35f, 0.58f, 0.86f));
                setColor(ImGuiCol_TabHovered, ImVec4(0.26f, 0.59f, 0.98f, 0.80f));
                setColor(ImGuiCol_TabActive, ImVec4(0.20f, 0.41f, 0.68f, 1.00f));
                setColor(ImGuiCol_TabUnfocused, ImVec4(0.07f, 0.10f, 0.15f, 0.97f));
                setColor(ImGuiCol_TabUnfocusedActive, ImVec4(0.14f, 0.26f, 0.42f, 1.00f));
                setColor(ImGuiCol_PlotLines, ImVec4(0.61f, 0.61f, 0.61f, 1.00f));
                setColor(ImGuiCol_PlotLinesHovered, ImVec4(1.00f, 0.43f, 0.35f, 1.00f));
                setColor(ImGuiCol_PlotHistogram, ImVec4(0.90f, 0.70f, 0.00f, 1.00f));
                setColor(ImGuiCol_PlotHistogramHovered, ImVec4(1.00f, 0.60f, 0.00f, 1.00f));
                setColor(ImGuiCol_TableHeaderBg, ImVec4(0.19f, 0.19f, 0.20f, 1.00f));
                setColor(ImGuiCol_TableBorderStrong, ImVec4(0.31f, 0.31f, 0.35f, 1.00f));
                setColor(ImGuiCol_TableBorderLight, ImVec4(0.23f, 0.23f, 0.25f, 1.00f));
                setColor(ImGuiCol_TableRowBg, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
                setColor(ImGuiCol_TableRowBgAlt, ImVec4(1.00f, 1.00f, 1.00f, 0.07f));
                setColor(ImGuiCol_TextSelectedBg, ImVec4(0.26f, 0.59f, 0.98f, 0.35f));
                setColor(ImGuiCol_DragDropTarget, ImVec4(1.00f, 1.00f, 0.00f, 0.90f));
                setColor(ImGuiCol_NavHighlight, ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
                setColor(ImGuiCol_NavWindowingHighlight, ImVec4(1.00f, 1.00f, 1.00f, 0.70f));
                setColor(ImGuiCol_NavWindowingDimBg, ImVec4(0.80f, 0.80f, 0.80f, 0.20f));
                setColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.00f, 0.00f, 0.00f, 0.50f));
                setColor(reLiveCol_MainMenuText, IM_COL32(100, 100, 100, 255));
                setColor(reLiveCol_MainMenuTextSelected, IM_COL32(100, 180, 250, 255));
                setColor(reLiveCol_PlaybarStripe0, IM_COL32(100,100,100,255));
                setColor(reLiveCol_PlaybarStripe1, IM_COL32(130,130,130,255));
                setColor(reLiveCol_PlaybarBorder, IM_COL32(200,200,200,255));
                setColor(reLiveCol_PlaybarEmpty, IM_COL32(60,60,60,255));
                setColor(reLiveCol_PlaybarCursor, IM_COL32(220,220,220,255));
                setColor(reLiveCol_TableActiveLine, IM_COL32(255,255,255,255));
                setColor(reLiveCol_TableUnplayed, IM_COL32(255,255,255,255));
                setColor(reLiveCol_TablePlayed, IM_COL32(150,150,150,255));
                break;
            }
            case LightTheme: {
                ImGui::GetStyle().FrameBorderSize = 1.0f;
                setColor(ImGuiCol_Text,  ImVec4(0.13f, 0.13f, 0.13f, 1.00f));
                setColor(ImGuiCol_TextDisabled,  ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
                setColor(ImGuiCol_WindowBg,  ImVec4(0.73f, 0.77f, 0.82f, 1.00f));
                setColor(ImGuiCol_ChildBg,  ImVec4(0.88f, 0.88f, 0.88f, 0.00f));
                setColor(ImGuiCol_PopupBg,  ImVec4(0.76f, 0.81f, 0.86f, 1.00f));
                setColor(ImGuiCol_Border,  ImVec4(0.47f, 0.47f, 0.55f, 0.50f));
                setColor(ImGuiCol_BorderShadow,  ImVec4(0.18f, 0.18f, 0.18f, 0.00f));
                setColor(ImGuiCol_FrameBg,  ImVec4(0.80f, 0.87f, 0.90f, 1.00f));
                setColor(ImGuiCol_FrameBgHovered,  ImVec4(0.26f, 0.59f, 0.98f, 0.40f));
                setColor(ImGuiCol_FrameBgActive,  ImVec4(0.26f, 0.59f, 0.98f, 0.67f));
                setColor(ImGuiCol_TitleBg,  IM_COL32(192, 221, 240, 255));
                setColor(ImGuiCol_TitleBgActive,  IM_COL32(216, 229, 247, 255));
                setColor(ImGuiCol_TitleBgCollapsed,  IM_COL32(147, 177, 196, 255));
                setColor(ImGuiCol_MenuBarBg,  ImVec4(0.76f, 0.81f, 0.86f, 1.00f));
                setColor(ImGuiCol_ScrollbarBg,  ImVec4(0.67f, 0.74f, 0.82f, 1.00f));
                setColor(ImGuiCol_ScrollbarGrab,  ImVec4(0.59f, 0.59f, 0.59f, 1.00f));
                setColor(ImGuiCol_ScrollbarGrabHovered,  ImVec4(0.41f, 0.41f, 0.41f, 1.00f));
                setColor(ImGuiCol_ScrollbarGrabActive,  ImVec4(0.51f, 0.51f, 0.51f, 1.00f));
                setColor(ImGuiCol_CheckMark,  ImVec4(0.12f, 0.12f, 0.12f, 1.00f));
                setColor(ImGuiCol_SliderGrab,  ImVec4(0.45f, 0.55f, 0.68f, 1.00f));
                setColor(ImGuiCol_SliderGrabActive,  ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
                setColor(ImGuiCol_Button,  ImVec4(0.83f, 0.88f, 0.95f, 1.00f));
                setColor(ImGuiCol_ButtonHovered,  ImVec4(0.67f, 0.80f, 0.96f, 1.00f));
                setColor(ImGuiCol_ButtonActive,  ImVec4(0.06f, 0.53f, 0.98f, 1.00f));
                setColor(ImGuiCol_Header,  ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
                setColor(ImGuiCol_HeaderHovered,  IM_COL32(203, 227, 251, 255));
                setColor(ImGuiCol_HeaderActive,  ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
                setColor(ImGuiCol_Separator,  ImVec4(0.43f, 0.43f, 0.50f, 0.50f));
                setColor(ImGuiCol_SeparatorHovered,  ImVec4(0.10f, 0.40f, 0.75f, 0.78f));
                setColor(ImGuiCol_SeparatorActive,  ImVec4(0.10f, 0.40f, 0.75f, 1.00f));
                setColor(ImGuiCol_ResizeGrip,  ImVec4(0.26f, 0.59f, 0.98f, 0.25f));
                setColor(ImGuiCol_ResizeGripHovered,  ImVec4(0.26f, 0.59f, 0.98f, 0.67f));
                setColor(ImGuiCol_ResizeGripActive,  ImVec4(0.26f, 0.59f, 0.98f, 0.95f));
                setColor(ImGuiCol_Tab,  ImVec4(0.26f, 0.51f, 0.85f, 0.86f));
                setColor(ImGuiCol_TabHovered,  ImVec4(0.41f, 0.68f, 1.00f, 0.80f));
                setColor(ImGuiCol_TabActive,  ImVec4(0.29f, 0.61f, 1.00f, 1.00f));
                setColor(ImGuiCol_TabUnfocused,  ImVec4(0.36f, 0.52f, 0.77f, 0.97f));
                setColor(ImGuiCol_TabUnfocusedActive,  ImVec4(0.26f, 0.49f, 0.79f, 1.00f));
                setColor(ImGuiCol_PlotLines,  ImVec4(0.61f, 0.61f, 0.61f, 1.00f));
                setColor(ImGuiCol_PlotLinesHovered,  ImVec4(1.00f, 0.43f, 0.35f, 1.00f));
                setColor(ImGuiCol_PlotHistogram,  ImVec4(0.90f, 0.70f, 0.00f, 1.00f));
                setColor(ImGuiCol_PlotHistogramHovered,  ImVec4(1.00f, 0.60f, 0.00f, 1.00f));
                setColor(ImGuiCol_TableHeaderBg,  ImVec4(0.59f, 0.59f, 0.74f, 1.00f));
                setColor(ImGuiCol_TableBorderStrong,  ImVec4(0.31f, 0.31f, 0.35f, 1.00f));
                setColor(ImGuiCol_TableBorderLight,  ImVec4(0.23f, 0.23f, 0.25f, 1.00f));
                setColor(ImGuiCol_TableRowBg,  ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
                setColor(ImGuiCol_TableRowBgAlt,  ImVec4(1.00f, 1.00f, 1.00f, 0.07f));
                setColor(ImGuiCol_TextSelectedBg,  ImVec4(0.26f, 0.59f, 0.98f, 0.35f));
                setColor(ImGuiCol_DragDropTarget,  ImVec4(1.00f, 1.00f, 0.00f, 0.90f));
                setColor(ImGuiCol_NavHighlight,  ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
                setColor(ImGuiCol_NavWindowingHighlight,  ImVec4(1.00f, 1.00f, 1.00f, 0.70f));
                setColor(ImGuiCol_NavWindowingDimBg,  ImVec4(0.80f, 0.80f, 0.80f, 0.20f));
                setColor(ImGuiCol_ModalWindowDimBg,  ImVec4(0.81f, 0.81f, 0.81f, 0.50f));
                setColor(reLiveCol_MainMenuText, IM_COL32(0, 0, 0, 255));
                setColor(reLiveCol_MainMenuTextSelected, IM_COL32(0, 97, 164, 255));
                setColor(reLiveCol_PlaybarStripe0, derivedColorV(ImVec4(0.73f, 0.77f, 0.82f, 1.00f), 0.7));
                setColor(reLiveCol_PlaybarStripe1, derivedColorV(ImVec4(0.73f, 0.77f, 0.82f, 1.00f), 0.9));
                setColor(reLiveCol_PlaybarBorder, IM_COL32(200,200,200,255));
                setColor(reLiveCol_PlaybarEmpty, IM_COL32(60,60,60,255));
                setColor(reLiveCol_PlaybarCursor, IM_COL32(32,32,32,255));
                setColor(reLiveCol_TableActiveLine, IM_COL32(255,255,255,255));
                setColor(reLiveCol_TableUnplayed, ImVec4(0.00f, 0.00f, 0.00f, 1.00f));
                setColor(reLiveCol_TablePlayed, ImVec4(0.30f, 0.30f, 0.30f, 1.00f));
                break;
            }
        }
    }
private:
    static float clamp(float d, float min = 0.0f, float max = 1.0f) {
        const float t = d < min ? min : d;
        return t > max ? max : t;
    }

    static ImU32 derivedColorV(ImVec4 col, float vv)
    {
        float h = 0, s = 0, v = 0;
        ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, h, s, v);
        float r = 0, g = 0, b = 0;
        ImGui::ColorConvertHSVtoRGB(h, s, vv, r, g, b);
        return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, col.w));
    }
    std::vector<ImU32> _colors;
};
