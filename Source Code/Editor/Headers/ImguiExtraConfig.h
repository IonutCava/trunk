
//-----------------------------------------------------------------------------
// USER IMPLEMENTATION
// This file contains compile-time options for ImGui.
// Other options (memory allocation overrides, callbacks, etc.) can be set at runtime via the ImGuiIO structure - ImGui::GetIO().
//-----------------------------------------------------------------------------

#pragma once
#include "Core/Math/Headers/MathVectors.h"

ImVec2 operator=(const Divide::vec2<Divide::F32>& f) {
    ImVec2 ret;
    ret.x = f.x;
    ret.y = f.y;
    return ret;
}

Divide::vec2<Divide::F32> operator=(const ImVec2& f) {
    return Divide::vec2<Divide::F32>(f.x, f.y);
}

ImVec4 operator=(const Divide::vec4<Divide::F32>& f) {
    ImVec4 ret;
    ret.x = f.x;
    ret.y = f.y;
    ret.z = f.z;
    ret.w = f.w;
    return ret;
}

Divide::vec4<Divide::F32> operator=(const ImVec4& f) {
    return Divide::vec4<Divide::F32>(f.x, f.y, f.z, f.w);
}