#include "stdafx.h"
#include "Core/Math/Headers/MathVectors.h"

ImVec2 ImVec2::operator=(const Divide::vec2<Divide::F32>& f) {
    ImVec2 ret;
    ret.x = f.x;
    ret.y = f.y;
    return ret;
}

ImVec4 ImVec4::operator=(const Divide::vec4<Divide::F32>& f) {
    ImVec4 ret;
    ret.x = f.x;
    ret.y = f.y;
    ret.z = f.z;
    ret.w = f.w;
    return ret;
}
