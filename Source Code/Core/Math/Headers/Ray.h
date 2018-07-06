/*
 * Ray class, for use with the optimized ray-box
 * intersection test described in:
 *
 *      Amy Williams, Steve Barrus, R. Keith Morley, and Peter Shirley
 *      "An Efficient and Robust Ray-Box Intersection Algorithm"
 *      Journal of graphics tools, 10(1):49-54, 2005
 *
 */

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

#ifndef _RAY_H_
#define _RAY_H_

#include "resource.h"

class Ray {
	public:
		Ray(vec3<F32> &o, vec3<F32> &d) {
			origin = o;
			direction = d;
			inv_direction = vec3<F32>(1/d.x, 1/d.y, 1/d.z);
			sign[0] = (inv_direction.x < 0);
			sign[1] = (inv_direction.y < 0);
			sign[2] = (inv_direction.z < 0);
		}

		Ray(const Ray &r) {
			origin = r.origin;
			direction = r.direction;
			inv_direction = r.inv_direction;
			sign[0] = r.sign[0]; sign[1] = r.sign[1]; sign[2] = r.sign[2];
		}

		vec3<F32> origin;
		vec3<F32> direction;
		vec3<F32> inv_direction;
		I32 sign[3];
};

#endif