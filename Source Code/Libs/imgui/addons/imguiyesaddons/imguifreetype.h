#ifndef IMGUIFREETYPE_H_
#define IMGUIFREETYPE_H_

// Original repository: https://github.com/Vuhdo/imgui_freetype (MIT licensed)
// (Kept up to date in: https://github.com/ocornut/imgui_club)

// USAGE:
/*
	Manually: 	use ImGuiFreeType::GetTexDataAsRGBA32(...) in your binding code.
	Automatic:	just set ImGuiFreeType::DefaultRasterizationFlags (or DefaultRasterizationFlagVector) at the start of the "main" method.
*/

#ifndef IMGUI_API
#	include <imgui.h>
#endif //IMGUI_API

#ifndef IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
#	define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION	// so that in imgui_draw.cpp the implementation of "stb_rect_pack.h" is NOT included
#endif //IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION


namespace ImGuiFreeType {
    //  Hinting greatly impacts visuals (and glyph sizes).
    //  When disabled, FreeType generates blurrier glyphs, more or less matches the stb's output.
    //  The Default hinting mode usually looks good, but may distort glyphs in an unusual way.
    //  The Light hinting mode generates fuzzier glyphs but better matches Microsoft's rasterizer.

    // You can set those flags on a per font basis in ImFontConfig::RasterizerFlags.
    // Use the 'extra_flags' parameter of BuildFontAtlas() to force a flag on all your fonts.
    enum RasterizerFlags
    {
        // By default, hinting is enabled and the font's native hinter is preferred over the auto-hinter.
        NoHinting       = 1 << 0,   // Disable hinting. This generally generates 'blurrier' bitmap glyphs when the glyph are rendered in any of the anti-aliased modes.
        NoAutoHint      = 1 << 1,   // Disable auto-hinter.
        ForceAutoHint   = 1 << 2,   // Indicates that the auto-hinter is preferred over the font's native hinter.
        LightHinting    = 1 << 3,   // A lighter hinting algorithm for gray-level modes. Many generated glyphs are fuzzier but better resemble their original shape. This is achieved by snapping glyphs to the pixel grid only vertically (Y-axis), as is done by Microsoft's ClearType and Adobe's proprietary font renderer. This preserves inter-glyph spacing in horizontal text.
        MonoHinting     = 1 << 4,   // Strong hinting algorithm that should only be used for monochrome output.
        Bold            = 1 << 5,   // Styling: Should we artificially embolden the font?
        Oblique         = 1 << 6,   // Styling: Should we slant the font, emulating italic style?
    };


IMGUI_API void GetTexDataAsAlpha8(ImFontAtlas* atlas,unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel,ImU32 flags=0,const ImVector<ImU32>* pOptionalFlagVector=NULL);
IMGUI_API void GetTexDataAsRGBA32(ImFontAtlas* atlas,unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel,ImU32 flags=0,const ImVector<ImU32>* pOptionalFlagVector=NULL);

#ifdef IMGUI_USE_AUTO_BINDING
extern ImU32 DefaultRasterizationFlags;			// = 0 by default. Tweakable
extern ImVector<ImU32> DefaultRasterizationFlagVector;	// if set, it overrides DefaultRasterizationFlags. One entry per font.
#endif //IMGUI_USE_AUTO_BINDING

} // namespace ImGuiFreeType


#endif //IMGUIFREETYPE_H_

