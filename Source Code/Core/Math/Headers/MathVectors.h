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
 * Author: Scott Lee
*/

/* Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _MATH_VECTORS_H_
#define _MATH_VECTORS_H_

#include "MathHelper.h"

template<class T>
class Plane;
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
template<class T>
class Quaternion;
/***********************************************************************
/* vec2 -  A 2-tuple used to represent things like a vector in 2D space,
/* a point in 2D space or just 2 values linked together
/***********************************************************************/
template<class T>
class vec2 {
public:
    vec2() : x(0), y(0) { }
    vec2(T value) : x(value), y(value) { }
    vec2(T _x,T _y) : x(_x), y(_y) { }
    vec2(const T *_v) : x(_v[0]), y(_v[1]) { }
    vec2(const vec2 &_v) : x(_v.x), y(_v.y) { }
    vec2(const vec3<T> &_v);
    vec2(const vec4<T> &_v);

          bool operator==(const vec2 &v)   const { return this->compare(v); }
          bool operator!=(const vec2 &v)   const { return !(*this == v); }
          vec2 &operator=(T _f)                  { this->x=_f; this->y=_f; return (*this); }
    const vec2 operator*(T _f)             const { return vec2(this->x * _f,this->y * _f); }
    const vec2 operator/(T _i)             const { return vec2(this->x / _i,this->y / _i); }
    const vec2 operator+(const vec2 &v)    const { return vec2(this->x + v.x,this->y + v.y); }
    const vec2 operator-()                 const { return vec2(-this->x,-this->y); }
    const vec2 operator-(const vec2 &v)    const { return vec2(this->x - v.x,this->y - v.y); }
          vec2 &operator*=(T _f)                 { return *this = *this * _f; }
          vec2 &operator/=(T _f)                 { return *this = *this / _f; }
          vec2 &operator*=(const vec2 &v)        { return *this = *this * v;  }
          vec2 &operator+=(const vec2 &_v)       { return *this = *this + _v; }
          vec2 &operator-=(const vec2 &_v)       { return *this = *this - _v; }
          T     operator*(const vec2 &_v)  const { return this->x * _v.x + this->y * _v.y; }
          T     &operator[](I32 i)               { return this->_v[i]; }
    const T     &operator[](I32 i)         const { return this->_v[i]; }

    operator T*()             { return this->_v; }
    operator const T*() const { return this->_v; }
    /// swap the components  of this vector with that of the specified one
    inline void swap(vec2 *iv)             { std::swap(this->x,iv->x); std::swap(this->x,iv->x);}
    /// swap the components  of this vector with that of the specified one
    inline void swap(vec2 &iv)             { std::swap(this->x,iv.x);  std::swap(this->x,iv.x); }
     /// set the 2 components of the vector manually using a source pointer to a (large enough) array
    inline void setV(const T* v)           { this->x = v[0]; this->y = v[1]; }
    /// set the 2 components of the vector manually
    inline void set(T value)               { this->x = value; this->y = value;}
    /// set the 2 components of the vector manually
    inline void set(T _x,T _y)             { this->x = _x; this->y = _y; }
    /// set the 2 components of the vector using a source vector
    inline void set(const vec2<T> v)       { this->x = v.x; this->y =v.y; }
     /// set the 2 components of the vector using the first 2 components of the source vector
    inline void set(const vec3<T>& v)      { this->set(t.xy()); }
     /// set the 2 components of the vector using the first 2 components of the source vector
    inline void set(const vec4<T>& v)      { this->set(t.xy()); }
    /// set the 2 components of the vector back to 0
    inline void reset()                    { this->x = this->y = 0; }
    /// return the vector's length
    inline T    length()             const { return square_root_tpl(this->x * this->x + this->y * this->y); }
    /// return the angle defined by the 2 components
    inline T    angle()              const { return (T)atan2(this->y,this->x); }
    /// return the angle defined by the 2 components
    inline T    angle(const vec2 &v) const { return (T)atan2(v.y-this->y,v.x-this->x); }
    /// convert the vector to unit length
    inline T    normalize();
    /// round both values
    inline void round();
    /// calculate the dot product between this vector and the specified one
    inline T    dot(const vec2 &v) const;
    /// project this vector on the line defined by the 2 points(A, B)
    inline T    projectionOnLine(const vec2 &vA, const vec2 &vB) const;
    /// compare 2 vectors within the specified tolerance
    inline bool compare(const vec2 &_v,F32 epsi = EPSILON_F32) const;
    /// return the closest point on the line defined by the 2 points (A, B) and this vector
    inline vec2 closestPointOnLine(const vec2 &vA, const vec2 &vB) const;
    /// return the closest point on the line segment defined between the 2 points (A, B) and this vector
    inline vec2 closestPointOnSegment(const vec2 &vA, const vec2 &vB) const;
    /// lerp between the 2 specified vectors by the specified amount
    inline vec2 lerp(const vec2 &u, const vec2 &v, T factor) const;
    /// lerp between the 2 specified vectors by the specified amount for each component
    inline vec2 lerp(const vec2 &u, const vec2 &v, const vec2& factor) const;
     /// export the vector's components in the first 2 positions of the specified array
    inline void get(T * v) const;

