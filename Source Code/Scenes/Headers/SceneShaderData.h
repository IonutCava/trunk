/*
Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _SCENE_SHADER_DATA_H_
#define _SCENE_SHADER_DATA_H_

#include "Platform/Headers/PlatformDefines.h"
#include "Rendering/Lighting/Headers/Light.h"

namespace Divide {
class SceneShaderData {
  private:
    struct SceneShaderBufferData {
        vec4<F32> _fogDetails;
        vec4<F32> _windDetails;
        vec4<F32> _shadowingSettings;
        vec4<F32> _otherData;
        vec4<F32> _otherData2;
        vec4<U32> _lightCountPerType;
        //U32     _lightCountPerType[to_const_uint(LightType::COUNT)];
    };

  public:
    SceneShaderData();
    ~SceneShaderData();

    inline void fogDetails(F32 colourR, F32 colourG, F32 colourB, F32 density) {
        _bufferData._fogDetails.set(colourR, colourG, colourB, density / 1000.0f);
    }

    inline void fogDensity(F32 density) {
        _bufferData._fogDetails.w = density;
    }

    inline void shadowingSettings(F32 lightBleedBias, F32 minShadowVariance, F32 shadowFadeDist, F32 shadowMaxDist) {
        _bufferData._shadowingSettings.set(lightBleedBias, minShadowVariance, shadowFadeDist, shadowMaxDist);
    }

    inline void windDetails(F32 directionX, F32 directionY, F32 directionZ, F32 speed) {
        _bufferData._windDetails.set(directionX, directionY, directionZ, speed);
    }

    inline void elapsedTime(U32 timeMS) {
        _bufferData._otherData.x = to_float(timeMS);
    }

    inline void enableDebugRender(bool state) {
        _bufferData._otherData.y = state ? 1.0f : 0.0f;
    }

    inline void toggleShadowMapping(bool state) {
        _bufferData._otherData.z = state ? 1.0f : 0.0f;
    }

    inline void setRendererFlag(U32 flag) {
        _bufferData._otherData.w = to_float(flag);
    }

    inline void deltaTime(F32 deltaTimeSeconds) {
        _bufferData._otherData2.x = deltaTimeSeconds;
    }

    inline void lightCount(LightType type, U32 lightCount) {
        _bufferData._lightCountPerType[to_uint(type)] = lightCount;
    }

    void uploadToGPU();
    void init();

  private:
      SceneShaderBufferData _bufferData;
      /// Generic scene data that doesn't change per shader
      ShaderBuffer* _sceneShaderData;
};
}; //namespace Divide

#endif //_SCENE_SHADER_DATA_H_
