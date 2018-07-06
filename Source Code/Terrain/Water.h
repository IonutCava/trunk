/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

#ifndef _WATER_PLANE_H_
#define _WATER_PLANE_H_

#include "resource.h"
#include "Geometry/Object3D.h"
#include "Geometry/Predefined/Quad3D.h"

#include "Hardware/Video/FrameBufferObject.h"
#include "Utility/Headers/ParamHandler.h"

class Shader;
class WaterPlane : public SceneNode
{
public:
	WaterPlane();
	~WaterPlane(){}
	void render();
	void setFBO(FrameBufferObject* fbo[3]);
	bool load(const std::string& name);
	bool unload();

	void setParams(F32 shininess, F32 noiseTile, F32 noiseFactor, F32 transparency);
	void setWaterTextureProjectionMatrix(const mat4& matrix) {_projectionMatrix = matrix;}
	inline Quad3D*     getQuad()    {return _plane;}
	bool computeBoundingBox() {return getQuad()->getBoundingBox().isComputed();}
private:
	Quad3D*			   _plane;
	Texture2D*		   _texture;
	Shader*		  	   _shader;
	mat4               _projectionMatrix;
	F32  _maxViewDistance;
	FrameBufferObject* _fbo[3];
	vec3 _waterMinDepth;
	vec3 _waterMaxDepth;
};
#endif