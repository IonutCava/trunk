/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _LIGHT_MANAGER_H_
#define _LIGHT_MANAGER_H_

#include "core.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Rendering/Lighting/Headers/LightGrid.h"
#include "Managers/Headers/FrameListenerManager.h"

class ShaderBuffer;
class SceneGraphNode;
class SceneRenderState;

DEFINE_SINGLETON_EXT1(LightManager, FrameListener)

public:
    enum ShadowSlotType {
        SHADOW_SLOT_TYPE_NORMAL = 0,
        SHADOW_SLOT_TYPE_CUBE = 1,
        SHADOW_SLOT_TYPE_ARRAY = 2
    };

    void init();

    ///Add a new light to the manager
    bool addLight(Light* const light);
    ///remove a light from the manager
    bool removeLight(U32 lightId);
    ///Update the ambient light values used in shader programs
    inline void setAmbientLight(const vec3<F32>& light){_ambientLight = light;}
    ///Retrieve the current ambient light values
    inline const vec3<F32>& getAmbientLight() const {return _ambientLight;}

    bool clear();
    void idle();
    void onCameraChange();
    inline Light::LightMap& getLights() {return _lights;}
           Light* getLight(U32 slot);

    inline Light*    getCurrentLight()             const { return _currLight; }
    inline void      setCurrentLight(Light* light)       { _currLight = light; _currentShadowPass = 0; }

    void updateResolution(I32 newWidth, I32 newHeight);

    ///shadow mapping
    void bindDepthMaps();
    bool shadowMappingEnabled() const;

    ///shadow mapping
    void previewShadowMaps(Light* light = nullptr);
    void togglePreviewShadowMaps();

    inline U8   currentShadowPass()  const { return _currentShadowPass; }
    inline void registerShadowPass()       { _currentShadowPass++; }

    void updateAndUploadLightData(const mat4<F32>& viewMatrix);

    /// Get the appropriate shadow bind slot for every light's shadow
    inline U8 getShadowBindSlot(ShadowSlotType type, U8 lightIndex) {
        assert(lightIndex < Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE);
        switch (type){
            default:
            case SHADOW_SLOT_TYPE_NORMAL: return normShadowLocation[lightIndex];
            case SHADOW_SLOT_TYPE_CUBE  : return cubeShadowLocation[lightIndex];
            case SHADOW_SLOT_TYPE_ARRAY : return arrayShadowLocation[lightIndex];
        };
    }

    /// Get the appropriate shadow bind slot for every light's shadow
    inline U8 getShadowBindSlot(LightType lightType, U8 lightIndex) {
        switch (lightType){
            default:
            case LIGHT_TYPE_SPOT        : return  getShadowBindSlot(SHADOW_SLOT_TYPE_NORMAL, lightIndex);
            case LIGHT_TYPE_POINT       : return  getShadowBindSlot(SHADOW_SLOT_TYPE_CUBE, lightIndex);
            case LIGHT_TYPE_DIRECTIONAL : return  getShadowBindSlot(SHADOW_SLOT_TYPE_ARRAY, lightIndex);
        };
    }

    bool buildLightGrid(const mat4<F32>& viewMatrix, const mat4<F32>& projectionMatrix, const vec2<F32>& zPlanes);

protected:
    ///This is inherited from FrameListener and is used to queue up reflection on every frame start
    bool framePreRenderEnded(const FrameEvent& evt);

private:
    enum ShaderBufferType {
        SHADER_BUFFER_NORMAL     = 0,
        SHADER_BUFFER_SHADOW     = 1,
        ShaderBuffer_PLACEHOLDER = 2
    };

    LightManager();
    ~LightManager();

private:
    Light::LightMap  _lights;
    bool      _init;
    bool      _previewShadowMaps;
    Light*    _currLight;
    bool      _shadowMapsEnabled;
    vec3<F32> _ambientLight;
    vec2<U16> _cachedResolution;
    U8        _currentShadowPass; //<used by CSM. Resets to 0 for every light

    vectorImpl<LightProperties >       _lightProperties;
    vectorImpl<LightShadowProperties > _lightShadowProperties;

    ShaderBuffer*           _lightShaderBuffer[ShaderBuffer_PLACEHOLDER];

    LightGrid*              _opaqueGrid;
    LightGrid*              _transparentGrid;

    U8 normShadowLocation[Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE];
    U8 cubeShadowLocation[Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE];
    U8 arrayShadowLocation[Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE];

END_SINGLETON

#endif