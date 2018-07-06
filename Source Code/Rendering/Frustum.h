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

#ifndef FRUSTUM_H_
#define FRUSTUM_H_

#include "Hardware/Platform/PlatformDefines.h"
#include "Utility/Headers/MathClasses.h"
#include "Utility/Headers/Singleton.h"

#define FRUSTUM_OUT			0
#define FRUSTUM_IN			1
#define FRUSTUM_INTERSECT	2

class BoundingBox;
SINGLETON_BEGIN( Frustum )

public:
	void Extract(const vec3& eye);

	bool ContainsPoint(const vec3& point) const;
	I8  ContainsBoundingBox(BoundingBox& bbox) const;
	I8  ContainsSphere(const vec3& center, F32 radius) const;


	vec3& getEyePos()					{return _eyePos;}
	mat4& getModelviewMatrix()			{return _modelViewMatrix;}
	mat4& getModelviewInvMatrix()		{return _modelViewMatrixInv;}
	mat4& getProjectionMatrix()			{return _projectionMatrix;}

private:
	vec3	_eyePos;
	vec4	_frustumPlanes[6];	
	mat4	_modelViewMatrix, _modelViewMatrixInv;	// Modelview Matrix and it's inverse
	mat4	_projectionMatrix; 				// Projection Matrix
	mat4	_modelViewProjectionMatrix;			// Modelview * Projection

SINGLETON_END()

#endif
