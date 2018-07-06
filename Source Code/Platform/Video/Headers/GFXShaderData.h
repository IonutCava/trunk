/*
Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _HARDWARE_VIDEO_GFX_SHADER_DATA_H_
#define _HARDWARE_VIDEO_GFX_SHADER_DATA_H_

#include "Rendering/Camera/Headers/Frustum.h"

namespace Divide {
class GFXShaderData {
  public:
    GFXShaderData();

  public:
    struct GPUData {
        GPUData();

        mat4<F32> _ProjectionMatrix;
        mat4<F32> _InvProjectionMatrix;
        mat4<F32> _ViewMatrix;
        mat4<F32> _ViewProjectionMatrix;
        vec4<F32> _cameraPosition; // xyz - position, w - aspect ratio
        vec4<F32> _ViewPort;
        vec4<F32> _ZPlanesCombined;  // xy - current, zw - main scene
        vec4<F32> _renderProperties;
        vec4<F32> _frustumPlanes[to_const_uint(Frustum::FrustPlane::COUNT)];
        vec4<F32> _clipPlanes[to_const_uint(Frustum::FrustPlane::COUNT)];

        inline F32 aspectRatio() const;
        inline vec2<F32> currentZPlanes() const;
        inline F32 FoV() const;
        inline F32 tanHFoV() const;

        bool operator==(const GPUData& other) const;
        bool operator!=(const GPUData& other) const;

    } _data;

    mat4<F32> _viewMatrixInv;
    mat4<F32> _viewProjMatrixInv;

    bool _needsUpload = true;
};
}; //namespace Divide

#endif //_HARDWARE_VIDEO_GFX_SHADER_DATA_H_

#include "GFXShaderData.inl"