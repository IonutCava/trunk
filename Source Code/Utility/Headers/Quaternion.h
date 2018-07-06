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

#ifndef _QUATERNION_H_
#define _QUATERNION_H_
/*
http://gpwiki.org/index.php/OpenGL:Tutorials:Using_Quaternions_to_represent_rotation
Quaternion class based on code from " OpenGL:Tutorials:Using Quaternions to represent rotation "
*/
#include "resource.h"
#define TOLERANCE 0.00001f

class Quaternion
{
public:
	Quaternion(F32 x, F32 y, F32 z, F32 w) : _x(x), _y(y), _z(z), _w(w),_dirty(true),_mat(NULL) {}
	Quaternion() : _x(0), _y(0), _z(0), _w(0),_dirty(true),_mat(NULL) {}

	void       normalize();
	Quaternion getConjugate() const;
	Quaternion operator* (const Quaternion &rq) const;
	vec3       operator* (const vec3 &vec) const;

	void	   FromAxis(const vec3& v, float angle);
	void       FromEuler(float pitch, float yaw, float roll);
	void       FromEuler(const vec3& v) {FromEuler(v.x,v.y,v.z);}
	mat4&      getMatrix();
	void       getAxisAngle(vec3 *axis, float *angle,bool inDegrees);
	bool       compare(Quaternion& q);
private:
	F32 _x,_y,_z,_w;
	mat4 *_mat;
	bool _dirty;
};
#endif