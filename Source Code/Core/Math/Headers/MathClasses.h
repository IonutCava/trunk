/***************************************************************************
 * Mathlib
 *
 * Copyright (C) 2003-2004, Alexander Zaprjagaev <frustum@frustum.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ***************************************************************************
 * Update 2004/08/19
 *
 * added ivec2, ivec3 & ivec4 methods
 * vec2, vec3 & vec4 data : added texture coords (s,t,p,q) and color enums (r,g,b,a)
 * mat3 & mat4 : added multiple float constructor ad modified methods returning mat3 or mat4
 * optimisations like "x / 2.0f" replaced by faster "x * 0.5f"
 * defines of multiples usefull maths values and radian/degree conversions
 * vec2 : added methods : set, reset, compare, dot, closestPointOnLine, closestPointOnSegment,
 *                        projectionOnLine, lerp, angle
 * vec3 : added methods : set, reset, compare, dot, cross, closestPointOnLine, closestPointOnSegment,
 *                        projectionOnLine, lerp, angle
 * vec4 : added methods : set, reset, compare
 ***************************************************************************
 */
/*
/*
 * MathLibrary
 * Copyright (c) 2011 NoLimitsDesigns
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.

 * If there are any concerns or questions about the code, please e-mail smasherprog@gmail.com or visit www.nolimitsdesigns.com
 */

/*
 * Author: Scott Lee
 */
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

#ifndef _MATH_CLASSES_H_
#define _MATH_CLASSES_H_

#include "config.h"
#define USE_SIMD_HEADER
#define EPSILON				0.000001f
#ifndef M_PI
#define M_PI				3.141592653589793238462643383279f		//  PI
#endif

#define M_PIDIV2			1.570796326794896619231321691639f		//  PI / 2
#define M_2PI				6.283185307179586476925286766559f		//  2 * PI
#define M_PI2				9.869604401089358618834490999876f		//  PI ^ 2
#define M_PIDIV180			0.01745329251994329576923690768488f		//  PI / 180
#define M_180DIVPI			57.295779513082320876798154814105f		//  180 / PI

#define DegToRad(a)	(a)*=M_PIDIV180
#define RadToDeg(a)	(a)*=M_180DIVPI
#define RADIANS(a)	((a)*M_PIDIV180)
#define DEGREES(a)	((a)*M_180DIVPI)

#define kilometre    *1000
#define metre		 *1
#define decimetre    *0.1f
#define centimetre   *0.01f
#define millimeter   *0.001f

#define getMsToSec(a) a*0.001f
#define getSecToMs(a) a*1000.0f

#define MsToSec(a)   (a)*=0.001f
#define SecToMs(a)   (a)*=1000.0f

#if defined(USE_MATH_SIMD) && defined(USE_SIMD_HEADER)
	#include "MathSIMD.h"
#else
	#include "MathFPU.h"
#endif

#include "MathHelper.h"

const  F32 INV_RAND_MAX = 1.0 / (RAND_MAX + 1);
inline F32 random(F32 max=1.0)      { return max * rand() * INV_RAND_MAX; }
inline F32 random(F32 min, F32 max) { return min + (max - min) * INV_RAND_MAX * rand(); }
inline I32 random(I32 max=RAND_MAX) { return rand()%(max+1); }
inline bool bitCompare(U32 bitMask, U32 bit) {return ((bitMask & bit) == bit);}

template<class T>
class vec2;
template<class T>
class vec3;
template<class T>
class vec4;
template<class T>
class mat3;
template<class T>
class mat4;

/********************************************//**
/* vec2
/**********************************************/
template<class T>
class vec2 {
public:
	vec2() : x(0), y(0) { }
	vec2(T _x,T _y) : x(_x), y(_y) { }
	vec2(const T *_v) : x(_v[0]), y(_v[1]) { }
	vec2(const vec2 &_v) : x(_v.x), y(_v.y) { }
	vec2(const vec3<T> &_v);
	vec2(const vec4<T> &_v);

	I32 operator==(const vec2 &_v) { return (FLOAT_COMPARE(this->x,_v.x) && FLOAT_COMPARE(this->y, _v.y)); }
	I32 operator!=(const vec2 &_v) { return !(*this == _v); }

	vec2 &operator=(T _f)                      { this->x=_f; this->y=_f; return (*this); }
	const vec2 operator*(T _f)           const { return vec2(this->x * _f,this->y * _f); }
	const vec2 operator/(T _i)           const { return vec2(this->x / _i,this->y / _i); }
	const vec2 operator+(const vec2 &_v) const { return vec2(this->x + _v.x,this->y + _v.y); }
	const vec2 operator-()               const { return vec2(-this->x,-this->y); }
	const vec2 operator-(const vec2 &_v) const { return vec2(this->x - _v.x,this->y - _v.y); }

	vec2 &operator*=(T _f) { return *this = *this * _f; }
	vec2 &operator/=(T _f) { return *this = *this / _f; }
	vec2 &operator+=(const vec2 &_v) { return *this = *this + _v; }
	vec2 &operator-=(const vec2 &_v) { return *this = *this - _v; }

	T operator*(const vec2 &_v) const { return this->x * _v.x + this->y * _v.y; }

	operator T*()             { return this->v; }
	operator const T*() const { return this->v; }
//	T &operator[](I32 _i) { return this->v[_i]; }
//	const T &operator[](I32 _i) const { return this->v[_i]; }

	inline void set(T _x,T _y)          { this->x = _x; this->y = _y; }
	inline void set(const vec2& source) { this->x = source.x; this->y = source.y; }
	inline void reset()                 { this->x = this->y = 0; }

	inline T normalize() {
		T inv,l = this->length();
		if(l < EPSILON) return 0;
		inv = 1.0f / l;
		this->x *= inv;
		this->y *= inv;
		return l;
	}
	inline T length()           const { return square_root_tpl(this->x * this->x + this->y * this->y); }
	inline T dot(const vec2 &v) const { return ((this->x*v.x) + (this->y*v.y)); } //dot product

	inline bool compare(const vec2 &_v,F32 epsi=EPSILON) const { 
		return (FLOAT_COMPARE_TOLERANCE(this->x,_v.x,epsi) && 
				FLOAT_COMPARE_TOLERANCE(this->y, _v.y, epsi)); 
	}