    union {
        struct {T x,y;};
        struct {T s,t;};
        struct {T width,height;};
        struct {T min,max;};
        T _v[2];
    };
};

template <class T>
inline vec2<T> normalize(vec2<T>& vector) {
    vector.normalize();
    return vector;
}

/***********************************************************************
/* vec3 -  A 3-tuple used to represent things like a vector in 3D space,
/* a point in 3D space or just 3 values linked together
/***********************************************************************/
template <class T>
class vec3 {
public:
    vec3() : x(0), y(0), z(0) { }
    vec3(T value) : x(value), y(value), z(value) { }
    vec3(T _x,T _y,T _z) : x(_x), y(_y), z(_z) { }
    vec3(const T *v) : x(v[0]), y(v[1]), z(v[2]) { }
    vec3(const vec2<T> &v,T _z) : x(v.x), y(v.y), z(_z) { }
    vec3(const vec3 &v) : x(v.x), y(v.y), z(v.z) { }
    vec3(const vec4<T> &v);

          bool  operator!=(const vec3 &v)  const { return !(*this == v); }
          bool  operator==(const vec3 &v)  const { return this->compare(v); }
          vec3 &operator=(T _f)                  { this->x=_f; this->y=_f; this->z=_f; return (*this); }
    const vec3  operator*(T _f)            const { return vec3(this->x * _f,this->y * _f,this->z * _f); }
    const vec3  operator/(T _f)            const { if(IS_ZERO(_f)) return *this;_f = 1.0f / _f; return (*this) * _f; }
    const vec3  operator+(const vec3 &v)   const { return vec3(this->x + v.x,this->y + v.y,this->z + v.z); }
    const vec3  operator-()                const { return vec3(-this->x,-this->y,-this->z); }
    const vec3  operator-(const vec3 &v)   const { return vec3(this->x - v.x,this->y - v.y,this->z - v.z); }
    const vec3  operator*(const vec3 &v)   const { return vec3(this->x * v.x,this->y * v.y,this->z * v.z);}
          vec3 &operator*=(T _f)                 { return *this = *this * _f; }
          vec3 &operator/=(T _f)                 { return *this = *this / _f; }
          vec3 &operator*=(const vec3 &v)        { return *this = *this * v;  }
          vec3 &operator+=(const vec3 &v)        { return *this = *this + v; }
          vec3 &operator-=(const vec3 &v)        { return *this = *this - v; }
    //    T     operator*(const vec3 &v)   const { return this->x * v.x + this->y * v.y + this->z * v.z; }
          T    &operator[](const I32 i)          { return this->_v[i];}

    operator T*()             { return this->_v; }
    operator const T*() const { return this->_v; }

    ///GLSL like accessors
    inline vec2<T> rg() const { return vec2<T>(this->r,this->g);}
    inline vec2<T> xy() const { return this->rg();}
    inline vec2<T> rb() const { return vec2<T>(this->r, this->b);}
    inline vec2<T> xz() const { return this->rb();}
    inline vec2<T> gb() const { return vec2<T>(this->g, this->b);}
    inline vec2<T> yz() const { return this->gb();}

