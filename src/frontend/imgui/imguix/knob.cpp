#include "knob.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace igx {

template <typename T>
bool Knob(const char* label, T* value_p, T minv, T maxv, const char* value_text, float diameter, float width)
{
    ImGuiStyle& style = ImGui::GetStyle();
    float line_height = ImGui::GetTextLineHeight();
    ImVec2 p = ImGui::GetCursorScreenPos();
    if(width <= diameter) {
        width = diameter;
    }
    auto labelSize = ImGui::CalcTextSize(label, label + std::strlen(label), true);
    auto valueSize = ImVec2{0,0};
    if(value_text) {
        valueSize = ImGui::CalcTextSize(value_text, value_text + std::strlen(value_text), true);
    }
    float sz = diameter;
    float radio =  sz*0.5f;
    ImVec2 center = ImVec2(p.x + std::max(width/2, labelSize.x/2), p.y + radio + line_height);
    float val1 = float(value_p[0] - minv)/(maxv - minv);
    //char textval[32];
    //ImFormatString(textval, IM_ARRAYSIZE(textval), "%04.1f", value_p[0]);

    ImVec2 textpos = p;
    float gamma = M_PI/4.0f;//0 value in knob
    float alpha = (M_PI-gamma)*val1*2.0f+gamma;

    float x1 = -sinf(alpha)*radio*0.2f + center.x;
    float y1 = cosf(alpha)*radio*0.2f + center.y;
    float x2 = -sinf(alpha)*radio*1.03f + center.x;
    float y2 = cosf(alpha)*radio*1.03f + center.y;

    ImGui::InvisibleButton(label,ImVec2(width, sz + line_height + (value_text ? line_height*2/3 : 0)));

    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemHovered();
    bool touched = false;

    if (is_active)
    {
        touched = true;
        ImVec2 mp = ImGui::GetIO().MousePos;
        alpha = std::atan2f(mp.x - center.x, center.y - mp.y) + M_PI;
        alpha = ImMax(gamma, std::min<float>(2.0f*M_PI-gamma,alpha));
        float value = 0.5f*(alpha-gamma)/(M_PI-gamma);
        value_p[0] = value*(maxv - minv) + minv;
        val1 = float(value_p[0] - minv)/(maxv - minv);
        alpha = (M_PI-gamma)*val1*2.0f+gamma;
    }

    ImU32 col32 = ImGui::GetColorU32(is_active ? ImGuiCol_ScrollbarGrabActive : is_hovered ? ImGuiCol_ScrollbarGrabHovered : ImGuiCol_ScrollbarGrab);
    ImU32 col32line = ImGui::GetColorU32(ImVec4(.3,.3,.3,1)); //ImGui::GetColorU32(ImGuiCol_SliderGrabActive);
    ImU32 col32text = ImGui::GetColorU32(ImGuiCol_Text);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    //draw_list->AddRect(p, {p.x+sz, p.y+sz}, ImGui::GetColorU32(ImVec4(0.8,0.8,0.8,1)));
    //draw_list->AddCircleFilled(center, radio * 0.8, col32, 16);
    //draw_list->AddText(textpos, col32text, textval);
    draw_list->AddText(ImVec2(p.x + (width - labelSize.x)/2, p.y), col32text, label, ImGui::FindRenderedTextEnd(label));
    if(value_text) {
        draw_list->AddText(ImVec2(p.x + (width - valueSize.x)/2, p.y + sz + line_height*2/3), col32text, value_text, value_text+std::strlen(value_text));
    }
    draw_list->PathArcTo(center, radio*0.87f, (90+45)*M_PI/180, (M_PI-gamma)*2.0f+gamma + 90*M_PI/180, 36);
    draw_list->PathStroke(ImGui::GetColorU32(ImVec4(0.3,0.3,0.3,1)), false, sz/8.0f);
    if(minv >= 0) {
        draw_list->PathArcTo(center, radio*0.87f, (90+45)*M_PI/180, alpha + 90*M_PI/180, 36);
        draw_list->PathStroke(ImGui::GetColorU32(ImVec4(0,1,1,1)), false, sz/8.0f);
    }
    else {
        if(value_p[0] < 0) {
            draw_list->PathArcTo(center, radio*0.87f, alpha + 90*M_PI/180, (90+45+135)*M_PI/180, 36);
            draw_list->PathStroke(ImGui::GetColorU32(ImVec4(0,1,1,1)), false, sz/8.0f);
        }
        else {
            draw_list->PathArcTo(center, radio * 0.87f, (90+45+135)*M_PI/180, alpha + 90 * M_PI / 180, 36);
            draw_list->PathStroke(ImGui::GetColorU32(ImVec4(0, 1, 1, 1)), false, sz / 8.0f);
        }
    }
    draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), ImGui::GetColorU32(ImVec4(.2,.2,.2,1)), sz/6.0f);
    draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), col32line, sz/8.0f);

    return touched;
}

template bool Knob<float>(const char* label, float* value_p, float minv, float maxv, const char* value_text, float diameter, float width);
template bool Knob<int>(const char* label, int* value_p, int minv, int maxv, const char* value_text, float diameter, float width);
template bool Knob<size_t>(const char* label, size_t* value_p, size_t minv, size_t maxv, const char* value_text, float diameter, float width);
template bool Knob<unsigned>(const char* label, unsigned* value_p, unsigned minv, unsigned maxv, const char* value_text, float diameter, float width);

}
