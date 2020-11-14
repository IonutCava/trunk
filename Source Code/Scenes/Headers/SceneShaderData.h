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

#include "Scenes/Headers/SceneState.h"
#include "Utility/Headers/Colours.h"

namespace Divide {

class GFXDevice;
class ShaderBuffer;

constexpr U8 GLOBAL_WATER_BODIES = 2;

class SceneShaderData {
    struct SceneShaderBufferData {
        // w - reserved
        vec4<F32> _sunDirection = { 0.0f };
        // w - reserved
        FColour4 _sunColour = DefaultColours::WHITE;
        // w - reserved
        FColour4 _zenithColour = { 0.025f, 0.1f, 0.5f, 1.0f};
        // w - reserved
        FColour4 _horizonColour = { 0.6f, 0.7f, 1.0f, 0.0f};
        // x,y,z - colour, w - density
        vec4<F32> _fogDetails = { 0.0f};
        // x,y,z - direction, w - speed
        vec4<F32> _windDetails = {0.0f};
        //x - light bleed bias, y - min shadow variance, z - fade distance, w - max distance
        vec4<F32> _shadowingSettings = {0.2f, 0.001f, 1000.0f, 1000.0f};
        //x - elapsed time, y - delta time, z,w - reserved
        vec4<F32> _otherData = {0.0f};
        WaterBodyData _waterEntities[GLOBAL_WATER_BODIES] = {};

        //RenderDoc: vec4 fogDetails; vec4 windDetails; vec4 shadowSettings; vec4 otherData;
    };

  public:
    explicit SceneShaderData(GFXDevice& context);
    ~SceneShaderData() = default;

    [[nodiscard]] ShaderBuffer* buffer() const noexcept {
        return _sceneShaderData;
    }

    void sunDetails(const vec3<F32>& direction, const FColour3& colour) noexcept {
        _bufferData._sunDirection.set(direction);
        _bufferData._sunColour.set(colour);
        _dirty = true;
    }

    void skyColour(const FColour4& horizon, const FColour4& zenith) noexcept {
        _bufferData._horizonColour = horizon;
        _bufferData._zenithColour = zenith;
        _dirty = true;
    }

    void fogDetails(const F32 colourR, const F32 colourG, const F32 colourB, const F32 density) noexcept {
        _bufferData._fogDetails.set(colourR, colourG, colourB, density);
        _dirty = true;
    }

    void fogDensity(F32 density) noexcept {
        CLAMP_01(density);
        _bufferData._fogDetails.w = density;
        _dirty = true;
    }

    void shadowingSettings(const F32 lightBleedBias, const F32 minShadowVariance, const F32 shadowFadeDist, const F32 shadowMaxDist) noexcept {
        _bufferData._shadowingSettings.set(lightBleedBias, minShadowVariance, shadowFadeDist, shadowMaxDist);
        _dirty = true;
    }

    void windDetails(const F32 directionX, const F32 directionY, const F32 directionZ, const F32 speed) noexcept {
        _bufferData._windDetails.set(directionX, directionY, directionZ, speed);
        _dirty = true;
    }

    void elapsedTime(const U32 timeMS) noexcept {
        _bufferData._otherData.x = to_F32(timeMS);
        _dirty = true;
    }

    void deltaTime(const F32 deltaTimeMS) noexcept {
        _bufferData._otherData.y = deltaTimeMS;
        _dirty = true;
    }

    bool waterDetails(const U8 index, const WaterBodyData& data) {
        if (index < GLOBAL_WATER_BODIES) {
            _bufferData._waterEntities[index] = data;
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
} //namespace Divide

#endif //_SCENE_SHADER_DATA_H_
