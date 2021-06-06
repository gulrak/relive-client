//
// Created by Steffen Schümann on 21.04.20.
//

#include "gradient.h"
#include <imgui/imgui_internal.h>

namespace igx {

Gradient::Gradient()
{
    addMark(0.0f, ImColor(0.0f,0.0f,0.0f));
    addMark(1.0f, ImColor(1.0f,1.0f,1.0f));
}

Gradient::~Gradient()
{
    for (GradientMark* mark : _marks)
    {
        delete mark;
    }
}

void Gradient::addMark(float position, ImColor color)
{
    position = ImClamp(position, 0.0f, 1.0f);
    auto* newMark = new GradientMark();
    newMark->position = position;
    newMark->color[0] = color.Value.x;
    newMark->color[1] = color.Value.y;
    newMark->color[2] = color.Value.z;

    _marks.push_back(newMark);
    refreshCache();
}

void Gradient::removeMark(GradientMark* mark)
{
    _marks.remove(mark);
    refreshCache();
}

void Gradient::getColorAt(float position, ImVec4& color) const
{
    position = ImClamp(position, 0.0f, 1.0f);
    int cachePos = int(position * 255);
    cachePos *= 3;
    color.x = _cachedValues[cachePos+0];
    color.y = _cachedValues[cachePos+1];
    color.z = _cachedValues[cachePos+2];
    color.w = 1;
}

void Gradient::computeColorAt(float position, float* color) const
{
    position = ImClamp(position, 0.0f, 1.0f);

    GradientMark* lower = nullptr;
    GradientMark* upper = nullptr;

    for(GradientMark* mark : _marks)
    {
        if(mark->position < position)
        {
            if(!lower || lower->position < mark->position)
            {
                lower = mark;
            }
        }

        if(mark->position >= position)
        {
            if(!upper || upper->position > mark->position)
            {
                upper = mark;
            }
        }
    }

    if(upper && !lower)
    {
        lower = upper;
    }
    else if(!upper && lower)
    {
        upper = lower;
    }
    else if(!lower && !upper)
    {
        color[0] = color[1] = color[2] = 0;
        return;
    }

    if(upper == lower)
    {
        color[0] = upper->color[0];
        color[1] = upper->color[1];
        color[2] = upper->color[2];
    }
    else
    {
        float distance = upper->position - lower->position;
        float delta = (position - lower->position) / distance;

        //lerp
        color[0] = ((1.0f - delta) * lower->color[0]) + ((delta) * upper->color[0]);
        color[1] = ((1.0f - delta) * lower->color[1]) + ((delta) * upper->color[1]);
        color[2] = ((1.0f - delta) * lower->color[2]) + ((delta) * upper->color[2]);
    }
}

void Gradient::refreshCache()
{
    _marks.sort([](const GradientMark * a, const GradientMark * b) { return a->position < b->position; });

    for(int i = 0; i < 256; ++i)
    {
        computeColorAt(i/255.0f, &_cachedValues[i*3]);
    }
}

void Gradient::drawHorizontal(const struct ImVec2 &bar_pos, float width, float height)
{
    draw(bar_pos, width, height, true);
}

void Gradient::drawVertical(const struct ImVec2 &bar_pos, float width, float height)
{
    draw(bar_pos, width, height, false);
}

void Gradient::draw(const struct ImVec2 &bar_pos, float width, float height, bool horizontal)
{
    ImVec4 colorA = {1,1,1,1};
    ImVec4 colorB = {1,1,1,1};
    float prev = horizontal ? bar_pos.x : bar_pos.y;
    float barEnd = horizontal ? bar_pos.y + height : bar_pos.x + width;
    const GradientMark* prevMark = nullptr;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->AddRectFilled(ImVec2(bar_pos.x - 2, bar_pos.y - 2),
                             ImVec2(bar_pos.x + width + 2, bar_pos.y + height + 2),
                             IM_COL32(100, 100, 100, 255));

    if(getMarks().empty())
    {
        draw_list->AddRectFilled(ImVec2(bar_pos.x, bar_pos.y),
                                 ImVec2(bar_pos.x + width, bar_pos.y + height),
                                 IM_COL32(255, 255, 255, 255));

    }

    ImU32 colorAU32 = 0;
    ImU32 colorBU32 = 0;

    for(const auto& mark : getMarks())
    {
        float from = prev;
        float to = prev = horizontal ? bar_pos.x + mark->position * width : bar_pos.y + mark->position * height;

        if(prevMark == nullptr)
        {
            colorA.x = mark->color[0];
            colorA.y = mark->color[1];
            colorA.z = mark->color[2];
        }
        else
        {
            colorA.x = prevMark->color[0];
            colorA.y = prevMark->color[1];
            colorA.z = prevMark->color[2];
        }

        colorB.x = mark->color[0];
        colorB.y = mark->color[1];
        colorB.z = mark->color[2];

        colorAU32 = ImGui::ColorConvertFloat4ToU32(colorA);
        colorBU32 = ImGui::ColorConvertFloat4ToU32(colorB);

        if(mark->position > 0.0)
        {
            if(horizontal) {
                draw_list->AddRectFilledMultiColor(ImVec2(from, bar_pos.y), ImVec2(to, barEnd), colorAU32, colorBU32, colorBU32, colorAU32);
            }
            else {
                draw_list->AddRectFilledMultiColor(ImVec2(bar_pos.x, from), ImVec2(barEnd, to), colorAU32, colorAU32, colorBU32, colorBU32);
            }
        }

        prevMark = mark;
    }

    if(prevMark && prevMark->position < 1.0)
    {
        if(horizontal) {
            draw_list->AddRectFilledMultiColor(ImVec2(prev, bar_pos.y), ImVec2(bar_pos.x + width, barEnd), colorBU32, colorBU32, colorBU32, colorBU32);
        }
        else {
            draw_list->AddRectFilledMultiColor(ImVec2(bar_pos.x, prev), ImVec2(barEnd, bar_pos.y + height), colorAU32, colorAU32, colorBU32, colorBU32);
        }
    }

    ImGui::SetCursorScreenPos(ImVec2(bar_pos.x, bar_pos.y + height + 10.0f));
}

void Gradient::drawInterval(const struct ImVec2 &bar_pos, float width, float height, float start, float end, bool horizontal)
{
    ImVec4 colorA = {1,1,1,1};
    ImVec4 colorB = {1,1,1,1};
    float range = end - start;
    float prev = horizontal ? bar_pos.x : bar_pos.y;
    float barEnd = horizontal ? bar_pos.y + height : bar_pos.x + width;
    const GradientMark* prevMark = nullptr;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImU32 colorAU32 = 0;
    ImU32 colorBU32 = 0;

    for(const auto& mark : getMarks())
    {
        if(mark->position > start && mark->position < end) {
            float from = prev;
            float to = prev = horizontal ? bar_pos.x + (mark->position-start)/range * width : bar_pos.y + (mark->position-start)/range * height;

            if (prevMark == nullptr) {
                getColorAt(start, colorA);
            }
            else {
                colorA = colorB;
            }

            colorB.x = mark->color[0];
            colorB.y = mark->color[1];
            colorB.z = mark->color[2];

            colorAU32 = ImGui::ColorConvertFloat4ToU32(colorA);
            colorBU32 = ImGui::ColorConvertFloat4ToU32(colorB);

            if (mark->position > 0.0) {
                if (horizontal) {
                    draw_list->AddRectFilledMultiColor(ImVec2(from, bar_pos.y), ImVec2(to, barEnd), colorAU32, colorBU32, colorBU32, colorAU32);
                }
                else {
                    draw_list->AddRectFilledMultiColor(ImVec2(bar_pos.x, from), ImVec2(barEnd, to), colorAU32, colorAU32, colorBU32, colorBU32);
                }
            }

            prevMark = mark;
        }
    }

    if (prevMark && prevMark->position < end) {
        colorAU32 = ImGui::ColorConvertFloat4ToU32(colorB);
    }
    else if (!prevMark) {
        getColorAt(start, colorA);
        colorAU32 = ImGui::ColorConvertFloat4ToU32(colorA);
    }
    getColorAt(end, colorB);
    colorBU32 = ImGui::ColorConvertFloat4ToU32(colorB);
    if (horizontal) {
        draw_list->AddRectFilledMultiColor(ImVec2(prev, bar_pos.y), ImVec2(bar_pos.x + width, barEnd), colorAU32, colorBU32, colorBU32, colorAU32);
    }
    else {
        draw_list->AddRectFilledMultiColor(ImVec2(bar_pos.x, prev), ImVec2(barEnd, bar_pos.y + height), colorAU32, colorAU32, colorBU32, colorBU32);
    }
}

}