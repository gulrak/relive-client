#pragma once

#include <imgui/imgui.h>

#include <list>

namespace igx {

struct GradientMark
{
    float color[4];
    float position; //0 to 1
};

class Gradient
{
public:
    Gradient();
    ~Gradient();

    void getColorAt(float position, ImVec4& color) const;
    void addMark(float position, ImColor color);
    void removeMark(GradientMark* mark);
    void refreshCache();
    std::list<GradientMark*> & getMarks(){ return _marks; }

    void drawHorizontal(struct ImVec2 const & bar_pos, float width, float height);
    void drawVertical(struct ImVec2 const & bar_pos, float width, float height);
    void drawInterval(const struct ImVec2 &bar_pos, float width, float height, float start, float end, bool horizontal = true);
private:
    void computeColorAt(float position, float* color) const;
    void draw(struct ImVec2 const & bar_pos, float width, float height, bool horizontal);
    std::list<GradientMark*> _marks;
    float _cachedValues[256 * 3];
};

}
