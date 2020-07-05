#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace igx {

inline bool VerticalTab(const char *text, bool *v)
{
    ImFont *font = GImGui->Font;
    const ImFontGlyph *glyph;
    char c;
    bool ret;
    ImGuiContext& g = *ImGui::GetCurrentContext();
    const ImGuiStyle& style = g.Style;
    float pad = style.FramePadding.x;
    ImVec4 color;
    ImVec2 text_size = ImGui::CalcTextSize(text);
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 pos = window->DC.CursorPos + ImVec2(pad, text_size.x + pad);

    const  ImU32 text_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
    color = style.Colors[ImGuiCol_Button];
    if (*v) color = style.Colors[ImGuiCol_ButtonActive];
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushID(text);
    ret = ImGui::Button("", ImVec2(text_size.y + pad * 2,
                                   text_size.x + pad * 2));
    ImGui::PopStyleColor();
    while ((c = *text++)) {
        glyph = font->FindGlyph(c);
        if (!glyph) continue;

        window->DrawList->PrimReserve(6, 4);
        window->DrawList->PrimQuadUV(
            pos + ImVec2(glyph->Y0, -glyph->X0),
            pos + ImVec2(glyph->Y0, -glyph->X1),
            pos + ImVec2(glyph->Y1, -glyph->X1),
            pos + ImVec2(glyph->Y1, -glyph->X0),

            ImVec2(glyph->U0, glyph->V0),
            ImVec2(glyph->U1, glyph->V0),
            ImVec2(glyph->U1, glyph->V1),
            ImVec2(glyph->U0, glyph->V1),
            text_color);
        pos.y -= glyph->AdvanceX;
    }
    ImGui::PopID();
    return ret;
}

}
