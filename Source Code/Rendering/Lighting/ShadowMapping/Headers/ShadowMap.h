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
#ifndef _SHADOW_MAP_H_
#define _SHADOW_MAP_H_

#include "core.h"

enum ShadowType{

	SHADOW_TYPE_NONE = -1,
    SHADOW_TYPE_Single,
    SHADOW_TYPE_PSSM,
    SHADOW_TYPE_CubeMap,
    SHADOW_TYPE_PLACEHOLDER,
};

class Light;
class ParamHandler;
class FrameBufferObject;
class SceneRenderState;
///All the information needed for a single light's shadowmap
class ShadowMap {
public:
	ShadowMap(Light* light);
	virtual ~ShadowMap();
	///Render the scene and save the frame to the shadow map
	virtual void render(SceneRenderState* renderState, boost::function0<void> sceneRenderFunction) = 0;
	///Setup needed before rendering the light
	void preRender();
	///Setup needed after rendering the light
	void postRender();
	///Get the current shadow mapping tehnique
	virtual ShadowType getShadowMapType() const = 0;

	inline  FrameBufferObject* getDepthMap() {return _depthMap;}
	inline  bool isBound() {return _isBound;}
	        U16  resolution();
	virtual void resolution(U16 resolution,SceneRenderState* sceneRenderState) {_init = true;}
	virtual bool Bind(U8 offset);
	virtual bool Unbind(U8 offset);
	virtual void previewShadowMaps() = 0;
protected:
	virtual void renderInternal(SceneRenderState* renderState) const = 0;

protected:
	///The depth maps. Number depends on the current method
	FrameBufferObject*  _depthMap;
	///A global resolution factor for all methods (higher = better quality)
	F32 _resolutionFactor;
	U16 _maxResolution;
	boost::function0<void> _callback;
	///Internal pointer to the parrent light
	Light* _light;
	ParamHandler& _par;
	bool _init;
	bool _isBound;
};

class ShadowMapInfo {
public:
	ShadowMapInfo(Light* light);
	virtual ~ShadowMapInfo();
	inline ShadowMap* getShadowMap() {return _shadowMap;}
	       ShadowMap* getOrCreateShadowMap(SceneRenderState* sceneRenderState);
	inline U16  resolution() {return _resolution;}
	inline void resolution(U16 resolution,SceneRenderState* sceneRenderState) {_resolution = resolution; if(_shadowMap) _shadowMap->resolution(_resolution,sceneRenderState);}
private:
	U16        _resolution;
	ShadowMap* _shadowMap;
	Light*     _light;
public:
	
	U8         _numSplits;
};

#endif