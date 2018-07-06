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

#ifndef _SKY_H
#define _SKY_H

#include "core.h"
#include "Graphs/Headers/SceneNode.h"

class Texture;
class Sphere3D;
class ShaderProgram;
class SceneGraphNode;
class RenderStateBlock;

typedef Texture TextureCubemap;

enum RenderStage;

class Sky : public SceneNode { 

public:
	Sky(const std::string& name);
	~Sky();
	void render(SceneGraphNode* const sgn);
	void setRenderingOptions(const vec3<F32>& eyePos,const vec3<F32>& sunVect, bool invert = false, bool drawSun = true, bool drawSky = true) ;
	void onDraw();
	void prepareMaterial(SceneGraphNode* const sgn);
	void releaseMaterial();

	void addToDrawExclusionMask(I32 stageMask);
	void removeFromDrawExclusionMask(I32 stageMask);
	///Draw states are used to test if the current object should be drawn depending on the current render pass
	bool getDrawState(RenderStage currentStage) const;
	///Skies are always visible (for now. Interiors will change that. Windows will reuqire a occlusion querry(?))
	bool isInView(bool distanceCheck,BoundingBox& boundingBox,const BoundingSphere& sphere) {return true;}
	void postLoad(SceneGraphNode* const sgn);
private:
	bool load();

private:
	bool			  _init,_invert,_drawSky,_drawSun;
	ShaderProgram*	  _skyShader;
	TextureCubemap*	  _skybox;
	vec3<F32>		  _sunVect,	_eyePos;
	Sphere3D          *_sky,*_sun;
	SceneGraphNode    *_sunNode, *_skyGeom;
	U8				  _exclusionMask;
	RenderStateBlock* _skyboxRenderState;
};

#endif