	/// return the coordinates of the closest point from *this to the line determined by points vA and vB
	inline vec2 closestPointOnLine(const vec2 &vA, const vec2 &vB) const { return (((vB-vA) * this->projectionOnLine(vA, vB)) + vA); }
	/// return the coordinates of the closest point from *this to the segment determined by points vA and vB
	inline vec2 closestPointOnSegment(const vec2 &vA, const vec2 &vB) const {
		T factor = this->projectionOnLine(vA, vB);
		if (factor <= 0) return vA;
		if (factor >= 1) return vB;
		return (((vB-vA) * factor) + vA);
	}
	/// return the projection factor from *this to the line determined by points vA and vB
	inline T projectionOnLine(const vec2 &vA, const vec2 &vB) const {
		vec2 v(vB - vA);
		return v.dot(*this - vA) / v.dot(v);
	}
	/// linear interpolation between 2 vectors
	inline vec2 lerp(vec2 &u, vec2 &v, T factor)     const { return ((u * (1 - factor)) + (v * factor)); }
	inline vec2 lerp(vec2 &u, vec2 &v, vec2& factor) const { 
		return (vec2((u.x * (1 - factor.x)) + (v.x * factor.x), 
					 (u.y * (1 - factor.y)) + (v.y * factor.y))); 
	}

	inline T angle()              const { return (T)atan2(this->y,this->x); }
	inline T angle(const vec2 &v) const { return (T)atan2(v.y-this->y,v.x-this->x); }

	inline void swap(vec2 &iv) { T tmp=x; x=iv.x; iv.x=tmp; tmp=y; y=iv.y; iv.y=tmp; }
	inline void swap(vec2 *iv) { this->swap(*iv); }

	union {
		struct {T x,y;};
		struct {T s,t;};
		struct {T width,height;};
		struct {T min,max;};
		T v[2];
	};
};

template<class T>
inline vec2<T> operator*(T fl, const vec2<T>& v) { return vec2<T>(v.x*fl, v.y*fl);}
template<class T>
inline T Dot(const vec2<T>& a, const vec2<T>& b) { return(a.x*b.x+a.y*b.y); }

template <class T>
class vec3 {
public:
	vec3() : x(0), y(0), z(0) { }
	vec3(T _x,T _y,T _z) : x(_x), y(_y), z(_z) { }
	vec3(const T *_v) : x(_v[0]), y(_v[1]), z(_v[2]) { }
	vec3(const vec2<T> &_v,T _z) : x(_v.x), y(_v.y), z(_z) { }
	vec3(const vec3 &_v) : x(_v.x), y(_v.y), z(_v.z) { }
	vec3(const vec4<T> &_v);

	I32 operator!=(const vec3 &_v) { return !(*this == _v); }

	I32 operator==(const vec3 &_v) { 
		return FLOAT_COMPARE(this->x,_v.x) && 
		       FLOAT_COMPARE(this->y,_v.y) && 
			   FLOAT_COMPARE(this->z, _v.z); 
	}

	inline bool compare(const vec3 &_v,F32 epsi=EPSILON) {
		return FLOAT_COMPARE_TOLERANCE(this->x,_v.x,epsi) && 
		 	   FLOAT_COMPARE_TOLERANCE(this->y,_v.y,epsi) && 
			   FLOAT_COMPARE_TOLERANCE(this->z,_v.z,epsi); 
	}

	vec3 &operator=(T _f) { this->x=_f; this->y=_f; this->z=_f; return (*this); }
	const vec3 operator*(T _f) const { return vec3(this->x * _f,this->y * _f,this->z * _f); }
	const vec3 operator/(T _f) const {
		if(IS_ZERO(_f)) return *this;
		_f = 1.0f / _f;
		return (*this) * _f;
	}

	const vec3 operator+(const vec3 &_v) const { return vec3(this->x + _v.x,this->y + _v.y,this->z + _v.z); }
	const vec3 operator-() const { return vec3(-this->x,-this->y,-this->z); }
	const vec3 operator-(const vec3 &_v) const { return vec3(this->x - _v.x,this->y - _v.y,this->z - _v.z); }
    const vec3 operator*(const vec3 &_v) const { return vec3(this->x * _v.x,this->y * _v.y,this->z * _v.z);}
	vec3 &operator*=(T _f) { return *this = *this * _f; }
	vec3 &operator/=(T _f) { return *this = *this / _f; }
	vec3 &operator+=(const vec3 &_v) { return *this = *this + _v; }
	vec3 &operator-=(const vec3 &_v) { return *this = *this - _v; }

	//T operator*(const vec3 &_v) const { return this->x * _v.x + this->y * _v.y + this->z * _v.z; }

	T operator*(const vec4<T> &_v) const;
	//T operator[](const I32 i) {switch(i){case 0: return this->x; case 1: return this->y; case 2: return this->z;};}
	operator T*() { return this->v; }
	operator const T*() const { return this->v; }
    inline vec2<T> rg() const {return vec2<T>(this->r,this->g);}
    inline vec2<T> xy() const {return this->rg();}
    inline bool isUniform() const { return IS_ZERO(this->x - this->y) && IS_ZERO(this->y - this->z);}

	inline void set(T _x, T _y, T _z)   { this->x = _x; this->y = _y; this->z = _z; }
	inline void set(const vec3& source) { this->x = source.x; this->y = source.y; this->z = source.z; }
	inline void reset()                 { this->x = this->y = this->z = 0; }

	inline T length()        const {return square_root_tpl(lengthSquared()); }
	inline T lengthSquared() const {return this->x * this->x + this->y * this->y + this->z * this->z; }

	inline T normalize() {
		T inv,l = this->length();
		if(l < EPSILON) return 0;
		inv = 1.0f / l;
		this->x *= inv;
		this->y *= inv;
		this->z *= inv;
		return l;
	}

	inline void cross(const vec3 &v1,const vec3 &v2) {
		this->x = v1.y * v2.z - v1.z * v2.y;
		this->y = v1.z * v2.x - v1.x * v2.z;
		this->z = v1.x * v2.y - v1.y * v2.x;
	}

	inline void cross(const vec3 &v2) {
		T x = this->y * v2.z - this->z * v2.y;
		T y = this->z * v2.x - this->x * v2.z;
		this->z = this->x * v2.y - this->y * v2.x;
		this->y = y;
		this->x = x;
	}

	inline T dot(const vec3 &v) const { return ((this->x*v.x) + (this->y*v.y) + (this->z*v.z)); }

	inline T distance(const vec3 &_v) const {
		return square_root_tpl(((_v.x - this->x)*(_v.x - this->x)) + 
			                   ((_v.y - this->y)*(_v.y - this->y)) + 
							   ((_v.z - this->z)*(_v.z - this->z)));
	}

	/// Returns the angle in radians between '*this' and 'v'
	inline T angle(vec3<T> &v) const {
		T angle = (T)fabs(acos(this->dot(v)/(this->length()*v.length())));
		if(angle < EPSILON) return 0;
		return angle;
	}

