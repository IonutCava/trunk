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
#ifndef _HARDWARE_VIDEO_NODE_BUFFERED_DATA_H_
#define _HARDWARE_VIDEO_NODE_BUFFERED_DATA_H_

namespace Divide {
    using SamplerAddress = U64;

#pragma pack(push, 1)
    struct NodeData
    {
        mat4<F32> _worldMatrix = MAT4_IDENTITY;

        // [0...2][0...2] = normal matrix
        // [3][0...2]     = bounds center
        // [0][3]         = 4x8U: bone count, lod level, tex op, bump method
        // [1][3]         = 2x16F: BBox HalfExtent (X, Y) 
        // [2][3]         = 2x16F: BBox HalfExtent (Z), BSphere Radius
        // [3][3]         = 2x16F: (Data Flag, reserved)
        mat4<F32> _normalMatrixW = MAT4_IDENTITY;

        // [0][0...3] = base colour
        // [1][0...2] = emissive
        // [1][3]     = parallax factor
        // [2][0]     = 4x8U: occlusion, metallic, roughness, reserved
        // [2][1]     = IBL texture size
        // [2][2]     = 2x16F: textureIDs 0, 1
        // [2][3]     = 2x16F: textureIDs 2, 3
        // [3][0]     = 2x16F: textureIDs 4, 5
        // [3][1]     = 2x16F: textureIDs 6, reserved
        // [3][2]     = reserved
        // [3][3]     = reserved
        mat4<F32> _colourMatrix = MAT4_ZERO;

        // [0...3][0...2] = previous world matrix
        // [0][3]         = 4x8U: animation ticked this frame (for motion blur), occlusion cull, reserved, reserved
        // [1][3]         = reserved
        // [2][3]         = reserved
        // [3][3]         = reserved
        mat4<F32> _prevWorldMatrix = MAT4_ZERO;

        SamplerAddress _texDiffuse0   = 0u;
        SamplerAddress _texOpacityMap = 0u;
        SamplerAddress _texDiffuse1   = 0u;
        SamplerAddress _texOMR        = 0u;
        SamplerAddress _texHeight     = 0u;
        SamplerAddress _texProjected  = 0u;
        SamplerAddress _texNormalMap  = 0u;

        F32 _padding[2] = { 0u, 0u };
    };
;
#pragma pack(pop)

} //namespace Divide

#endif //_HARDWARE_VIDEO_NODE_BUFFERED_DATA_H_