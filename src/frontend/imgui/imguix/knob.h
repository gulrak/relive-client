#pragma once

namespace igx {

template <typename T>
bool Knob(const char* label, T* value_p, T minv, T maxv, const char* value_text = nullptr, float diameter = 64.0f, float width = 0);

}