	inline vec3 direction(const vec3& u) const {
		vec3 v(u.x - this->x, u.y - this->y, u.z-this->z);
		v.normalize();
		return v;
	}

	inline vec3 closestPointOnLine(const vec3 &vA, const vec3 &vB) const { 
		return (((vB-vA) * this->projectionOnLine(vA, vB)) + vA); 
	}

	inline vec3 closestPointOnSegment(const vec3 &vA, const vec3 &vB) const {
		T factor = this->projectionOnLine(vA, vB);
		if (factor <= 0.0f) return vA;
		if (factor >= 1.0f) return vB;
		return (((vB-vA) * factor) + vA);
	}

	inline T projectionOnLine(const vec3 &vA, const vec3 &vB) const {
		vec3 v(vB - vA);
		return v.dot(*this - vA) / v.dot(v);
	}

	inline vec3 lerp(vec3 &u, vec3 &v, T factor) const { return ((u * (1 - factor)) + (v * factor)); }
	inline vec3 lerp(vec3 &u, vec3 &v, vec3& factor) const {
		return (vec3((u.x * (1 - factor.x)) + (v.x * factor.x),
					 (u.y * (1 - factor.y)) + (v.y * factor.y),
					 (u.z * (1 - factor.z)) + (v.z * factor.z))); 
	}

	inline void rotateX (D32 radians){
	   this->y = (T)( cos(radians)*this->y + sin(radians)*this->z);
	   this->z = (T)(-sin(radians)*this->y + cos(radians)*this->z);
   }

	inline void rotateY (D32 radians){
	   this->x = (T)(cos(radians)*this->x - sin(radians)*this->z);
	   this->z = (T)(sin(radians)*this->x + cos(radians)*this->z);
   }

	inline void rotateZ (D32 radians){
	   this->x = (T)( cos(radians)*this->x + sin(radians)*this->y);
	   this->y = (T)(-sin(radians)*this->x; + cos(radians)*this->y);
   }

	union {
		struct {T x,y,z;};
		struct {T s,t,p;};
		struct {T r,g,b;};
		struct {T pitch,yaw,roll;};
		struct {T width,height,depth;};
		T v[3];
	};

	inline void swap(vec3 &iv) { T tmp=x; x=iv.x; iv.x=tmp; tmp=y; y=iv.y; iv.y=tmp; tmp=z; z=iv.z; iv.z=tmp; }
	inline void swap(vec3 *iv) { this->swap(*iv); }

	inline void  get(T * v) const {
		v[0] = (T)this->x;
		v[1] = (T)this->y;
		v[2] = (T)this->z;
	}
};

template<class T>
inline vec3<T> operator*(T fl, const vec3<T>& v)	{ return vec3<T>(v.x*fl, v.y*fl, v.z*fl);}

