/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef _LIGHT_MANAGER_H_
#define _LIGHT_MANAGER_H_

#include "core.h"
#include "Rendering/Lighting/Headers/Light.h"

class SceneGraphNode;
class SceneRenderState;

DEFINE_SINGLETON(LightManager)

public:
	void init();
	typedef Unordered_map<U32, Light*> LightMap;
	///Add a new light to the manager
	bool addLight(Light* const light);
	///remove a light from the manager
	bool removeLight(U32 lightId);
	///set ambient light properties
	void setAmbientLight(const vec4<F32>& light);
	///Find all the lights affecting the currend node. Return the number of found lights
	///Note: the returned value is clamped between 0 and MAX_LIGHTS_PER_SCENE_NODE
	///Use typeFilter to find only lights of a certain type
	U8 findLightsForSceneNode(SceneGraphNode* const node, LightType typeFilter = LIGHT_TYPE_PLACEHOLDER );
	bool clear();
	U32  generateNewID();
	void update(bool force = false);
	void idle();
	inline LightMap& getLights()      {return _lights;}
	inline Light*    getLight(U32 id) {return _lights[id];}
	inline Light*    getLightForCurrentNode(U8 index) {assert(index < _currLightsPerNode.size()); _currLight = _currLightsPerNode[index]; return _currLight;}
	///shadow mapping
	void bindDepthMaps(Light* light,U8 lightIndex, U8 offset = 8, bool overrideDominant = false);
	void unbindDepthMaps(Light* light, U8 offset = 8, bool overrideDominant = false);
	bool shadowMappingEnabled();
	void generateShadowMaps(SceneRenderState* renderState);
	inline void setDominantLight(Light* const light) {_dominantLight = light;}

	///shadow mapping
	void previewShadowMaps(Light* light = NULL);
	inline void togglePreviewShadowMaps() {_previewShadowMaps = !_previewShadowMaps;}
	vectorImpl<I32 > getDepthMapResolution();
	inline const vectorImpl<mat4<F32> >& getLightProjectionMatricesCache() {return _lightProjectionMatricesCache;}
	inline U16   getLightCountForCurrentNode() {return _currLightsPerNode.size();}
	inline vectorImpl<I32> getLightTypesForCurrentNode() {return _currLightTypes;}

	bool checkId(U32 value);
	void drawDepthMap(U8 light, U8 index);

private:
	LightManager();
	~LightManager();

private:
	LightMap _lights;
	bool     _previewShadowMaps;
	Light*   _dominantLight;
	Light*   _currLight;
	bool     _shadowMapsEnabled;
	I32      _shadowArrayOffset;
	I32      _shadowCubeOffset;
	vectorImpl<I32>         _currLightTypes;
	vectorImpl<Light* >     _currLightsPerNode;
	vectorImpl<mat4<F32 > > _lightProjectionMatricesCache;
END_SINGLETON

#endif