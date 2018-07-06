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

#ifndef _QUATERNION_H_
#define _QUATERNION_H_

/*
http://gpwiki.org/index.php/OpenGL:Tutorials:Using_Quaternions_to_represent_rotation
Quaternion class based on code from " OpenGL:Tutorials:Using Quaternions to represent rotation "
*/
#include "core.h"

#define TOLERANCE 0.00001f

template<class T>
class Quaternion
{
public:
	Quaternion(T x, T y, T z, T w) : _x(x), _y(y), _z(z), _w(w),_dirty(true) {}
	Quaternion() : _x(0), _y(0), _z(0), _w(0),_dirty(true) {}
	Quaternion(const mat3<T>& rotationMatrix) : _dirty(true){FromMatrix(rotationMatrix);}
	//! normalising a quaternion works similar to a vector. This method will not do anything
	//! if the quaternion is close enough to being unit-length. define TOLERANCE as something
	//! small like 0.00001f to get accurate results
	void  normalize() {
		_dirty = true;
		// Don't normalize if we don't have to
		T mag2 = (_w * _w + _x * _x + _y * _y + _z * _z);
		if (  mag2!=0.f && (fabs(mag2 - 1.0f) > TOLERANCE)) {
			T mag = square_root_tpl(mag2);
			_w /= mag;
			_x /= mag;
			_y /= mag;
			_z /= mag;
		}
	}
	//! We need to get the inverse of a quaternion to properly apply a quaternion-rotation to a vector
	//! The conjugate of a quaternion is the same as the inverse, as long as the quaternion is unit-length
	inline Quaternion getConjugate() const { return Quaternion<T>(-_x, -_y, -_z, _w);}
	//! Multiplying q1 with q2 applies the rotation q2 to q1
	//! the constructor takes its arguments as (x, y, z, w)
	Quaternion operator* (const Quaternion &rq) const {
		return Quaternion<T>(_w * rq._x + _x * rq._w + _y * rq._z - _z * rq._y,
			                 _w * rq._y + _y * rq._w + _z * rq._x - _x * rq._z,
				             _w * rq._z + _z * rq._w + _x * rq._y - _y * rq._x,
					         _w * rq._w - _x * rq._x - _y * rq._y - _z * rq._z);
	}
	//! Multiplying a quaternion q with a vector v applies the q-rotation to v
	vec3<T>  operator* (const vec3<T> &vec) const {
		vec3<T> vn(vec);
		vn.normalize();

		Quaternion<T> vecQuat, resQuat;
		vecQuat._x = vn.x;
		vecQuat._y = vn.y;
		vecQuat._z = vn.z;
		vecQuat._w = 0.0f;

		resQuat = vecQuat * getConjugate();
		resQuat = *this * resQuat;

		return (vec3<T>(resQuat._x, resQuat._y, resQuat._z));
	}

	//! Convert from Axis Angle
	void FromAxis(const vec3<T>& v, T angle){
		_dirty = true;
		T sinAngle;
		angle = RADIANS(angle);
		angle *= 0.5f;
		vec3<T> vn(v);
		vn.normalize();

		sinAngle = sin(angle);

		_x = (vn.x * sinAngle);
		_y = (vn.y * sinAngle);
		_z = (vn.z * sinAngle);
		_w = cos(angle);
	}

	inline void FromEuler(const vec3<T>& v) {FromEuler(v.x,v.y,v.z);}

	//! Convert from Euler Angles
	//! Basically we create 3 Quaternions, one for pitch, one for yaw, one for roll
	//! and multiply those together.
	//! the calculation below does the same, just shorter
	void  FromEuler(T pitch, T yaw, T roll) {
		_dirty = true;

 		T p = pitch * M_PIDIV180 / 2.0;
		T y = yaw * M_PIDIV180 / 2.0;
		T r = roll * M_PIDIV180 / 2.0;

		T sinp = sin(p);
		T siny = sin(y);
		T sinr = sin(r);
		T cosp = cos(p);
		T cosy = cos(y);
		T cosr = cos(r);

		this->_x = sinr * cosp * cosy - cosr * sinp * siny;
		this->_y = cosr * sinp * cosy + sinr * cosp * siny;
		this->_z = cosr * cosp * siny - sinr * sinp * cosy;
		this->_w = cosr * cosp * cosy + sinr * sinp * siny;
		normalize();
	}

