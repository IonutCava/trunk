#include "stdafx.h"

#include "imgui.h"
#include "Core/Math/Headers/MathVectors.h"

ImVec2 ImVec2::operator=(const Divide::vec2<Divide::F32>& f) {
    return ImVec2(f.x, f.y);
}

ImVec4 ImVec4::operator=(const Divide::vec4<Divide::F32>& f) {
    return ImVec4(f.x, f.y, f.z, f.w);
}
