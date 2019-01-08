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

#ifndef _LIGHT_POOL_H_
#define _LIGHT_POOL_H_

#include "config.h"

#include "Light.h"

#include "Scenes/Headers/SceneComponent.h"
#include "Platform/Threading/Headers/Task.h"
#include "Platform/Video/Headers/RenderStagePass.h"
#include "Core/Headers/PlatformContextComponent.h"

namespace Divide {

namespace Time {
    class ProfileTimer;
};

class ShaderBuffer;
class SceneGraphNode;
class SceneRenderState;

class LightPool : public SceneComponent,
                  public PlatformContextComponent {
  protected:
      struct LightProperties {
          /// rgb = diffuse
          /// w = cosOuterConeAngle;
          FColour _diffuse = { DefaultColours::WHITE.rgb(), 0.0f };
          /// light position (or direction for Directional lights)
          /// w = range
          vec4<F32> _position = { 0.0f, 0.0f, 0.0f, 0.0f };
          /// xyz = spot direction
          /// w = spot angle
          vec4<F32> _direction = { 0.0f, 0.0f, 0.0f, 45.0f };
          /// x = light type: 0 - directional, 1 - point, 2 - spot
          /// y = casts shadows, 
          /// z - reserved
          /// w = reserved
          vec4<I32> _options = { 0, 1, 0, 0 };
      };

  public:
    typedef vector<Light*> LightList;

    explicit LightPool(Scene& parentScene, PlatformContext& context);
    ~LightPool();

    /// Add a new light to the manager
    bool addLight(Light& light);
    /// remove a light from the manager
    bool removeLight(Light& light);
    /// disable or enable a specific light type
    inline void toggleLightType(LightType type) {
        toggleLightType(type, !lightTypeEnabled(type));
    }
    inline void toggleLightType(LightType type, const bool state) {
        _lightTypeState[to_U32(type)] = state;
    }
    inline bool lightTypeEnabled(LightType type) const {
        return _lightTypeState[to_U32(type)];
    }
    /// Retrieve the number of active lights in the scene;
    inline const U32 getActiveLightCount(RenderStage stage, LightType type) const {
        return _activeLightCount[to_base(stage)][to_U32(type)];
    }

    bool clear();
    inline LightList& getLights(LightType type) { 
        SharedLock r_lock(_lightLock); 
        return _lights[to_U32(type)];
    }

    Light* getLight(I64 lightGUID, LightType type);

    void prepareLightData(RenderStage stage,
                          const vec3<F32>& eyePos, const mat4<F32>& viewMatrix);
    void uploadLightData(RenderStage stage,
                         ShaderBufferLocation lightDataLocation,
                         ShaderBufferLocation shadowDataLocation,
                         GFX::CommandBuffer& bufferInOut);

    void drawLightImpostors(RenderStage stage, GFX::CommandBuffer& bufferInOut) const;

    void postRenderAllPasses();

    static void idle();
    /// shadow mapping
    static void bindShadowMaps(GFXDevice& context, GFX::CommandBuffer& bufferInOut);
    static void togglePreviewShadowMaps(GFXDevice& context, Light& light);

    /// Get the appropriate shadow bind slot for every light's shadow
    static U8 getShadowBindSlotOffset(ShadowType type) {
        return _shadowLocation[to_U32(type)];
    }

    /// Get the appropriate shadow bind slot offset for every light's shadow
    static U8 getShadowBindSlotOffset(LightType lightType) {
        switch (lightType) {
            default:
            case LightType::SPOT:
                return getShadowBindSlotOffset(ShadowType::SINGLE);
            case LightType::POINT:
                return getShadowBindSlotOffset(ShadowType::CUBEMAP);
            case LightType::DIRECTIONAL:
                return getShadowBindSlotOffset(ShadowType::LAYERED);
        };
    }
    
  protected:
    typedef vectorEASTL<Light::ShadowProperties> LightShadowProperties;
    typedef vector<Light*> LightVec;

    friend class SceneManager;
    bool generateShadowMaps(const Camera& playerCamera, GFX::CommandBuffer& bufferInOut);

    inline LightList::const_iterator findLight(I64 GUID, LightType type) const {
        SharedLock r_lock(_lightLock);
        return findLightLocked(GUID, type);
    }

    inline LightList::const_iterator findLightLocked(I64 GUID, LightType type) const {
        return std::find_if(std::cbegin(_lights[to_U32(type)]), std::cend(_lights[to_U32(type)]),
                            [&GUID](Light* const light) {
                                return (light && light->getGUID() == GUID);
                            });
    }

    void shadowCastingLights(const vec3<F32>& eyePos, LightVec& sortedShadowLights) const;

  private:
      void init();

  private:
     struct BufferData {
         // x = directional light count, y = point light count, z = spot light count, w = shadow light count
         vec4<I32> _globalData;
         std::array<LightProperties, Config::Lighting::MAX_POSSIBLE_LIGHTS> _lightProperties;
     };

    std::array<std::array<U32, to_base(LightType::COUNT)>, to_base(RenderStage::COUNT)> _activeLightCount;
    std::array<LightVec, to_base(RenderStage::COUNT)> _sortedLights;

    std::array<BufferData, to_base(RenderStage::COUNT)> _sortedLightProperties;

    ShaderBuffer* _lightShaderBuffer;
    ShaderBuffer* _shadowBuffer;

    LightShadowProperties _sortedShadowProperties;

    mutable SharedMutex _lightLock;
    std::array<bool, to_base(LightType::COUNT)> _lightTypeState;
    std::array<LightList, to_base(LightType::COUNT)> _lights;
    bool _init;
    Texture_ptr _lightIconsTexture;
    ShaderProgram_ptr _lightImpostorShader;

    Time::ProfileTimer& _shadowPassTimer;

    static bool _previewShadowMaps;
    static std::array<U8, to_base(ShadowType::COUNT)> _shadowLocation;
};

};  // namespace Divide

#endif //_LIGHT_POOL_H_