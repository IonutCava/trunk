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
#include "Managers/Headers/FrameListenerManager.h"

class ShaderBuffer;
class SceneGraphNode;
class SceneRenderState;

DEFINE_SINGLETON_EXT1(LightManager,FrameListener)

public:
    enum ShadowSlotType {
        SHADOW_SLOT_TYPE_NORMAL = 0,
        SHADOW_SLOT_TYPE_CUBE = 1,
        SHADOW_SLOT_TYPE_ARRAY = 2
    };

    typedef Unordered_map<U32, Light*> LightMap;

    void init();

    ///Add a new light to the manager
    bool addLight(Light* const light);
    ///remove a light from the manager
    bool removeLight(U32 lightId);
    ///Update the ambient light values used in shader programs
    inline void setAmbientLight(const vec4<F32>& light){_ambientLight = light;}
    ///Retrieve the current ambient light values
    inline const vec4<F32>& getAmbientLight() const {return _ambientLight;}
    ///Find all the lights affecting the currend node. Return the number of found lights
    ///Note: the returned value is clamped between 0 and MAX_LIGHTS_PER_SCENE_NODE
    ///Use typeFilter to find only lights of a certain type
    U8   findLightsForSceneNode(SceneGraphNode* const node, LightType typeFilter = LIGHT_TYPE_PLACEHOLDER );
    U32  generateNewID();
    bool clear();
    void idle();
    void update(const bool force = false);
    inline LightMap& getLights()      {return _lights;}
    inline Light*    getLight(U32 id) {if(_lights.find(id) != _lights.end() ) return _lights[id]; else return nullptr;}
    inline Light*    getLightForCurrentNode(U8 index) {assert(index < _currLightsPerNode.size()); _currLight = _currLightsPerNode[index]; return _currLight;}

    inline Light*    getCurrentLight()             const { return _currLight; }
    inline void      setCurrentLight(Light* light)       { _currLight = light; _currentShadowPass = 0; }

    void updateResolution(I32 newWidth, I32 newHeight);

    ///shadow mapping
    void bindDepthMaps(U8 lightIndex, bool overrideDominant = false);
    bool shadowMappingEnabled() const;
    inline void setDominantLight(Light* const light) {_dominantLight = light;}

    ///shadow mapping
    void previewShadowMaps(Light* light = nullptr);
    void togglePreviewShadowMaps();

    inline       U16                     getLightCountForCurrentNode()          const {return (U16)_currLightsPerNode.size();}
    inline const vectorImpl<I32>&        getLightTypesForCurrentNode()          const {return _currLightTypes;}
    inline const vectorImpl<I32>&        getLightIndicesForCurrentNode()        const {return _currLightIndices;}
    inline const vectorImpl<I32>&        getShadowCastingLightsForCurrentNode() const {return _currShadowLights;}
    bool checkId(U32 value);
    void drawDepthMap(U8 light, U8 index);

    inline U8   currentShadowPass()  const { return _currentShadowPass; }
    inline void registerShadowPass()       { _currentShadowPass++; }

    void setLight(Light* const light, bool shadowPass = false);

    /// Get the appropriate shadow bind slot for every light's shadow
    inline I32 getShadowBindSlot(ShadowSlotType type, U8 lightIndex) {
        assert(lightIndex < Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE);
        I32 retValue = -1;
        switch (type){
            case SHADOW_SLOT_TYPE_NORMAL: retValue = normShadowLocation[lightIndex];  break;
            case SHADOW_SLOT_TYPE_CUBE  : retValue = cubeShadowLocation[lightIndex];  break;
            case SHADOW_SLOT_TYPE_ARRAY : retValue = arrayShadowLocation[lightIndex]; break;
        };

        return retValue;
    }
    /// Get the appropriate shadow bind slot for every light's shadow
    inline I32 getShadowBindSlot(LightType lightType, U8 lightIndex) {
        ShadowSlotType type = SHADOW_SLOT_TYPE_NORMAL;
        switch (lightType){
            case LIGHT_TYPE_DIRECTIONAL : type = SHADOW_SLOT_TYPE_ARRAY; break;
            case LIGHT_TYPE_POINT : type = SHADOW_SLOT_TYPE_CUBE; break;
            case LIGHT_TYPE_SPOT  : type = SHADOW_SLOT_TYPE_NORMAL; break;
        };
        return getShadowBindSlot(type, lightIndex);
    }

protected:
    ///This is inherited from FrameListener and is used to queue up reflection on every frame start
    bool framePreRenderEnded(const FrameEvent& evt);

private:
    LightManager();
    ~LightManager();

private:
    LightMap  _lights;
    bool      _previewShadowMaps;
    Light*    _dominantLight;
    Light*    _currLight;
    bool      _shadowMapsEnabled;
    vec4<F32> _ambientLight;
    mat4<F32> _viewMatrixCache;
    U8        _currentShadowPass; //<used by CSM. Resets to 0 for every light
    vectorImpl<I32>         _currLightTypes;
    vectorImpl<I32>         _currShadowLights;
    vectorImpl<I32>         _currLightIndices;
    vectorImpl<Light* >     _currLightsPerNode;
    vectorImpl<Light* >     _tempLightsPerNode;

    ShaderBuffer*           _lightShaderBuffer;
    ShaderBuffer*           _shadowShaderBuffer;


    I32 normShadowLocation[Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE];
    I32 cubeShadowLocation[Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE];
    I32 arrayShadowLocation[Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE];

END_SINGLETON

#endif