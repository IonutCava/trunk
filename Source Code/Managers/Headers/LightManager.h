/*
   Copyright (c) 2013 DIVIDE-Studio
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

class SceneGraphNode;
class SceneRenderState;

DEFINE_SINGLETON_EXT1(LightManager,FrameListener)

public:
	void init();
	typedef Unordered_map<U32, Light*> LightMap;
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
	U8 findLightsForSceneNode(SceneGraphNode* const node, LightType typeFilter = LIGHT_TYPE_PLACEHOLDER );
	bool clear();
	U32  generateNewID();
	void update();
	void idle();
	inline LightMap& getLights()      {return _lights;}
	inline Light*    getLight(U32 id) {return _lights[id];}
	inline Light*    getLightForCurrentNode(U8 index) {assert(index < _currLightsPerNode.size()); _currLight = _currLightsPerNode[index]; return _currLight;}
	///shadow mapping
	void bindDepthMaps(Light* light,U8 lightIndex, U8 offset = 8, bool overrideDominant = false);
	void unbindDepthMaps(Light* light, U8 offset = 8, bool overrideDominant = false);
	bool shadowMappingEnabled() const;
	inline void setDominantLight(Light* const light) {_dominantLight = light;}

	///shadow mapping
	void previewShadowMaps(Light* light = NULL);
	inline void togglePreviewShadowMaps() {_previewShadowMaps = !_previewShadowMaps;}
	vectorImpl<I32 > getDepthMapResolution() const;

	inline       U16                     getLightCountForCurrentNode()          const {return _currLightsPerNode.size();}
	inline const vectorImpl<mat4<F32> >& getLightProjectionMatricesCache()      const {return _lightProjectionMatricesCache;}
	inline const vectorImpl<I32>&        getLightTypesForCurrentNode()          const {return _currLightTypes;}
    inline const vectorImpl<I32>&        getLightsEnabledForCurrentNode()       const {return _currLightsEnabled;}
	inline const vectorImpl<I32>&        getShadowCastingLightsForCurrentNode() const {return _currShadowLights;}
	bool checkId(U32 value);
	void drawDepthMap(U8 light, U8 index);

protected:
	///This is inherited from FrameListener and is used to queue upreflection on every frame start
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
	I32       _shadowArrayOffset;
	I32       _shadowCubeOffset;
	vec4<F32> _ambientLight;
	vectorImpl<I32>         _currLightTypes;
    vectorImpl<I32>         _currLightsEnabled;
	vectorImpl<I32>         _currShadowLights;
	vectorImpl<Light* >     _currLightsPerNode;
	vectorImpl<mat4<F32 > > _lightProjectionMatricesCache;
END_SINGLETON

#endif