/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef _SKY_H
#define _SKY_H

#include "core.h"

class Texture;
class Sphere3D;
class ShaderProgram;
class SceneGraphNode;
class RenderStateBlock;

typedef Texture TextureCubemap;

enum RENDER_STAGE;

///Only one sky is visible per scene, right?
///Singleton it is then. No need to track it via the SceneGraph
DEFINE_SINGLETON( Sky ) 

public:
	void draw() const;
	void setParams(const vec3<F32>& eyePos,const vec3<F32>& sunVect, bool invert, bool drawSun, bool drawSky) ;

	void addToDrawExclusionMask(I32 stageMask);
	void removeFromDrawExclusionMask(I32 stageMask);
	///Draw states are used to test if the current object should be drawn depending on the current render pass
	bool getDrawState(RENDER_STAGE currentStage) const;

private:
	bool load();

private:
	bool			  _init,_invert,_drawSky,_drawSun;
	ShaderProgram*	  _skyShader;
	TextureCubemap*	  _skybox;
	vec3<F32>			  _sunVect,	_eyePos;
	Sphere3D          *_sky,*_sun;
	SceneGraphNode    *_skyNode, *_sunNode;
	U8				  _exclusionMask;
	RenderStateBlock* _skyboxRenderState;

private:
	Sky();
	~Sky();
	void drawSky() const;
	void drawSun() const;
	void drawSkyAndSun() const;

END_SINGLETON

#endif


