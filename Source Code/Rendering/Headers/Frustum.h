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
	I8  ContainsBoundingBox(const BoundingBox& bbox) const;
	I8  ContainsSphere(const vec3<F32>& center, F32 radius) const;

    inline void setZPlanes(const vec2<F32>& zPlanes)       {_zPlanes = zPlanes;}
	inline const vec3<F32>& getEyePos()				 const {return _eyePos;}
	inline const vec2<F32>& getZPlanes()             const {return _zPlanes;}

private:
	vec3<F32> _eyePos;
	vec2<F32> _zPlanes;
	vec4<F32> _frustumPlanes[6];

END_SINGLETON

#endif
