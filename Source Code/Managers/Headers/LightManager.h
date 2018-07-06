/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _LIGHT_MANAGER_H_
#define _LIGHT_MANAGER_H_

#include "Rendering/Lighting/Headers/Light.h"

namespace Divide {

class ShaderBuffer;
class SceneGraphNode;
class SceneRenderState;

DEFINE_SINGLETON(LightManager)
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
    void init();

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
    inline Light* currentShadowCastingLight() const { return _currentShadowCastingLight; }

    bool clear();
    void idle();
    inline Light::LightList& getLights(LightType type) { return _lights[to_uint(type)]; }
    Light* getLight(I64 lightGUID, LightType type);

    /// shadow mapping
    void bindShadowMaps();
    bool shadowMappingEnabled() const;

    /// shadow mapping
    void previewShadowMaps(Light* light);
    void togglePreviewShadowMaps();

    void updateAndUploadLightData(const vec3<F32>& eyePos, const mat4<F32>& viewMatrix);
    void uploadLightData(LightType lightsByType, ShaderBufferLocation location);

    /// Get the appropriate shadow bind slot for every light's shadow
    U8 getShadowBindSlotOffset(ShadowType type);
    /// Get the appropriate shadow bind slot offset for every light's shadow
    inline U8 getShadowBindSlotOffset(LightType lightType) {
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

    void drawLightImpostors() const;

  protected:
    friend class RenderPass;
    bool generateShadowMaps();

    inline Light::LightList::const_iterator findLight(I64 GUID, LightType type) const {
        return std::find_if(std::begin(_lights[to_uint(type)]), std::end(_lights[to_uint(type)]),
                            [&GUID](Light* const light) {
                                return (light && light->getGUID() == GUID);
                            });
    }

  private:
    enum class ShaderBufferType : U32 {
        NORMAL = 0,
        SHADOW = 1,
        COUNT
    };

    LightManager();
    ~LightManager();

  private:
    std::array<bool, to_const_uint(LightType::COUNT)> _lightTypeState;
    std::array<Light::LightList, to_const_uint(LightType::COUNT)> _lights;
    bool _init;
    bool _previewShadowMaps;
    bool _shadowMapsEnabled;
    Texture* _lightIconsTexture;
    Light* _currentShadowCastingLight;
    ShaderProgram* _lightImpostorShader;
    std::array<U32, to_const_uint(LightType::COUNT)> _activeLightCount;

    std::array<ShaderBuffer*, to_const_uint(ShaderBufferType::COUNT)>  _lightShaderBuffer;
    std::array<U8, to_const_uint(ShadowType::COUNT)> _shadowLocation;

    typedef std::array<LightProperties, Config::Lighting::MAX_POSSIBLE_LIGHTS> LightPropertiesArray;
    std::array<LightPropertiesArray, to_const_uint(LightType::COUNT)> _lightProperties;
    std::array<Light::ShadowProperties, Config::Lighting::MAX_POSSIBLE_LIGHTS> _lightShadowProperties;
    std::array<Light*, Config::Lighting::MAX_SHADOW_CASTING_LIGHTS> _shadowCastingLights;
    vectorImpl<std::future<void>> _lightSortingTasks;
END_SINGLETON

};  // namespace Divide
#endif