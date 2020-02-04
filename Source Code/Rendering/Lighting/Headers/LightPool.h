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
          FColour4 _diffuse = { DefaultColours::WHITE.rgb(), 0.0f };
          /// light position ((0,0,0) for directional lights)
          /// w = range
          vec4<F32> _position = { 0.0f, 0.0f, 0.0f, 0.0f };
          /// xyz = light direction (spot and directional lights only. (0,0,0) for point)
          /// w = spot angle
          vec4<F32> _direction = { 0.0f, 0.0f, 0.0f, 45.0f };
          /// x = light type: 0 - directional, 1 - point, 2 - spot, 3 - none
          /// y = shadow index (-1 = no shadows)
          /// z = reserved
          /// w = reserved
          vec4<I32> _options = { 3, 1, 0, 0 };
      };

#pragma pack(push, 1)
      struct ShadowProperties {
          vec4<F32> _lightDetails[Config::Lighting::MAX_SHADOW_CASTING_LIGHTS];
          vec4<F32> _lightPosition[ShadowMap::MAX_SHADOW_PASSES];
          mat4<F32> _lightVP[ShadowMap::MAX_SHADOW_PASSES];

          inline bufferPtr data() const {
              return (bufferPtr)&_lightDetails[0]._v;
          }
      };
#pragma pack(pop)

  public:
    using LightList = vectorEASTL<Light*>;

    explicit LightPool(Scene& parentScene, PlatformContext& context);
    ~LightPool();

    /// Add a new light to the manager
    bool addLight(Light& light);
    /// remove a light from the manager
    bool removeLight(Light& light);
    /// disable or enable a specific light type
    inline void toggleLightType(LightType type) noexcept {
        toggleLightType(type, !lightTypeEnabled(type));
    }
    inline void toggleLightType(LightType type, const bool state) noexcept {
        _lightTypeState[to_U32(type)] = state;
    }
    inline bool lightTypeEnabled(LightType type) const noexcept {
        return _lightTypeState[to_U32(type)];
    }
    /// Retrieve the number of active lights in the scene;
    inline const U32 getActiveLightCount(RenderStage stage, LightType type) const noexcept {
        return _activeLightCount[to_base(stage)][to_U32(type)];
    }

    bool clear();
    inline LightList& getLights(LightType type) { 
        SharedLock r_lock(_lightLock); 
        return _lights[to_U32(type)];
    }

    Light* getLight(I64 lightGUID, LightType type);

    void prepareLightData(RenderStage stage, const vec3<F32>& eyePos, const mat4<F32>& viewMatrix);

    void uploadLightData(RenderStage stage, GFX::CommandBuffer& bufferInOut);

    void drawLightImpostors(RenderStage stage, GFX::CommandBuffer& bufferInOut) const;

    void preRenderAllPasses(const Camera& playerCamera);
    void postRenderAllPasses(const Camera& playerCamera);

    static void idle();
    static void togglePreviewShadowMaps(GFXDevice& context, Light& light);

    /// Get the appropriate shadow bind slot for every light's shadow
    static U8 getShadowBindSlotOffset(ShadowType type) noexcept {
        return _shadowLocation[to_U32(type)];
    }

    /// Get the appropriate shadow bind slot offset for every light's shadow
    static U8 getShadowBindSlotOffset(LightType lightType) noexcept {
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
    typedef std::array<Light::ShadowProperties, Config::Lighting::MAX_SHADOW_CASTING_LIGHTS> LightShadowProperties;

    friend class SceneManager;
    void generateShadowMaps(const Camera& playerCamera, GFX::CommandBuffer& bufferInOut);

    inline LightList::const_iterator findLight(I64 GUID, LightType type) const {
        SharedLock r_lock(_lightLock);
        return findLightLocked(GUID, type);
    }

    inline LightList::const_iterator findLightLocked(I64 GUID, LightType type) const {
        return eastl::find_if(eastl::cbegin(_lights[to_U32(type)]), eastl::cend(_lights[to_U32(type)]),
                             [&GUID](Light* const light) noexcept {
                                 return (light && light->getGUID() == GUID);
                             });
    }

  private:
      void init();

  private:
     struct BufferData {
         // x = directional light count, y = point light count, z = spot light count, w = shadow light count
         vec4<U32> _globalData;
         std::array<LightProperties, Config::Lighting::MAX_POSSIBLE_LIGHTS> _lightProperties;
     };

    std::array<std::array<U32, to_base(LightType::COUNT)>, to_base(RenderStage::COUNT)> _activeLightCount;
    std::array<LightList, to_base(RenderStage::COUNT)> _sortedLights;

    std::array<BufferData, to_base(RenderStage::COUNT)> _sortedLightProperties;

    ShaderBuffer* _lightShaderBuffer;
    ShaderBuffer* _shadowBuffer;

    LightList _sortedShadowLights;
    ShadowProperties _shadowBufferData;

    mutable SharedMutex _lightLock;
    std::array<bool, to_base(LightType::COUNT)> _lightTypeState;
    std::array<LightList, to_base(LightType::COUNT)> _lights;
    bool _init;
    Texture_ptr _lightIconsTexture;
    ShaderProgram_ptr _lightImpostorShader;

    Time::ProfileTimer& _shadowPassTimer;

    static bool _debugDraw;
    static std::array<U8, to_base(ShadowType::COUNT)> _shadowLocation;
};

};  // namespace Divide

#endif //_LIGHT_POOL_H_