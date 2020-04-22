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
#ifndef _SCENE_SHADER_DATA_H_
#define _SCENE_SHADER_DATA_H_

#include "Platform/Video/Headers/RenderAPIEnums.h"

namespace Divide {

class GFXDevice;
class ShaderBuffer;

constexpr U8 MAX_WATER_BODIES = 6;

class SceneShaderData {
  private:
    struct WaterBodyData {
        vec4<F32> _positionW = {0.0f};
        vec4<F32> _details = {0.0f};
    };

    struct SceneShaderBufferData {
        // x,y,z - colour, w - density
        vec4<F32> _fogDetails = { 0.0f};
        // x,y,z - direction, w - speed
        vec4<F32> _windDetails = {0.0f};
        //x - light bleed bias, y - min shadow variance, z - fade distance, w - max distance
        vec4<F32> _shadowingSettings = {0.2f, 0.001f, 1000.0f, 1000.0f};
        //x - elapsed time, y - delta time, z - debug render, w.x - detail level
        vec4<F32> _otherData = {0.0f};
        WaterBodyData _waterEntities[MAX_WATER_BODIES] = {};

        //RenderDoc: vec4 fogDetails; vec4 windDetails; vec4 shadowSettings; vec4 otherData;
    };

  public:
    SceneShaderData(GFXDevice& context);
    ~SceneShaderData();

    inline ShaderBuffer* buffer() const noexcept {
        return _sceneShaderData;
    }

    inline void fogDetails(F32 colourR, F32 colourG, F32 colourB, F32 density) noexcept {
        _bufferData._fogDetails.set(colourR, colourG, colourB, density);
        _dirty = true;
    }

    inline void fogDensity(F32 density) noexcept {
        CLAMP_01(density);
        _bufferData._fogDetails.w = density;
        _dirty = true;
    }

    inline void shadowingSettings(F32 lightBleedBias, F32 minShadowVariance, F32 shadowFadeDist, F32 shadowMaxDist) noexcept {
        _bufferData._shadowingSettings.set(lightBleedBias, minShadowVariance, shadowFadeDist, shadowMaxDist);
        _dirty = true;
    }

    inline void windDetails(F32 directionX, F32 directionY, F32 directionZ, F32 speed) noexcept {
        _bufferData._windDetails.set(directionX, directionY, directionZ, speed);
        _dirty = true;
    }

    inline void elapsedTime(U32 timeMS) noexcept {
        _bufferData._otherData.x = to_F32(timeMS);
        _dirty = true;
    }

    inline void deltaTime(F32 deltaTimeMS) noexcept {
        _bufferData._otherData.y = deltaTimeMS;
        _dirty = true;
    }

    inline void enableDebugRender(bool state) noexcept {
        _bufferData._otherData.z = state ? 1.0f : 0.0f;
        _dirty = true;
    }

    inline bool waterDetails(U8 index, const vec3<F32>& positionW, const vec3<F32>& dimensions) {
        if (index < MAX_WATER_BODIES) {
            WaterBodyData& waterBody = _bufferData._waterEntities[index];
            waterBody._positionW.set(positionW);
            waterBody._details.set(dimensions);
            _dirty = true;

            return true;
        }

        return false;
    }

    ShaderBuffer* uploadToGPU();

  private:
      GFXDevice& _context;
      bool _dirty;
      SceneShaderBufferData _bufferData;
      /// Generic scene data that doesn't change per shader
      ShaderBuffer* _sceneShaderData;
};
}; //namespace Divide

#endif //_SCENE_SHADER_DATA_H_
