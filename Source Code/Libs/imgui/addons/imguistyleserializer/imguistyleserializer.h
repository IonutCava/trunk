#ifndef IMGUISTYLESERIALIZER_H_
#define IMGUISTYLESERIALIZER_H_

#ifndef IMGUI_API
#include <imgui.h>
#endif //IMGUI_API

enum ImGuiStyleEnum {
    ImGuiStyle_DefaultClassic=0,
    ImGuiStyle_DefaultDark,
    ImGuiStyle_DefaultLight,

    ImGuiStyle_Gray,        // (mine) This is the default theme of my main.cpp demo.
    ImGuiStyle_Light,       // (mine)
    ImGuiStyle_BlackCodz01, // Posted by @codz01 here: https://github.com/ocornut/imgui/issues/707 (hope I can use it)
    ImGuiStyle_DarkCodz01,  // Posted by @codz01 here: https://github.com/ocornut/imgui/issues/707 (hope I can use it)
    ImGuiStyle_GrayCodz01,  // Posted by @codz01 here: https://github.com/ocornut/imgui/issues/1607 (hope I can use it)
    ImGuiStyle_Purple,      // Posted by @fallrisk here: https://github.com/ocornut/imgui/issues/1607  (hope I can use it)
    ImGuiStyle_Cherry,      // Posted by @r-lyeh here: https://github.com/ocornut/imgui/issues/707 (hope I can use it)
    ImGuiStyle_DarkOpaque,  // (mine)
    ImGuiStyle_Soft,        // Posted by @olekristensen here: https://github.com/ocornut/imgui/issues/539 (hope I can use it)
    ImGuiStyle_EdinBlack,   // Posted (via image) by edin_p in the screenshot section of Dear ImGui
    ImGuiStyle_EdinWhite,   // Posted (via image) by edin_p in the screenshot section of Dear ImGui
    ImGuiStyle_Maya,        // Posted by @ongamex here https://gist.github.com/ongamex/4ee36fb23d6c527939d0f4ba72144d29
    ImGuiStyle_LightGreen,  // Posted by @ebachard here: https://github.com/ocornut/imgui/pull/1776 (hope I can use it)
    ImGuiStyle_Design,      // Posted by @usernameiwantedwasalreadytaken here: https://github.com/ocornut/imgui/issues/707 (hope I can use it)
    ImGuiStyle_Dracula,     // Posted by @ice1000 here: https://github.com/ocornut/imgui/issues/707 (hope I can use it)
    ImGuiStyle_Greenish,    // Posted by @dertseha here: https://github.com/ocornut/imgui/issues/1902 (Hope I can use it)

    ImGuiStyle_DarkOpaqueInverse,
    ImGuiStyle_GrayCodz01Inverse,
    ImGuiStyle_PurpleInverse,
    ImGuiStyle_LightGreenInverse,
    ImGuiStyle_DesignInverse,

    ImGuiStyle_Count
};

namespace ImGui	{
// Warning: this file does not depend on imguihelper (so it's easier to reuse it in stand alone projects).
// The drawback is that it's not possible to serialize/deserialize a style together with other stuff (for example 2 styles together) into/from a single file.
// And it's not possible to serialize/deserialize a style into/from a memory buffer too.
#ifndef NO_IMGUISTYLESERIALIZER_SAVESTYLE
IMGUI_API bool SaveStyle(const char* filename,const ImGuiStyle& style=ImGui::GetStyle());
#endif //NO_IMGUISTYLESERIALIZER_SAVESTYLE
#ifndef NO_IMGUISTYLESERIALIZER_LOADSTYLE
IMGUI_API bool LoadStyle(const char* filename,ImGuiStyle& style=ImGui::GetStyle());
#endif //NO_IMGUISTYLESERIALIZER_LOADSTYLE
IMGUI_API bool ResetStyle(int styleEnum, ImGuiStyle& style=ImGui::GetStyle());
IMGUI_API const char** GetDefaultStyleNames();   // ImGuiStyle_Count names re returned

// satThresholdForInvertingLuminance: in [0,1] if == 0.f luminance is not inverted at all
// shiftHue: in [0,1] if == 0.f hue is not changed at all
IMGUI_API void ChangeStyleColors(ImGuiStyle& style,float satThresholdForInvertingLuminance=.1f,float shiftHue=0.f);
} // namespace ImGui

#endif //IMGUISTYLESERIALIZER_H_