    /// set the 3 components of the vector manually using a source pointer to a (large enough) array
    inline void setV(const T* v)        { this->x = v[0]; this->y = v[1]; this->z = v[2]; }
    /// set the 3 components of the vector manually
    inline void set(T value)           { this->x = value; this->y = value; this->z = value;}
    /// set the 3 components of the vector manually
    inline void set(T _x, T _y, T _z)   { this->x = _x;  this->y = _y;  this->z = _z; }
    /// set the 3 components of the vector using a smaller source vector
    inline void set(const vec2<T>& v)   { this->x = v.x; this->y = v.y; this->z = 0.0;}
    /// set the 3 components of the vector using a source vector
    inline void set(const vec3<T>& v)   { this->x = v.x; this->y = v.y; this->z = v.z; }
    /// set the 3 components of the vector using the first 3 components of the source vector
    inline void set(const vec4<T>& v)   { this->set(v.xyz()); }
    /// set all the components back to 0
    inline void reset()                 { this->x = this->y = this->z = 0; }
    /// return the vector's length
    inline T    length()          const {return square_root_tpl(lengthSquared()); }
    /// return true if length is zero
    inline bool isZeroLength()    const { return lengthSquared() < EPSILON_F32; }
    /// compare 2 vectors within the specified tolerance
    inline bool compare(const vec3 &v,F32 epsi = EPSILON_F32) const;
    /// uniform vector: x = y = z
    inline bool isUniform() const;
    /// return the squared distance of the vector
    inline T    lengthSquared() const;
    /// calculate the dot product between this vector and the specified one
    inline T    dot(const vec3 &v) const;
    /// returns the angle in radians between '*this' and 'v'
    inline T    angle(vec3 &v) const;
    /// compute the vector's distance to another specified vector
    inline T    distance(const vec3 &v) const;
    /// compute the vector's squared distance to another specified vector
    inline T    distanceSquared(const vec3 &v) const;
    /// transform the vector to unit length
    inline T    normalize();
    /// round all three values
    inline void round();
    /// project this vector on the line defined by the 2 points(A, B)
    inline T    projectionOnLine(const vec3 &vA, const vec3 &vB) const;
    /// get the direction vector to the specified point
    inline vec3 direction(const vec3& u) const;
    /// return the closest point on the line defined by the 2 points (A, B) and this vector
    inline vec3 closestPointOnLine(const vec3 &vA, const vec3 &vB) const;
    /// return the closest point on the line segment created between the 2 points (A, B) and this vector
    inline vec3 closestPointOnSegment(const vec3 &vA, const vec3 &vB) const;
    /// lerp between the 2 specified vectors by the specified amount
    inline vec3 lerp(const vec3 &u, const vec3 &v, T factor) const;
    /// lerp between the 2 specified vectors by the specified amount for each component
    inline vec3 lerp(const vec3 &u, const vec3 &v, const vec3& factor) const;
    /// lerp between this and the specified vector by the specified amount
    inline void lerp(const vec3 &v, T factor);
    /// lerp between this and the specified vector by the specified amount for each component
    inline void lerp(const vec3 &v, const vec3& factor);
    /// this calculates a vector between the two specified points and returns the result
    inline vec3 vector(const vec3 &vp1, const vec3 &vp2) const;
    /// set this vector to be equal to the cross of the 2 specified vectors
    inline void cross(const vec3 &v1,const vec3 &v2);
    /// set this vector to be equal to the cross between itself and the specified vector
    inline void cross(const vec3 &v2);
    /// rotate this vector on the X axis
    inline void rotateX(D32 radians);
    /// rotate this vector on the Y axis
    inline void rotateY(D32 radians);
    /// rotate this vector on the Z axis
    inline void rotateZ(D32 radians);
    /// swap the components  of this vector with that of the specified one
    inline void swap(vec3 &iv);
    /// swap the components  of this vector with that of the specified one
    inline void swap(vec3 *iv);
    /// export the vector's components in the first 3 positions of the specified array
    inline void get(T * v) const;

    union {
        struct {T x,y,z;};
        struct {T s,t,p;};
        struct {T r,g,b;};
        struct {T pitch,yaw,roll;};
        struct {T width,height,depth;};
        T _v[3];
    };
};

template <class T>
inline vec3<T> normalize(vec3<T>& vector) {
    vector.normalize();
    return vector;
}

