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

#ifndef _LIGHT_POOL_H_
#define _LIGHT_POOL_H_

#include "config.h"

#include "Scenes/Headers/SceneComponent.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Platform/Threading/Headers/Task.h"

namespace Divide {

namespace Time {
    class ProfileTimer;
};

class ShaderBuffer;
class SceneGraphNode;
class SceneRenderState;

class LightPool : public SceneComponent {
  protected:
      struct LightProperties {
          /// rgb = diffuse
          /// w = cosOuterConeAngle;
          vec4<F32> _diffuse;
          /// light position (or direction for Directional lights)
          /// w = range
          vec4<F32> _position;
          /// xyz = spot direction
          /// w = spot angle
          vec4<F32> _direction;
          /// x = light type: 0.0 - directional, 1.0  - point, 2.0 - spot#
          /// y = casts shadows, 
          /// z - shadow block index
          /// w - reserved;
          vec4<I32> _options;

          inline void set(const LightProperties& other) {
              _diffuse.set(other._diffuse);
              _position.set(other._position);
              _direction.set(other._direction);
              _options.set(other._options);
          }
      };



  public:
    enum class ShaderBufferType : U32 {
        NORMAL = 0,
        SHADOW = 1,
        COUNT
    };

    explicit LightPool(Scene& parentScene, GFXDevice& context);
    ~LightPool();

    /// Add a new light to the manager
    bool addLight(Light& light);
    /// remove a light from the manager
    bool removeLight(I64 lightGUID, LightType type);
    /// disable or enable a specific light type
    inline void toggleLightType(LightType type) {
        toggleLightType(type, !lightTypeEnabled(type));
    }
    inline void toggleLightType(LightType type, const bool state) {
        _lightTypeState[to_uint(type)] = state;
    }
    inline bool lightTypeEnabled(LightType type) const {
        return _lightTypeState[to_uint(type)];
    }
    /// Retrieve the number of active lights in the scene;
    inline const U32 getActiveLightCount(LightType type) const { return _activeLightCount[to_uint(type)]; }

    bool clear();
    inline Light::LightList& getLights(LightType type) { return _lights[to_uint(type)]; }
    Light* getLight(I64 lightGUID, LightType type);

    /// shadow mapping
    void previewShadowMaps(Light* light);

    void prepareLightData(const vec3<F32>& eyePos, const mat4<F32>& viewMatrix);
    void uploadLightData(ShaderBufferLocation lightDataLocation,
                         ShaderBufferLocation shadowDataLocation);

    void drawLightImpostors(RenderSubPassCmds& subPassesInOut) const;

    static void idle();
    /// shadow mapping
    static void bindShadowMaps(GFXDevice& context);
    static void togglePreviewShadowMaps(GFXDevice& context);
    static bool shadowMappingEnabled();

    /// Get the appropriate shadow bind slot for every light's shadow
    static U8 getShadowBindSlotOffset(ShadowType type) {
        return _shadowLocation[to_uint(type)];
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

    static Light* currentShadowCastingLight() { 
        return _currentShadowCastingLight;
    }

  protected:
    friend class SceneManager;
    bool generateShadowMaps(GFXDevice& context, SceneRenderState& sceneRenderState);

    inline Light::LightList::const_iterator findLight(I64 GUID, LightType type) const {
        return std::find_if(std::begin(_lights[to_uint(type)]), std::end(_lights[to_uint(type)]),
                            [&GUID](Light* const light) {
                                return (light && light->getGUID() == GUID);
                            });
    }

  private:
      void init();

  private:
    GFXDevice& _context;

    vectorImpl<TaskHandle> _lightUpdateTask;

    std::array<bool, to_const_uint(LightType::COUNT)> _lightTypeState;
    std::array<Light::LightList, to_const_uint(LightType::COUNT)> _lights;
    bool _init;
    Texture_ptr _lightIconsTexture;
    ShaderProgram_ptr _lightImpostorShader;
    std::array<U32, to_const_uint(LightType::COUNT)> _activeLightCount;

    std::array<ShaderBuffer*, to_const_uint(ShaderBufferType::COUNT)>  _lightShaderBuffer;

    typedef vectorImpl<LightProperties> LightPropertiesVec;
    typedef vectorImpl<Light::ShadowProperties> LightShadowProperties;
    typedef vectorImpl<Light*> LightVec;

    LightVec _sortedLights;
    LightVec _sortedShadowCastingLights;
    LightPropertiesVec _sortedLightProperties;
    LightShadowProperties _sortedShadowProperties;

    GUID_DELEGATE_CBK _previewShadowMapsCBK;
    Time::ProfileTimer& _shadowPassTimer;

    static bool _previewShadowMaps;
    static bool _shadowMapsEnabled;
    static Light* _currentShadowCastingLight;
    static std::array<U8, to_const_uint(ShadowType::COUNT)> _shadowLocation;
};

};  // namespace Divide

#endif //_LIGHT_POOL_H_