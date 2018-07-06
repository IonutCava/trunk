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

  public:
    void init();

    /// Add a new light to the manager
    bool addLight(Light& light);
    /// remove a light from the manager
    bool removeLight(I64 lightGUID, LightType type);
    /// Update the ambient light values used in shader programs
    inline void setAmbientLight(const vec3<F32>& light) { _ambientLight = light; }
    /// Retrieve the current ambient light values
    inline const vec3<F32>& getAmbientLight() const { return _ambientLight; }
    /// Retrieve the number of active lights in the scene;
    inline const U32 getActiveLightCount() const { return _activeLightCount; }
    inline Light* currentShadowCastingLight() const { return _currentShadowCastingLight; }

    bool clear();
    void idle();
    void onCameraUpdate(Camera& camera);
    inline Light::LightList& getLights(LightType type) { return _lights[to_uint(type)]; }
    Light* getLight(I64 lightGUID, LightType type);

    /// shadow mapping
    void bindShadowMaps();
    bool shadowMappingEnabled() const;

    /// shadow mapping
    void previewShadowMaps(Light* light = nullptr);
    void togglePreviewShadowMaps();

    void updateAndUploadLightData(const mat4<F32>& viewMatrix);

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
    std::array<Light::LightList, to_const_uint(LightType::COUNT)> _lights;
    bool _init;
    bool _previewShadowMaps;
    bool _shadowMapsEnabled;
    Light* _currentShadowCastingLight;
    U32 _activeLightCount;
    vec3<F32> _ambientLight;
    mat4<F32> _viewMatrixCache;

    std::array<ShaderBuffer*, to_const_uint(ShaderBufferType::COUNT)>  _lightShaderBuffer;
    std::array<U8, to_const_uint(ShadowType::COUNT)> _shadowLocation;

END_SINGLETON

};  // namespace Divide
#endif