template<class T>
inline T Dot(const vec3<T>& a, const vec3<T>& b) { return(a.x*b.x+a.y*b.y+a.z*b.z); }
template<class T>
inline vec3<T> Cross(const vec3<T> &v1, const vec3<T> &v2) {
	return vec3<T>(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}
template<class T>
inline vec2<T>::vec2(const vec3<T> &_v) {
	this->x = _v.x;
	this->y = _v.y;
}

/// This calculates a vector between 2 points and returns the result
template<class T>
inline vec3<T>& vector(const vec3<T> &vp1, const vec3<T> &vp2) {
	vec3<T> *ret = new vec3<T>(vp1.x - vp2.x, vp1.y - vp2.y, vp1.z - vp2.z);
	return *ret;
}

template<class T>
inline vec3<T>::vec3(const vec4<T> &_v) {
	this->x = _v.x;
	this->y = _v.y;
	this->z = _v.z;
}

/********************//**
/* vec4
/**********************/

template <class T>
class vec4 {
public:
	vec4() : x(0), y(0), z(0), w(1) { }
	vec4(T _x,T _y,T _z,T _w) : x(_x), y(_y), z(_z), w(_w) { }
	vec4(const T *_v) : x(_v[0]), y(_v[1]), z(_v[2]), w(_v[3]) { }
	vec4(const vec3<T> &_v) : x(_v.x), y(_v.y), z(_v.z), w(1) { }
	vec4(const vec3<T> &_v,T _w) : x(_v.x), y(_v.y), z(_v.z), w(_w) { }
	vec4(const vec4 &_v) : x(_v.x), y(_v.y), z(_v.z), w(_v.w) { }

	///should be comparable with the FLOAT_COMPARE case
	I32 operator==(const vec4 &_v) { return IS_ZERO(this->x - _v.x) && IS_ZERO(this->y - _v.y) &&
                                            IS_ZERO(this->z - _v.z) && IS_ZERO(this->w - _v.w); }
	I32 operator!=(const vec4 &_v) { return !(*this == _v); }

	vec4 &operator=(T _f) { this->x=_f; this->y=_f; this->z=_f; this->w=_f; return (*this);}
	const vec4 operator*(T _f) const { return vec4(this->x * _f,this->y * _f,this->z * _f,this->w * _f); }
	const vec4 operator/(T _f) const {
		if(IS_ZERO(_f)) return *this;
		_f = 1.0f / _f;
		return (*this) * _f;
	}
	const vec4 operator+(const vec4 &_v) const { return vec4(this->x + _v.x,this->y + _v.y,this->z + _v.z,this->w + _v.w); }
	const vec4 operator-() const { return vec4(-x,-y,-z,-w); }
	const vec4 operator-(const vec4 &_v) const { return vec4(this->x - _v.x,this->y - _v.y,this->z - _v.z,this->w - _v.w); }

	vec4 &operator*=(T _f)           { return *this = *this * _f; }
	vec4 &operator/=(T _f)           { return *this = *this / _f; }
	vec4 &operator+=(const vec4 &_v) { return *this = *this + _v; }
	vec4 &operator-=(const vec4 &_v) { return *this = *this - _v; }

	T operator*(const vec3<T> &_v) const { return this->x * _v.x + this->y * _v.y + this->z * _v.z + this->w; }
	T operator*(const vec4<T> &_v) const { return this->x * _v.x + this->y * _v.y + this->z * _v.z + this->w * _v.w; }

	operator T*() { return this->v; }
	operator const T*() const { return this->v; }
//	T &operator[](I32 _i) { return this->v[_i]; }
//	const T &operator[](I32 _i) const { return this->v[_i]; }

    inline vec2<T> rg()  const {return vec2<T>(this->r,this->g);}
    inline vec2<T> xy()  const {return this->rg();}
    inline vec3<T> rgb() const {return vec3<T>(this->r,this->g,this->b);}
    inline vec3<T> xyz() const {return this->rgb();}

	inline void set(T _x,T _y,T _z,T _w) { this->x = _x;       this->y =_y;        this->z =_z;        this->w =_w;}
	inline void set(const vec4& source)  { this->x = source.x; this->y = source.y; this->z = source.z; this->w = source.w;}

	inline void reset() { this->x = this->y = this->z = this->w = 0;}

	inline bool compare(const vec4 &_v,F32 epsi=EPSILON) { 
		return (FLOAT_COMPARE_TOLERANCE(this->x, _v.x, epsi) && 
				FLOAT_COMPARE_TOLERANCE(this->y, _v.y, epsi) && 
				FLOAT_COMPARE_TOLERANCE(this->z, _v.z, epsi) && 
				FLOAT_COMPARE_TOLERANCE(this->w, _v.w, epsi)); }

	inline vec4 lerp(const vec4 &u, const vec4 &v, T factor) const { return ((u * (1 - factor)) + (v * factor)); }

	inline void swap(vec4 &iv) { T tmp=x; x=iv.x; iv.x=tmp; tmp=y; y=iv.y; iv.y=tmp; tmp=z; z=iv.z; iv.z=tmp; tmp=w; w=iv.w; iv.w=tmp;}
	inline void swap(vec4 *iv) { this->swap(*iv); }

	union {
		struct {T x,y,z,w;};
		struct {T s,t,p,q;};
		struct {T r,g,b,a;};
		struct {T fov,ratio,znear,zfar;};
		struct {T width,height,depth,key;};
		T v[4];
	};
};

template<class T>
inline vec4<T> operator*(T fl, const vec4<T>& v)	{ return vec4<T>(v.x*fl, v.y*fl, v.z*fl,  v.w*fl);}

template<class T>
inline vec2<T>::vec2(const vec4<T> &_v) {
	this->x = _v.x;
	this->y = _v.y;
}

/******************************//**
/* mat3
/*********************************/
template<class T>
class mat3 {
public:
	mat3() {
		mat[0] = 1.0; mat[3] = 0.0; mat[6] = 0.0;
		mat[1] = 0.0; mat[4] = 1.0; mat[7] = 0.0;
		mat[2] = 0.0; mat[5] = 0.0; mat[8] = 1.0;
	}
	mat3(T m0, T m1, T m2,
		 T m3, T m4, T m5,
		 T m6, T m7, T m8) {
		mat[0] = m0; mat[3] = m3; mat[6] = m6;
		mat[1] = m1; mat[4] = m4; mat[7] = m7;
		mat[2] = m2; mat[5] = m5; mat[8] = m8;
	}
	mat3(const T *m) {
		mat[0] = m[0]; mat[3] = m[3]; mat[6] = m[6];
		mat[1] = m[1]; mat[4] = m[4]; mat[7] = m[7];
		mat[2] = m[2]; mat[5] = m[5]; mat[8] = m[8];
	}
	mat3(const mat3 &m) {
		mat[0] = m[0]; mat[3] = m[3]; mat[6] = m[6];
		mat[1] = m[1]; mat[4] = m[4]; mat[7] = m[7];
		mat[2] = m[2]; mat[5] = m[5]; mat[8] = m[8];
	}
	mat3(const mat4<T> &m);

	vec3<T> operator*(const vec3<T> &v) const {
		return vec3<T>(mat[0] * v[0] + mat[3] * v[1] + mat[6] * v[2],
					   mat[1] * v[0] + mat[4] * v[1] + mat[7] * v[2],
					   mat[2] * v[0] + mat[5] * v[1] + mat[8] * v[2]);
	}

	vec4<T> operator*(const vec4<T> &v) const {
		return vec4<T>(mat[0] * v[0] + mat[3] * v[1] + mat[6] * v[2],
					   mat[1] * v[0] + mat[4] * v[1] + mat[7] * v[2],
					   mat[2] * v[0] + mat[5] * v[1] + mat[8] * v[2],
					   v[3]);
	}
	mat3 operator*(T f) const {
		return mat3(mat[0] * f, mat[1] * f, mat[2] * f,
					mat[3] * f,	mat[4] * f,	mat[5] * f,
					mat[6] * f,	mat[7] * f,	mat[8] * f);
	}
	mat3 operator*(const mat3 &m) const {
		return mat3(mat[0] * m[0] + mat[3] * m[1] + mat[6] * m[2],
					mat[1] * m[0] + mat[4] * m[1] + mat[7] * m[2],
					mat[2] * m[0] + mat[5] * m[1] + mat[8] * m[2],
					mat[0] * m[3] + mat[3] * m[4] + mat[6] * m[5],
					mat[1] * m[3] + mat[4] * m[4] + mat[7] * m[5],
					mat[2] * m[3] + mat[5] * m[4] + mat[8] * m[5],
					mat[0] * m[6] + mat[3] * m[7] + mat[6] * m[8],
					mat[1] * m[6] + mat[4] * m[7] + mat[7] * m[8],
					mat[2] * m[6] + mat[5] * m[7] + mat[8] * m[8]);
	}
	mat3 operator+(const mat3 &m) const {
		return mat3(mat[0] + m[0], mat[1] + m[1], mat[2] + m[2],
					mat[3] + m[3], mat[4] + m[4], mat[5] + m[5],
					mat[6] + m[6], mat[7] + m[7], mat[8] + m[8]);
	}
	mat3 operator-(const mat3 &m) const {
		return mat3(mat[0] - m[0], mat[1] - m[1], mat[2] - m[2],
					mat[3] - m[3], mat[4] - m[4], mat[5] - m[5],
					mat[6] - m[6], mat[7] - m[7], mat[8] - m[8]);
	}

	mat3 &operator*=(T f) { return *this = *this * f; }
	mat3 &operator*=(const mat3 &m) { return *this = *this * m; }
	mat3 &operator+=(const mat3 &m) { return *this = *this + m; }
	mat3 &operator-=(const mat3 &m) { return *this = *this - m; }

	bool operator == (mat3& B){
        for (I32 i = 0; i < 9; i++){
            if (!FLOAT_COMPARE(m[i],B[i])) return false;
        }
        return true;
    }

    bool operator != (mat3& B){ return !(*this == B);}

	operator T*()             { return mat; }
	operator const T*() const { return mat; }

	T &operator[](I8 i)             { return mat[i]; }
	const T operator[](I32 i) const { return mat[i]; }

	inline mat3 transpose() const {
		return mat3(mat[0], mat[3], mat[6],
					mat[1], mat[4], mat[2],
					mat[7], mat[5], mat[8]);
	}

	inline T det() const {
		return ((mat[0] * mat[4] * mat[8]) +
				(mat[3] * mat[7] * mat[2]) +
				(mat[6] * mat[1] * mat[5]) -
				(mat[6] * mat[4] * mat[2]) -
				(mat[3] * mat[1] * mat[8]) -
				(mat[0] * mat[7] * mat[5]));
	}

	inline mat3 inverse() const {
		T idet = 1.0f / det();
		return mat3((mat[4] * mat[8] - mat[7] * mat[5]) * idet,
					-(mat[1] * mat[8] - mat[7] * mat[2]) * idet,
					 (mat[1] * mat[5] - mat[4] * mat[2]) * idet,
					-(mat[3] * mat[8] - mat[6] * mat[5]) * idet,
					 (mat[0] * mat[8] - mat[6] * mat[2]) * idet,
					-(mat[0] * mat[5] - mat[3] * mat[2]) * idet,
					 (mat[3] * mat[7] - mat[6] * mat[4]) * idet,
					-(mat[0] * mat[7] - mat[6] * mat[1]) * idet,
					 (mat[0] * mat[4] - mat[3] * mat[1]) * idet);
	}

	inline void zero() {
		mat[0] = 0.0; mat[3] = 0.0; mat[6] = 0.0;
		mat[1] = 0.0; mat[4] = 0.0; mat[7] = 0.0;
		mat[2] = 0.0; mat[5] = 0.0; mat[8] = 0.0;
	}

	inline void identity() {
		mat[0] = 1.0; mat[3] = 0.0; mat[6] = 0.0;
		mat[1] = 0.0; mat[4] = 1.0; mat[7] = 0.0;
		mat[2] = 0.0; mat[5] = 0.0; mat[8] = 1.0;
	}

    inline bool isIdentity() const{
           return  (FLOAT_COMPARE(mat[0],1.0) && IS_ZERO(mat[1])           && IS_ZERO(mat[2])            && IS_ZERO(mat[3])  &&
                    IS_ZERO(mat[4])           && FLOAT_COMPARE(mat[5],1.0) && IS_ZERO(mat[6])            && IS_ZERO(mat[7])  &&
                    IS_ZERO(mat[8])           && IS_ZERO(mat[9])           && FLOAT_COMPARE(mat[10],1.0) && IS_ZERO(mat[11]) &&
                    IS_ZERO(mat[12])          && IS_ZERO(mat[13])          && IS_ZERO(mat[14])           && FLOAT_COMPARE(mat[15],1.0));
    }

	inline void rotate(const vec3<T> &v,T angle) {
		rotate(v.x,v.y,v.z,angle);
	}

	void rotate(T x,T y,T z,T angle) {
		DegToRad(angle);
		T c = (T)cos(angle);
		T s = (T)sin(angle);
		T l = Util::square_root_tpl(x * x + y * y + z * z);
		if(l < EPSILON) l = 1;
		else l = 1 / l;
		x *= l;
		y *= l;
		z *= l;
		T xy = x * y;
		T yz = y * z;
		T zx = z * x;
		T xs = x * s;
		T ys = y * s;
		T zs = z * s;
		T c1 = 1 - c;
		mat[0] = c1 * x * x + c;	mat[3] = c1 * xy - zs;		mat[6] = c1 * zx + ys;
		mat[1] = c1 * xy + zs;		mat[4] = c1 * y * y + c;	mat[7] = c1 * yz - xs;
		mat[2] = c1 * zx - ys;		mat[5] = c1 * yz + xs;		mat[8] = c1 * z * z + c;
	}

	inline void rotate_x(T angle) {
		DegToRad(angle);
		T c = (T)cos(angle);
		T s = (T)sin(angle);
		mat[0] = 1; mat[3] = 0; mat[6] = 0;
		mat[1] = 0; mat[4] = c; mat[7] = -s;
		mat[2] = 0; mat[5] = s; mat[8] = c;
	}

	inline void rotate_y(T angle) {
		DegToRad(angle);
		T c = (T)cos(angle);
		T s = (T)sin(angle);
		mat[0] = c;  mat[3] = 0; mat[6] = s;
		mat[1] = 0;  mat[4] = 1; mat[7] = 0;
		mat[2] = -s; mat[5] = 0; mat[8] = c;
	}

	inline void rotate_z(T angle) {
		DegToRad(angle);
		T c = (T)cos(angle);
		T s = (T)sin(angle);
		mat[0] = c; mat[3] = -s; mat[6] = 0;
		mat[1] = s; mat[4] = c;  mat[7] = 0;
		mat[2] = 0; mat[5] = 0;  mat[8] = 1;
	}

	inline void scale(T x,T y,T z) {
		mat[0] = x; mat[3] = 0; mat[6] = 0;
		mat[1] = 0; mat[4] = y; mat[7] = 0;
		mat[2] = 0; mat[5] = 0; mat[8] = z;
	}

	inline void scale(const vec3<T> &v) {
		scale(v.x,v.y,v.z);
	}

	void orthonormalize(void) {
		vec3<T> x(mat[0],mat[1],mat[2]);
		vec3<T> y(mat[3],mat[4],mat[5]);
		vec3<T> z;
		x.normalize();
		z.cross(x,y);
		z.normalize();
		y.cross(z,x);
		y.normalize();
		mat[0] = x.x; mat[3] = y.x; mat[6] = z.x;
		mat[1] = x.y; mat[4] = y.y; mat[7] = z.y;
		mat[2] = x.z; mat[5] = y.z; mat[8] = z.z;
	}

	inline void swap(mat3& B){
        for(U8 i = 0; i < 3; i++){
		    std::swap(m[i][0], B.m[i][0]);
		    std::swap(m[i][1], B.m[i][1]);
		    std::swap(m[i][2], B.m[i][2]);
        }
	}

    union{
	    T mat[9];
        T m[3][3];
    };
};

/*************//**
/* mat4
/***************/
template<class T>
class mat4 {
public:
	mat4()/* : mat(NULL)*/{
		allocateMem();
		this->mat[0] = 1.0; this->mat[4] = 0.0; this->mat[8]  = 0.0; this->mat[12] = 0.0;
		this->mat[1] = 0.0; this->mat[5] = 1.0; this->mat[9]  = 0.0; this->mat[13] = 0.0;
		this->mat[2] = 0.0; this->mat[6] = 0.0; this->mat[10] = 1.0; this->mat[14] = 0.0;
		this->mat[3] = 0.0; this->mat[7] = 0.0; this->mat[11] = 0.0; this->mat[15] = 1.0;
	}

	mat4(T m0, T m1, T m2, T m3,
		 T m4, T m5, T m6, T m7,
		 T m8, T m9, T m10,T m11,
		 T m12,T m13,T m14,T m15)/* : mat(NULL)*/{
		allocateMem();
		this->mat[0] = m0; this->mat[4] = m4; this->mat[8]  = m8;  this->mat[12] = m12;
		this->mat[1] = m1; this->mat[5] = m5; this->mat[9]  = m9;  this->mat[13] = m13;
		this->mat[2] = m2; this->mat[6] = m6; this->mat[10] = m10; this->mat[14] = m14;
		this->mat[3] = m3; this->mat[7] = m7; this->mat[11] = m11; this->mat[15] = m15;
	}

	mat4(const vec4<T> &col1,const vec4<T> &col2,const vec4<T> &col3,const vec4<T> &col4)/* : mat(NULL)*/{
		allocateMem();
		this->mat[0] = col1.x; this->mat[4] = col2.x; this->mat[8]  = col3.x; this->mat[12] = col4.x;
		this->mat[1] = col1.y; this->mat[5] = col2.y; this->mat[9]  = col3.y; this->mat[13] = col4.y;
		this->mat[2] = col1.z; this->mat[6] = col2.z; this->mat[10] = col3.z; this->mat[14] = col4.z;
		this->mat[3] = col1.w; this->mat[7] = col2.w; this->mat[11] = col3.w; this->mat[15] = col4.w;
	}

	mat4(const vec3<T> &v)/* : mat(NULL)*/{
		allocateMem();
		translate(v);
	}

	mat4(T x,T y,T z)/* : mat(NULL)*/{
		allocateMem();
		translate(x,y,z);
	}

	mat4(const vec3<T> &axis,T angle)/* : mat(NULL)*/{
		allocateMem();
		rotate(axis,angle);
	}

	mat4(T x,T y,T z,T angle)/* : mat(NULL)*/{
		allocateMem();
		rotate(x,y,z,angle);
	}

	mat4(const mat3<T> &m)/* : mat(NULL)*/{
		allocateMem();
		this->mat[0] = m[0]; this->mat[4] = m[3]; this->mat[8]  = m[6]; this->mat[12] = 0.0;
		this->mat[1] = m[1]; this->mat[5] = m[4]; this->mat[9]  = m[7]; this->mat[13] = 0.0;
		this->mat[2] = m[2]; this->mat[6] = m[5]; this->mat[10] = m[8]; this->mat[14] = 0.0;
		this->mat[3] = 0.0;  this->mat[7] = 0.0;  this->mat[11] = 0.0;  this->mat[15] = 1.0;
	}

	mat4(const T *m)/* : mat(NULL)*/{
		allocateMem();
		this->mat[0] = m[0]; this->mat[4] = m[4]; this->mat[8]  = m[8];  this->mat[12] = m[12];
		this->mat[1] = m[1]; this->mat[5] = m[5]; this->mat[9]  = m[9];  this->mat[13] = m[13];
		this->mat[2] = m[2]; this->mat[6] = m[6]; this->mat[10] = m[10]; this->mat[14] = m[14];
		this->mat[3] = m[3]; this->mat[7] = m[7]; this->mat[11] = m[11]; this->mat[15] = m[15];
	}

	mat4(const mat4 &m)/* : mat(NULL)*/{
		allocateMem();
		this->mat[0] = m[0]; this->mat[4] = m[4]; this->mat[8]  = m[8];  this->mat[12] = m[12];
		this->mat[1] = m[1]; this->mat[5] = m[5]; this->mat[9]  = m[9];  this->mat[13] = m[13];
		this->mat[2] = m[2]; this->mat[6] = m[6]; this->mat[10] = m[10]; this->mat[14] = m[14];
		this->mat[3] = m[3]; this->mat[7] = m[7]; this->mat[11] = m[11]; this->mat[15] = m[15];
	}

	vec3<T> operator*(const vec3<T> &v) const {
		return vec3<T>(this->mat[0] * v[0] + this->mat[4] * v[1] + this->mat[8]  * v[2] + this->mat[12],
					   this->mat[1] * v[0] + this->mat[5] * v[1] + this->mat[9]  * v[2] + this->mat[13],
					   this->mat[2] * v[0] + this->mat[6] * v[1] + this->mat[10] * v[2] + this->mat[14]);
	}

	vec4<T> operator*(const vec4<T> &v) const {
		return vec4<T>(this->mat[0] * v[0] + this->mat[4] * v[1] + this->mat[8]  * v[2] + this->mat[12] * v[3],
					   this->mat[1] * v[0] + this->mat[5] * v[1] + this->mat[9]  * v[2] + this->mat[13] * v[3],
					   this->mat[2] * v[0] + this->mat[6] * v[1] + this->mat[10] * v[2] + this->mat[14] * v[3],
					   this->mat[3] * v[0] + this->mat[7] * v[1] + this->mat[11] * v[2] + this->mat[15] * v[3]);
	}

	mat4<T> operator*(T f) const {
		return mat4<T>(this->mat[0]  * f, this->mat[1]  * f, this->mat[2]  * f, this->mat[3]  * f,
					   this->mat[4]  * f, this->mat[5]  * f, this->mat[6]  * f, this->mat[7]  * f,
					   this->mat[8]  * f, this->mat[9]  * f, this->mat[10] * f, this->mat[11] * f,
					   this->mat[12] * f, this->mat[13] * f, this->mat[14] * f, this->mat[15] * f);
	}

	mat4<T> operator*(const mat4<T> &m) const {
		mat4<T> ret;
		Mat4::Multiply(this->mat,m.mat,ret.mat);
		return ret;
	}

    bool operator == (mat4& B){
        for (I32 i = 0; i < 16; i++){
            if (!FLOAT_COMPARE(mat[i],B[i])) return false;
        }
        return true;
    }

    bool operator != (mat4& B){ return !(*this == B);}

	inline vec4<T> getCol(I32 index) const {
		return vec4<T>(this->mat[0 + (index*4)],
					   this->mat[1 + (index*4)],
					   this->mat[2 + (index*4)],
					   this->mat[3 + (index*4)]);
		/*return vec4<T>(this->m[0][index],
                         this->m[1][index],
                         this->m[2][index],
                         this->m[3][index]);*/
	}

	inline void setCol(I32 index, const vec4<T>& value){
		this->mat[0 + (index*4)] = value.x;
		this->mat[1 + (index*4)] = value.y;
		this->mat[2 + (index*4)] = value.z;
		this->mat[3 + (index*4)] = value.w;
		/*this->m[0][index] = value.x;
		this->m[1][index] = value.y;
		this->m[2][index] = value.z;
		this->m[3][index] = value.w;*/
	}

	/// premultiply the matrix by the given matrix
	void multmatrix(const mat4& m) {
		T tmp[4];
		for (I8 j=0; j<4; j++) {
			tmp[0] = mat[j];
			tmp[1] = mat[4+j];
			tmp[2] = mat[8+j];
			tmp[3] = mat[12+j];
			for (I8 i=0; i<4; i++) {
				mat[4*i+j] = m[4*i]*tmp[0] + m[4*i+1]*tmp[1] +
				m[4*i+2]*tmp[2] + m[4*i+3]*tmp[3];
			}
		}
	}

	mat4 operator+(const mat4 &m) const {
		return mat4(this->mat[0]  + m[0],  this->mat[1]  + m[1],  this->mat[2]  + m[2],  this->mat[3]  + m[3],
					this->mat[4]  + m[4],  this->mat[5]  + m[5],  this->mat[6]  + m[6],  this->mat[7]  + m[7],
					this->mat[8]  + m[8],  this->mat[9]  + m[9],  this->mat[10] + m[10], this->mat[11] + m[11],
					this->mat[12] + m[12], this->mat[13] + m[13], this->mat[14] + m[14], this->mat[15] + m[15]);
	}

	mat4 operator-(const mat4 &m) const {
		return mat4(this->mat[0]  - m[0],  this->mat[1]  - m[1],  this->mat[2]  - m[2],  this->mat[3]  - m[3],
					this->mat[4]  - m[4],  this->mat[5]  - m[5],  this->mat[6]  - m[6],  this->mat[7]  - m[7],
					this->mat[8]  - m[8],  this->mat[9]  - m[9],  this->mat[10] - m[10], this->mat[11] - m[11],
					this->mat[12] - m[12], this->mat[13] - m[13], this->mat[14] - m[14], this->mat[15] - m[15]);
	}

	inline T &element(I8 row, I8 column, bool rowMajor = false)	{
		//return (rowMajor ? this->m[column][row] : this->m[row][column]);
		return (rowMajor ? this->mat[column+row*4] : this->mat[row+column*4]);
	}

	inline const T &element(I8 row, I8 column, bool rowMajor = false) const {
		//return (rowMajor ? this->m[column][row] : this->m[row][column]);
		return (rowMajor ? this->mat[column+row*4] : this->mat[row+column*4]);
	}

	mat4 &operator*=(T f)           { return *this = *this * f; }
	mat4 &operator*=(const mat4 &m) { return *this = *this * m; }
	mat4 &operator+=(const mat4 &m) { return *this = *this + m; }
	mat4 &operator-=(const mat4 &m) { return *this = *this - m; }

	operator T*()             { return this->mat; }
	operator const T*() const { return this->mat; }

	T &operator[](I32 i)             { return this->mat[i]; }
	const T &operator[](I32 i) const { return this->mat[i]; }

	inline mat4 rotation(void) const {
		return mat4(this->mat[0], this->mat[1], this->mat[2], 0,
					this->mat[4], this->mat[5], this->mat[6], 0,
					this->mat[8], this->mat[9], this->mat[10],0,
					0, 0, 0, 1);
	}

	inline void transpose(mat4& out) const {
		out = mat4(this->mat[0], this->mat[4], this->mat[8],  this->mat[12],
				   this->mat[1], this->mat[5], this->mat[9],  this->mat[13],
				   this->mat[2], this->mat[6], this->mat[10], this->mat[14],
				   this->mat[3], this->mat[7], this->mat[11], this->mat[15]);
	}

	inline mat4 transpose() const {
		return mat4(this->mat[0], this->mat[4], this->mat[8],  this->mat[12],
					this->mat[1], this->mat[5], this->mat[9],  this->mat[13],
					this->mat[2], this->mat[6], this->mat[10], this->mat[14],
					this->mat[3], this->mat[7], this->mat[11], this->mat[15]);
	}

	inline mat4 transpose_rotation() const {
		return mat4(this->mat[0],  this->mat[4],  this->mat[8],  this->mat[3],
					this->mat[1],  this->mat[5],  this->mat[9],  this->mat[7],
					this->mat[2],  this->mat[6],  this->mat[10], this->mat[11],
					this->mat[12], this->mat[13], this->mat[14], this->mat[15]);
	}

	inline T det(void)             const { return Mat4::det(this->mat); }
	inline void inverse(mat4& ret) const { Mat4::Inverse(this->mat,ret.mat); }

	inline void zero() {
		this->mat[0] = 0.0; this->mat[4] = 0.0; this->mat[8] = 0.0; this->mat[12] = 0.0;
		this->mat[1] = 0.0; this->mat[5] = 0.0; this->mat[9] = 0.0; this->mat[13] = 0.0;
		this->mat[2] = 0.0; this->mat[6] = 0.0; this->mat[10] = 0.0; this->mat[14] = 0.0;
		this->mat[3] = 0.0; this->mat[7] = 0.0; this->mat[11] = 0.0; this->mat[15] = 0.0;
	}

	inline void identity() {
		this->mat[0] = 1.0; this->mat[4] = 0.0; this->mat[8]  = 0.0; this->mat[12] = 0.0;
		this->mat[1] = 0.0; this->mat[5] = 1.0; this->mat[9]  = 0.0; this->mat[13] = 0.0;
		this->mat[2] = 0.0; this->mat[6] = 0.0; this->mat[10] = 1.0; this->mat[14] = 0.0;
		this->mat[3] = 0.0; this->mat[7] = 0.0; this->mat[11] = 0.0; this->mat[15] = 1.0;
	}

	void rotate(const vec3<T> &axis,T angle) {
		DegToRad(angle);
		T c = (T)cos(angle);
		T s = (T)sin(angle);
		vec3<T> v = axis;
		v.normalize();
		T xx = v.x * v.x;
		T yy = v.y * v.y;
		T zz = v.z * v.z;
		T xy = v.x * v.y;
		T yz = v.y * v.z;
		T zx = v.z * v.x;
		T xs = v.x * s;
		T ys = v.y * s;
		T zs = v.z * s;
		mat[0] = (1 - c) * xx + c;  mat[4] = (1 - c) * xy - zs; mat[8]  = (1 - c) * zx + ys;
		mat[1] = (1 - c) * xy + zs; mat[5] = (1 - c) * yy + c;  mat[9]  = (1 - c) * yz - xs;
		mat[2] = (1 - c) * zx - ys; mat[6] = (1 - c) * yz + xs; mat[10] = (1 - c) * zz + c;
	}

	inline void rotate(T x,T y,T z,T angle) {
		rotate(vec3<T>(x,y,z),angle);
	}

	inline void rotate_x(T angle) {
		DegToRad(angle);
		T c = (T)cos(angle);
		T s = (T)sin(angle);
		mat[5] =  c;  mat[9]  = -s;
		mat[6] =  s;  mat[10] =  c;
	}

	inline void rotate_y(T angle) {
		DegToRad(angle);
		T c = (T)cos(angle);
		T s = (T)sin(angle);
		mat[0] =  c; mat[8]  =  s;
		mat[2] = -s; mat[10] =  c;
	}

	inline void rotate_z(T angle) {
		DegToRad(angle);
		T c = (T)cos(angle);
		T s = (T)sin(angle);
		mat[0] =  c;  mat[4] = -s;
		mat[1] =  s;  mat[5] =  c;
	}

	inline void scale(const vec3<T> &v) {
		mat[0] = v.x;
	    mat[5] = v.y;
		mat[10] = v.z;
	}

	inline void scale(T x,T y,T z) {
		scale(vec3<T>(x,y,z));
	}

	inline void translate(const vec3<T> &v) {
		this->mat[12] = v.x;
		this->mat[13] = v.y;
		this->mat[14] = v.z;
	}

	inline void translate(T x,T y,T z) {
		translate(vec3<T>(x,y,z));
	}

	void reflect(const vec4<T> &plane) {
		T x = plane.x;
		T y = plane.y;
		T z = plane.z;
		T x2 = x * 2.0f;
		T y2 = y * 2.0f;
		T z2 = z * 2.0f;
		this->mat[0] = 1 - x * x2; this->mat[4] = -y * x2;    this->mat[8] = -z * x2;      this->mat[12] = -plane.w * x2;
		this->mat[1] = -x * y2;    this->mat[5] = 1 - y * y2; this->mat[9] = -z * y2;      this->mat[13] = -plane.w * y2;
		this->mat[2] = -x * z2;    this->mat[6] = -y * z2;    this->mat[10] = 1 - z * z2;  this->mat[14] = -plane.w * z2;
		this->mat[3] = 0;          this->mat[7] = 0;          this->mat[11] = 0;           this->mat[15] = 1;
	}

	inline void reflect(T x,T y,T z,T w) {
		reflect(vec4<T>(x,y,z,w));
	}

	inline void perspective(T fov,T aspect,T znear,T zfar) {
		T y = (T)tan(fov * M_PI / 360.0f);
		T x = y * aspect;
		this->mat[0] = 1 / x; this->mat[4] = 0;     this->mat[8]  = 0;                                this->mat[12] = 0;
		this->mat[1] = 0;     this->mat[5] = 1 / y; this->mat[9]  = 0;                                this->mat[13] = 0;
		this->mat[2] = 0;     this->mat[6] = 0;     this->mat[10] = -(zfar + znear) / (zfar - znear); this->mat[14] = -(2 * zfar * znear) / (zfar - znear);
		this->mat[3] = 0;     this->mat[7] = 0;     this->mat[11] = -1;                               this->mat[15] = 0;
	}

	inline void look_at(const vec3<T> &eye,const vec3<T> &dir,const vec3<T> &up) {
		vec3<T> x,y,z;
		mat4<T> m0,m1;
		z = eye - dir;
		z.normalize();
		x.cross(up,z);
		x.normalize();
		y.cross(z,x);
		y.normalize();
		m0[0] = x.x; m0[4] = x.y; m0[8] = x.z; m0[12] = 0.0;
		m0[1] = y.x; m0[5] = y.y; m0[9] = y.z; m0[13] = 0.0;
		m0[2] = z.x; m0[6] = z.y; m0[10] = z.z; m0[14] = 0.0;
		m0[3] = 0.0; m0[7] = 0.0; m0[11] = 0.0; m0[15] = 1.0;
		m1.translate(-eye);
		*this = m0 * m1;
	}

	inline void look_at(const T *eye,const T *dir,const T *up) {
		look_at(vec3<T>(eye),vec3<T>(dir),vec3<T>(up));
	}

    inline void extractMat3(mat3<T>& matrix3) const {
        matrix3.m[0][0] = m[0][0];
        matrix3.m[0][1] = m[0][1];
        matrix3.m[0][2] = m[0][2];
        matrix3.m[1][0] = m[1][0];
        matrix3.m[1][1] = m[1][1];
        matrix3.m[1][2] = m[1][2];
        matrix3.m[2][0] = m[2][0];
        matrix3.m[2][1] = m[2][1];
        matrix3.m[2][2] = m[2][2];
    }

    inline void swap(mat4& B){
        for(U8 i = 0; i < 4; i++){
		    std::swap(m[i][0], B.m[i][0]);
		    std::swap(m[i][1], B.m[i][1]);
		    std::swap(m[i][2], B.m[i][2]);
            std::swap(m[i][3], B.m[i][3]);
        }
	}

///It's hard to allow 16-bytes alignment here to use with SIMD instructions when all the values can
///be changed from anywhere in the code by reference / pointer. Use the slower unaligned loader instead for SSE
#ifdef USE_MATH_SIMD
#define DECL_ALIGN __declspec(align(ALIGNED_BYTES))
#define NEW_ALIGN(X,Y) (X*)malloc_simd(Y * sizeof(X))
#define DEL_ALIGN(X) free_simd(X)
#else
#define DECL_ALIGN
#define NEW_ALIGN(X,Y) New X[Y]
#define DEL_ALIGN(X) SAFE_DELETE_ARRAY(X)
#endif

     union {
	    DECL_ALIGN  T mat[16];/*T* mat;*/
        DECL_ALIGN  T m[4][4];/*T* mat;*/
     };
/*	~mat4()
	{
		DEL_ALIGN(mat);
	}
*/
	inline void allocateMem() { /*if(mat != NULL) DEL_ALIGN(mat); mat = NEW_ALIGN(T,16);*/}
};

template<class T>
inline mat3<T>::mat3(const mat4<T> &m) {
	this->mat[0] = m[0]; this->mat[3] = m[4]; this->mat[6] = m[8];
	this->mat[1] = m[1]; this->mat[4] = m[5]; this->mat[7] = m[9];
	this->mat[2] = m[2]; this->mat[5] = m[6]; this->mat[8] = m[10];
}

/// Converts a point from world coordinates to projection coordinates
///(from Y = depth, Z = up to Y = up, Z = depth)
template <class T>
inline void projectPoint(const vec3<T>& position,vec3<T>& output){
	output.x = position.x;
	output.y = position.z;
	output.z = position.y;
}

#endif