	void FromMatrix(const mat3<T>& rotationMatrix) {
		T t = 1 + rotationMatrix.a1 + rotationMatrix.b2 + rotationMatrix.c3;

		// large enough
		if( t > static_cast<TReal>(0.001)){
			T s = sqrt( t) * static_cast<T>(2.0);
			_x = (rotationMatrix.c2 - rotationMatrix.b3) / s;
			_y = (rotationMatrix.a3 - rotationMatrix.c1) / s;
			_z = (rotationMatrix.b1 - rotationMatrix.a2) / s;
			_w = static_cast<T>(0.25) * s;
		// else we have to check several cases
		}else if( pRotMatrix.a1 > pRotMatrix.b2 && pRotMatrix.a1 > pRotMatrix.c3 ){
			// Column 0:
			T s = sqrt( static_cast<T>(1.0) + pRotMatrix.a1 - pRotMatrix.b2 - pRotMatrix.c3) * static_cast<T>(2.0);
			_x = static_cast<T>(0.25) * s;
			_y = (rotationMatrix.b1 + rotationMatrix.a2) / s;
			_z = (rotationMatrix.a3 + rotationMatrix.c1) / s;
			_w = (rotationMatrix.c2 - rotationMatrix.b3) / s;
		}else if( rotationMatrix.b2 > rotationMatrix.c3){
			// Column 1:
			T s = sqrt( static_cast<T>(1.0) + rotationMatrix.b2 - rotationMatrix.a1 - rotationMatrix.c3) * static_cast<T>(2.0);
			_x = (rotationMatrix.b1 + rotationMatrix.a2) / s;
			_y = static_cast<T>(0.25) * s;
			_z = (rotationMatrix.c2 + rotationMatrix.b3) / s;
			_w = (rotationMatrix.a3 - rotationMatrix.c1) / s;
		}else{
			// Column 2:
			T s = sqrt( static_cast<T>(1.0) + pRotMatrix.c3 - pRotMatrix.a1 - pRotMatrix.b2) * static_cast<T>(2.0);
			_x = (rotationMatrix.a3 + rotationMatrix.c1) / s;
			_y = (rotationMatrix.c2 + rotationMatrix.b3) / s;
			_z = static_cast<T>(0.25) * s;
			_w = (rotationMatrix.b1 - rotationMatrix.a2) / s;
		}
	}

	//! Convert to Matrix
	const mat4<T>& getMatrix(){
		if(_dirty) {
			T x2 =  _x + _x;
			T y2 = _y + _y;
			T z2 = _z + _z;

			T xx = _x * x2;
			T xy = _x * y2;
			T xz = _x * z2;
			T yy = _y * y2;
			T yz = _y * z2;
			T zz = _z * z2;
			T wx = _w * x2;
			T wy = _w * y2;
			T wz = _w * z2;

			_mat = mat4<T>(1.0f-(yy + zz),  xy + wz,        xz - wy,        0.0f,
		    			   xy - wz,         1.0f-(xx + zz), yz + wx,        0.0f,
		  				   xz + wy,         yz - wx,        1.0f-(xx + yy), 0.0f,
						   0.0f,            0.0f,           0.0f,           1.0f);
			_dirty = false;
		}

		return _mat;
	}

	//! Convert to Axis/Angles
	void  getAxisAngle(vec3<T> *axis, T *angle,bool inDegrees) const{
		T scale = square_root_tpl(_x * _x + _y * _y + _z * _z);
		axis->x = _x / scale;
		axis->y = _y / scale;
		axis->z = _z / scale;
		if(inDegrees)
			*angle = DEGREES(acos(_w) * 2.0f);
		else
			*angle = acos(_w) * 2.0f;
	}

	inline bool compare(const Quaternion& q) const {
		vec4<T> thisQ(_x,_y,_z,_w);
		vec4<T> otherQ(q._x,q._y,q._z,q._w);

		return thisQ.compare(otherQ);
	}

	inline T const& getX() const {return _x;}
	inline T const& getY() const {return _y;}
	inline T const& getZ() const {return _z;}
	inline T const& getW() const {return _w;}

private:
	T _x,_y,_z,_w;
	mat4<T> _mat;
	bool _dirty;
};
#endif