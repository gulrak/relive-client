
#include <imguix/imguix.h>
#include <imgui/imgui_internal.h>
#include <resources/feather_icons.h>

bool igx::ColorTouchButton()
{
    return false;
}

bool igx::VerticalTitledGroup(const char* title, ImVec2 size, ImU32 background_color, ImU32 text_color, bool* open)
{

    ImGuiContext& g = *ImGui::GetCurrentContext();
    const ImGuiStyle& style = g.Style;
    auto avail = ImGui::GetContentRegionAvail();
    if(size.x == 0) {
        size.x = avail.x;
    }
    if(size.y == 0) {
        size.y = avail.y - style.ItemSpacing.y;
    }
    ImVec2 p = ImGui::GetCursorScreenPos();
    float pad = style.FramePadding.x;
    ImVec4 color;
    ImVec2 title_size = ImGui::CalcTextSize(title);
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 pos = window->DC.CursorPos + ImVec2(pad, title_size.x + pad);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float lineHeight = ImGui::GetTextLineHeight();
    if(open && !*open) {
        size.x = lineHeight + 8;
    }
    else {
        ImGui::BeginGroup();
        ImGui::Indent(lineHeight + 8);
        ImGui::Spacing();
    }
    auto contentPos = ImVec2(p.x + lineHeight + 8, p.y+2);
    auto contentSize = ImVec2(size.x - contentPos.x - 2, size.y - 4);
    drawList->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), background_color ? background_color : IM_COL32(80,80,80,255), 8, ImDrawCornerFlags_All);
    drawList->AddText(ImVec2(p.x+4, p.y+4), text_color ? text_color : IM_COL32(255,255,255,255), ICON_FTH_POWER);
    AddTextVertical(drawList, title, ImVec2(p.x+3, p.y + size.y - (size.y - title_size.x)/2), text_color ? text_color : IM_COL32(255,255,255,255));
    return !open || *open;
}

void igx::BeginColoredBGChild(const char* id, ImVec2 size, ImU32 col)
{
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), col ? col : IM_COL32(40,40,40,255));
    //ImGui::SetNext
    ImGui::BeginChild(id, size);
    ImGui::Indent(4);
}

void igx::AddTextVertical(ImDrawList* DrawList, const char *text, ImVec2 pos, ImU32 text_color) {
    pos.x = IM_ROUND(pos.x);
    pos.y = IM_ROUND(pos.y);
    ImFont *font = GImGui->Font;
    const ImFontGlyph *glyph;
    char c;
    ImGuiContext& g = *GImGui;
    ImVec2 text_size = ImGui::CalcTextSize(text);
    while ((c = *text++)) {
        glyph = font->FindGlyph(c);
        if (!glyph) continue;

        DrawList->PrimReserve(6, 4);
        DrawList->PrimQuadUV(
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
}