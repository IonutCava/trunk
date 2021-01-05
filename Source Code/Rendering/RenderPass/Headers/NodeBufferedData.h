/*
   Copyright (c) 2018 DIVIDE-Studio
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
#ifndef _RENDER_PASS_NODE_BUFFERED_DATA_H_
#define _RENDER_PASS_NODE_BUFFERED_DATA_H_
#include "Platform/Video/Headers/RenderAPIEnums.h"

namespace Divide {
enum class TextureUsage : unsigned char;

constexpr TextureUsage g_materialTextures[] = {
    TextureUsage::UNIT0,
    TextureUsage::OPACITY,
    TextureUsage::UNIT1,
    TextureUsage::OCCLUSION_METALLIC_ROUGHNESS,
    TextureUsage::HEIGHTMAP,
    TextureUsage::PROJECTION,
    TextureUsage::NORMALMAP
};

constexpr size_t MATERIAL_TEXTURE_COUNT = (sizeof(g_materialTextures) / sizeof(g_materialTextures[0]));
using NodeMaterialTextures = std::array<SamplerAddress, MATERIAL_TEXTURE_COUNT>;

#pragma pack(push, 1)
    struct NodeTransformData
    {
        mat4<F32> _worldMatrix = MAT4_IDENTITY;

        // [0...2][0...2] = normal matrix
        // [3][0...2]     = bounds center
        // [0][3]         = 4x8U: bone count, lod level, isHovered, reserved
        // [1][3]         = 2x16F: BBox HalfExtent (X, Y) 
        // [2][3]         = 2x16F: BBox HalfExtent (Z), BSphere Radius
        // [3][3]         = 2x16F: (Data Flag, reserved)
        mat4<F32> _normalMatrixW = MAT4_IDENTITY;

        // [0...3][0...2] = previous world matrix
        // [0][3]         = 4x8U: animation ticked this frame (for motion blur), occlusion cull, reserved, reserved
        // [1][3]         = reserved
        // [2][3]         = reserved
        // [3][3]         = reserved
        mat4<F32> _prevWorldMatrix = MAT4_ZERO;
    };

    struct NodeMaterialData
    {
        //base colour
        vec4<F32> _albedo;
        //rgb - emissive, 
        //a   - parallax factor
        vec4<F32> _emissiveAndParallax;
        //x = 4x8U: occlusion, metallic, roughness, reserved
        //y = IBL texture size
        //z = 4x8U: tex op, bump method, reserved, reserved
        //w = Probe lookup index + 1 (0 = sky cubemap)
        vec4<U32> _data;

        vec2<U64> _textures[(MATERIAL_TEXTURE_COUNT + 1) / 2];
    };

    [[nodiscard]] size_t HashMaterialData(const NodeMaterialData& dataIn) noexcept;
    [[nodiscard]] size_t HashTexturesData(const NodeMaterialTextures& dataIn) noexcept;
#pragma pack(pop)

} //namespace Divide

#endif //_RENDER_PASS_NODE_BUFFERED_DATA_H_