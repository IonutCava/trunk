/*
   Copyright (c) 2020 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#pragma once
#ifndef _MATERIAL_ENUMS_H_
#define _MATERIAL_ENUMS_H_

#include "Platform/Headers/PlatformDataTypes.h"

namespace Divide {
    enum class BumpMethod : U8 {
        NONE = 0,
        NORMAL = 1,
        PARALLAX = 2,
        PARALLAX_OCCLUSION = 3,
        COUNT
    };
    namespace Names {
        static const char* bumpMethod[] = {
            "NONE", "NORMAL", "PARALLAX", "PARALLAX_OCCLUSION", "UNKNOWN"
        };
    };

    /// How should each texture be added
    enum class TextureOperation : U8 {
        NONE = 0,
        MULTIPLY = 1,
        ADD = 2,
        SUBTRACT = 3,
        DIVIDE = 4,
        SMOOTH_ADD = 5,
        SIGNED_ADD = 6,
        DECAL = 7,
        REPLACE = 8,
        COUNT
    };
    namespace Names {
        static const char* textureOperation[] = {
            "NONE", "MULTIPLY", "ADD", "SUBTRACT", "DIVIDE", "SMOOTH_ADD", "SIGNED_ADD", "DECAL", "REPLACE", "UNKNOW",
        };
    };

    enum class TranslucencySource : U8 {
        ALBEDO,
        OPACITY_MAP_R, //single channel
        OPACITY_MAP_A, //rgba texture
        COUNT
    };

    /// Not used yet but implemented for shading model selection in shaders
    /// This enum matches the ASSIMP one on a 1-to-1 basis
    enum class ShadingMode : U8 {
        FLAT = 0,
        PHONG,
        BLINN_PHONG,
        TOON,
        // Use PBR for the following
        OREN_NAYAR,
        COOK_TORRANCE,
        COUNT
    };
    namespace Names {
        static const char* shadingMode[] = {
            "FLAT", "PHONG", "BLINN_PHONG", "TOON", "OREN_NAYAR", "COOK_TORRANCE", "NONE"
        };
    };

}; //namespace Divide

#endif //_MATERIAL_ENUMS_H_