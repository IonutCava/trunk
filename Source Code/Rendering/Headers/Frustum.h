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

#ifndef _FRUSTUM_H_
#define _FRUSTUM_H_

#include "Core/Math/Headers/MathClasses.h"
#include "Core/Headers/Singleton.h"

#define FRUSTUM_OUT			0
#define FRUSTUM_IN			1
#define FRUSTUM_INTERSECT	2

class BoundingBox;
DEFINE_SINGLETON( Frustum )

public:
	void Extract(const vec3<F32>& eye);

	bool ContainsPoint(const vec3<F32>& point) const;
	I8  ContainsBoundingBox(BoundingBox& bbox) const;
	I8  ContainsSphere(const vec3<F32>& center, F32 radius) const;


	inline       vec3<F32>& getEyePos()					           {return _eyePos;}
	inline const mat4<F32>& getModelviewMatrix()		           {return _modelViewMatrix;}
	inline const mat4<F32>& getModelviewInvMatrix() 		       {return _modelViewMatrixInv;}
	inline const mat4<F32>& getProjectionMatrix()		 	       {return _projectionMatrix;}
	inline const mat4<F32>& getModelViewProjectionMatrix()         {return _modelViewProjectionMatrix;}
	inline const mat4<F32>& getInverseModelViewProjectionMatrix()  {return _inverseModelViewProjectionMatrix;}

private:
	vec3<F32> _eyePos;
	vec4<F32> _frustumPlanes[6];	
	mat4<F32> _modelViewMatrix, _modelViewMatrixInv;	///< Modelview Matrix and it's inverse
	mat4<F32> _projectionMatrix; 				        ///< Projection Matrix
	mat4<F32> _modelViewProjectionMatrix;			    ///< Modelview * Projection
	mat4<F32> _inverseModelViewProjectionMatrix;      ///< Projection * inverse(ModelView)

END_SINGLETON

#endif
