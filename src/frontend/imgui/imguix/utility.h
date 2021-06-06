#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <cmath>

namespace igx {

inline float distancePtLine(const ImVec2& a, const ImVec2& b, const ImVec2& p)
{
    ImVec2 n = b - a;
    ImVec2 pa = a - p;
    ImVec2 c = n * (ImDot(pa, n) / ImDot(n, n));
    ImVec2 d = pa - c;
    return std::sqrt(ImDot(d, d));
}

inline float distance(const ImVec2& a, const ImVec2& b)
{
    auto xd = b.x - a.x;
    auto yd = b.y - a.y;
    return std::sqrt(xd * xd + yd * yd);
}

inline float quantize(float value, int steps)
{
    if(steps <= 0) {
        return value;
    }
    return float(int(value * steps)) / steps;
}

}
