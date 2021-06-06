#pragma once

#include <imguix/utility.h>

#include <cmath>
#include <functional>
#include <utility>
#include <vector>

#include "logging.h"

namespace igx {

inline bool CurveEditor(const char* label,
                        ImVec2 size,
                        std::vector<std::pair<float, float>>& points,
                        const std::function<float(float)>& plot_source = std::function<float(float)>(),
                        const std::function<std::string(float)>& label_source = std::function<std::string(float)>(),
                        int steps = 0)
{
    const float handle_radius = 4.0f;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGuiStyle& style = ImGui::GetStyle();
    auto avail = ImGui::GetContentRegionAvail();
    if (size.x <= 0) {
        size.y = avail.x;
    }
    if (size.y <= 0) {
        size.y = avail.y;
    }
    auto p1 = ImGui::GetCursorScreenPos();
    auto p2 = ImVec2(p1.x + size.x, p1.y + size.y);
    auto rect = ImRect(p1, p2);
    ImGui::ItemSize(rect);
    if (!ImGui::ItemAdd(rect, ImGui::GetID(label))) {
        return false;
    }
    draw_list->PathRect(p1, p2, 4);
    draw_list->PathFillConvex(ImGui::GetColorU32(ImGuiCol_WindowBg));
    float centerY = p1.y + size.y / 2.0f;

    // draw plot
    ImVec2 ph;
    if (plot_source) {
        for (int x = 0; x < int(size.x); ++x) {
            auto p = ImVec2(p1.x + float(x), centerY - plot_source(float(x) / (size.x-1)) * size.y / 2);
            if (x > 0)
                draw_list->AddLine(ph, p, ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
            ph = p;
        }
    }

    // draw control lines
    for (int idx = 0; idx < points.size() - 1; ++idx) {
        auto& p = points[idx];
        auto& pn = points[idx + 1];
        draw_list->AddLine(ImVec2(p1.x + p.first * size.x, p1.y + size.y - p.second * size.y), ImVec2(p1.x + pn.first * size.x, p1.y + size.y - pn.second * size.y), ImGui::GetColorU32(ImGuiCol_PlotLines), 3);
    }

    // draw and handle control points
    bool changed = false;
    bool hovering = false;
    static int dragIndex = -1;
    ImVec2 addPointPos{-1.0f, -1.0f};
    int addPointIndex = -1;
    int remPointIndex = -1;
    for (int idx = 0; idx < points.size(); ++idx) {
        auto& p = points[idx];
        auto pos = ImVec2(p1.x + p.first * size.x, p1.y + size.y - p.second * size.y);
        if (ImGui::IsMouseHoveringRect(ImVec2(pos.x - handle_radius, pos.y - handle_radius), ImVec2(pos.x + handle_radius, pos.y + handle_radius))) {
            draw_list->AddCircleFilled(pos, 5, ImGui::GetColorU32(ImGuiCol_Text), 4);
            hovering = true;
            if (ImGui::IsMouseClicked(0)) {
                dragIndex = idx;
            }
            else if (ImGui::IsMouseReleased(0)) {
                dragIndex = -1;
            }
        }
        else {
            draw_list->AddCircle(pos, 5, ImGui::GetColorU32(ImGuiCol_Text), 4, 2);
        }
        if(label_source) {
            auto label = label_source(p.second);
            auto label_size = ImGui::CalcTextSize(label.c_str());
            auto label_pos = ImVec2(pos.x + 8, pos.y - 5);
            if(label_pos.x + label_size.x > p1.x + size.x) {
                label_pos.x = pos.x - label_size.x - 8;
            }
            if(label_pos.y + label_size.y > p1.y + size.y) {
                label_pos.y = pos.y - label_size.y;
            }
            else if(label_pos.y < p1.y + 3) {
                label_pos.y = p1.y + 3;
            }
            draw_list->AddText(label_pos, ImGui::GetColorU32(ImGuiCol_Text), label.c_str());
        }
        if (dragIndex == idx && ImGui::IsMouseDragging(0)) {
            if (idx > 0 && idx < points.size() - 1) {
                p.first = ImClamp((ImGui::GetIO().MousePos.x - p1.x) / size.x, points[idx - 1].first, points[idx + 1].first);
            }
            p.second = quantize(ImClamp(1.0f - (ImGui::GetIO().MousePos.y - p1.y) / size.y, 0.0f, 1.0f), steps);
            changed = true;
        }
        if(idx < points.size()-1) {
            auto pn = points[idx + 1];
            auto pos2 = ImVec2(p1.x + pn.first * size.x, p1.y + size.y - pn.second * size.y);
            if (addPointIndex < 0 && !hovering && dragIndex < 0 && ImGui::IsMouseClicked(0)) {
                if (distancePtLine(pos, pos2, ImGui::GetIO().MousePos) < handle_radius * 2 && distance(pos, ImGui::GetIO().MousePos) > handle_radius * 4 && distance(pos2, ImGui::GetIO().MousePos) > handle_radius * 4) {
                    auto pm = ImVec2((ImGui::GetIO().MousePos.x - p1.x) / size.x, 1.0f - (ImGui::GetIO().MousePos.y - p1.y) / size.y);
                    addPointPos = ImLineClosestPoint(ImVec2(points[idx].first, points[idx].second), ImVec2(points[idx + 1].first, points[idx + 1].second), pm);
                    addPointIndex = idx + 1;
                }
            }
            if (distance(pos, pos2) < handle_radius * 1.5f) {
                remPointIndex = (idx ? idx : idx + 1);
            }
        }
    }
    if (remPointIndex > 0) {
        points.erase(points.begin() + remPointIndex);
    }
    if (addPointIndex > 0) {
        points.insert(points.begin() + addPointIndex, std::make_pair(addPointPos.x, quantize(addPointPos.y, steps)));
        changed = true;
    }
    return changed;
}

}  // namespace igx