/*************************************************************************************
/* vec4 -  A 4-tuple used to represent things like a vector in 4D space (w-component)
/* or just 4 values linked together
/************************************************************************************/
template <class T>
class vec4 {
public:
    vec4() : x(0), y(0), z(0), w(1)                             { }
    vec4(T value) : x(value), y(value), z(value), w(value)      { }
    vec4(T _x,T _y,T _z,T _w) : x(_x), y(_y), z(_z), w(_w)      { }
    vec4(const T *v) : x(v[0]), y(v[1]), z(v[2]), w(v[3])       { }
    vec4(const vec3<T> &v) : x(v.x), y(v.y), z(v.z), w(1)       { }
    vec4(const vec3<T> &v,T _w) : x(v.x), y(v.y), z(v.z), w(_w) { }
    vec4(const vec4 &v) : x(v.x), y(v.y), z(v.z), w(v.w)        { }

          bool  operator==(const vec4 &v)   const { return this->compare(v); }
          bool  operator!=(const vec4 &v)   const { return !(*this == v); }
          vec4 &operator=(T _f)                   { this->x=_f; this->y=_f; this->z=_f; this->w=_f; return (*this);}
    const vec4  operator*(T _f)             const { return vec4(this->x * _f,this->y * _f,this->z * _f,this->w * _f); }
    const vec4  operator/(T _f)             const { if(IS_ZERO(_f)) return *this; _f = 1.0f / _f; return (*this) * _f; }
    const vec4  operator-()                 const { return vec4(-x,-y,-z,-w); }
    const vec4  operator+(const vec4 &v)    const { return vec4(this->x + v.x,this->y + v.y,this->z + v.z,this->w + v.w); }
    const vec4  operator-(const vec4 &v)    const { return vec4(this->x - v.x,this->y - v.y,this->z - v.z,this->w - v.w); }
          vec4 &operator*=(T _f)                  { return *this = *this * _f; }
          vec4 &operator/=(T _f)                  { return *this = *this / _f; }
          vec4 &operator*=(const vec4 &v)         { return *this = *this * v; }
          vec4 &operator+=(const vec4 &v)         { return *this = *this + v; }
          vec4 &operator-=(const vec4 &v)         { return *this = *this - v; }
          T     operator*(const vec3<T> &v) const { return this->x * v.x + this->y * v.y + this->z * v.z + this->w; }
          T     operator*(const vec4<T> &v) const { return this->x * v.x + this->y * v.y + this->z * v.z + this->w * v.w; }

    operator T*()             { return this->_v; }
    operator const T*() const { return this->_v; }

          T &operator[](I32 i)        { return this->_v[i]; }
    const T &operator[](I32 _i) const { return this->_v[_i]; }

