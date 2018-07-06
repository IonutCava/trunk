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

#include "resource.h"

class Sphere3D;
class ShaderProgram;
class Texture;
class SceneGraphNode;
enum RENDER_STAGE;
typedef Texture TextureCubemap;
DEFINE_SINGLETON( Sky ) 

public:
	void draw() const;
	void setParams(const vec3& eyePos,const vec3& sunVect, bool invert, bool drawSun, bool drawSky) ;

	void addToRenderExclusionMask(U8 stageMask);
	void removeFromRenderExclusionMask(U8 stageMask);
	bool getRenderState(RENDER_STAGE currentStage) const;

private:
	bool load();

private:
	bool			  _init,_invert,_drawSky,_drawSun;
	ShaderProgram*	  _skyShader;
	TextureCubemap*	  _skybox;
	vec3			  _sunVect,	_eyePos;
	Sphere3D          *_sky,*_sun;
	SceneGraphNode    *_skyNode, *_sunNode;
	U8				  _exclusionMask;

private:
	Sky();
	~Sky();
	void drawSky() const;
	void drawSun() const;
	void drawSkyAndSun() const;

END_SINGLETON

#endif


