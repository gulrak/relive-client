#pragma once

#include <imgui/imgui.h>

namespace igx
{

extern bool ColorTouchButton();
extern bool VerticalTitledGroup(const char* title, ImVec2 size = {0,0}, ImU32 background_color = 0, ImU32 text_color = 0, bool* open = nullptr);
extern void BeginColoredBGChild(const char* id, ImVec2 size, ImU32 col = 0);
// Drawing Functions

extern void AddTextVertical(ImDrawList* DrawList, const char *text, ImVec2 pos, ImU32 text_color);

}