    /// GLSL like accessors
    inline vec2<T> rg()  const {return vec2<T>(this->r,this->g);}
    inline vec2<T> xy()  const {return this->rg();}
    inline vec2<T> rb()  const { return vec2<T>(this->r, this->b);}
    inline vec2<T> xz()  const { return this->rb();}
    inline vec2<T> gb()  const { return vec2<T>(this->g, this->b);}
    inline vec2<T> yz()  const { return this->gb();}
    inline vec2<T> ra()  const { return vec2<T>(this->r, this->a); }
    inline vec2<T> xw()  const { return this->ra(); }
    inline vec2<T> ga()  const { return vec2<T>(this->g, this->a); }
    inline vec2<T> yw()  const { return this->ga(); }
    inline vec2<T> ba()  const { return vec2<T>(this->b, this->a); }
    inline vec2<T> zw()  const { return this->ba(); }
    inline vec3<T> rgb() const {return vec3<T>(this->r,this->g,this->b);}
    inline vec3<T> xyz() const {return this->rgb();}
    inline vec3<T> bgr() const {return vec3<T>(this->b,this->g,this->r);}
    inline vec3<T> zyx() const {return this->bgr();}
    inline vec3<T> rga() const { return vec3<T>(this->r, this->g, this->a); }
    inline vec3<T> xyw() const { return this->rga(); }
    inline vec3<T> gba() const { return vec3<T>(this->g, this->b, this->a); }
    inline vec3<T> yzw() const { return this->gba(); }
    /// set the 4 components of the vector manually using a source pointer to a (large enough) array
    inline void setV(const T* v)         { this->x = v[0]; this->y = v[1]; this->z = v[2]; if(v[3]) this->w = v[3]; else this->w = 1.0f; }
    /// set the 4 components of the vector manually
    inline void set(T value)             { this->x = value; this->y = value; this->z = value; this->w = value; }
    /// set the 4 components of the vector manually
    inline void set(T _x,T _y,T _z,T _w) { this->x = _x;  this->y =_y;   this->z =_z;   this->w =_w;}
    /// set the 4 components of the vector using a source vector
    inline void set(const vec4& v)       { this->x = v.x; this->y = v.y; this->z = v.z; this->w = v.w;}
    /// set the 4 components of the vector using a smaller source vector
    inline void set(const vec3<T>& v)    { this->x = v.x; this->y = v.y; this->z = v.z; this->w = 1.0;}
    /// set the 4 components of the vector using a smaller source vector
    inline void set(const vec3<T>& v, T w) { this->x = v.x; this->y = v.y; this->z = v.z; this->w = w; }
    /// set the 4 components of the vector using a smaller source vector
    inline void set(const vec2<T>& v)    { this->x = v.x; this->y = v.y; this->z = 0.0; this->w = 1.0;}
    /// set the 4 components of the vector using smallers source vectors
    inline void set(const vec2<T>& v1, const vec2<T>& v2)    { this->x = v1.x; this->y = v1.y; this->z = v2.x; this->w = v2.y; }
    /// set all the components back to 0
    inline void reset()                  { this->x = this->y = this->z = this->w = 0;}
    /// compare 2 vectors within the specified tolerance
    inline bool compare(const vec4 &v,F32 epsi = EPSILON_F32) const;
    /// lerp between the 2 specified vectors by the specified amount
    inline vec4 lerp(const vec4 &u, const vec4 &v, T factor) const;
    /// lerp between the 2 specified vectors by the specified amount for each component
    inline vec4 lerp(const vec4 &u, const vec4 &v, const vec4& factor) const;
    /// swap the components  of this vector with that of the specified one
    inline void swap(vec4 *iv);
    /// swap the components  of this vector with that of the specified one
    inline void swap(vec4 &iv);
    /// transform the vector to unit length
    inline T    normalize();
    /// min/max functions
    inline vec4 min(const vec4 &v1, const vec4 &v2) { return vec4(std::min(v1.x, v2.x), std::min(v1.y, v2.y), std::min(v1.z, v2.z), std::min(v1.w, v2.w)); }
    inline vec4 max(const vec4 &v1, const vec4 &v2) { return vec4(std::max(v1.x, v2.x), std::max(v1.y, v2.y), std::max(v1.z, v2.z), std::max(v1.w, v2.w)); }
    /// return the vector's length
    inline T    length()    const {return square_root_tpl(lengthSquared()); }
    /// return the squared distance of the vector
    inline T    lengthSquared() const;
    /// round all four values
    inline void round();

    union {
        struct {T x,y,z,w;};
        struct {T s,t,p,q;};
        struct {T r,g,b,a;};
        struct {T fov,ratio,znear,zfar;};
        struct {T width,height,depth,key;};
        T _v[4];
    };
};

template <class T>
inline vec4<T> normalize(vec4<T>& vector) {
    vector.normalize();
    return vector;
}

///Quaternion multiplications require these to be floats
extern vec2<F32> VECTOR2_ZERO;
extern vec3<F32> VECTOR3_ZERO;
extern vec4<F32> VECTOR4_ZERO;
extern vec3<F32> WORLD_X_AXIS;
extern vec3<F32> WORLD_Y_AXIS;
extern vec3<F32> WORLD_Z_AXIS;
extern vec3<F32> WORLD_X_NEG_AXIS;
extern vec3<F32> WORLD_Y_NEG_AXIS;
extern vec3<F32> WORLD_Z_NEG_AXIS;
extern vec3<F32> DEFAULT_GRAVITY;
extern vec4<F32> UNIT_RECT;

extern vec2<I32> iVECTOR2_ZERO;
extern vec3<I32> iVECTOR3_ZERO;
extern vec4<I32> iVECTOR4_ZERO;
extern vec3<I32> iWORLD_X_AXIS;
extern vec3<I32> iWORLD_Y_AXIS;
extern vec3<I32> iWORLD_Z_AXIS;
extern vec3<I32> iWORLD_X_NEG_AXIS;
extern vec3<I32> iWORLD_Y_NEG_AXIS;
extern vec3<I32> iWORLD_Z_NEG_AXIS;

//Inline definitions
#include "MathVectors-Inl.h"

#endif
