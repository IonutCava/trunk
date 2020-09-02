#include "stdafx.h"

#include "imgui.h"

ImVec2& ImVec2::operator=(const Divide::vec2<Divide::F32>& f) {
    *this = ImVec2(f.x, f.y);
    return *this;
}

ImVec4& ImVec4::operator=(const Divide::vec4<Divide::F32>& f) {
    *this = ImVec4(f.x, f.y, f.z, f.w);
    return *this;
}
