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
#ifndef _SINGLE_SHADOW_MAP_H_
#define _SINGLE_SHADOW_MAP_H_
#include "ShadowMap.h"
class Quad3D;
class ShaderProgram;
///A single shadow map system. Used, for example, by spot lights.
class SingleShadowMap : public ShadowMap {
public:
	SingleShadowMap(Light* light);
	~SingleShadowMap();
	void render(SceneRenderState* renderState, boost::function0<void> sceneRenderFunction);
	///Get the current shadow mapping tehnique
	ShadowType getShadowMapType() const {return SHADOW_TYPE_Single;}
	///Update depth maps
	void resolution(U16 resolution,SceneRenderState* sceneRenderState);
	void previewShadowMaps();

protected:
	void renderInternal(SceneRenderState* renderState) const;

private:
	Quad3D* _renderQuad;
    ShaderProgram* _previewDepthMapShader;
};

#endif