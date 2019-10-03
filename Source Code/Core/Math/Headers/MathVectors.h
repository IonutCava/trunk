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

/* Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#pragma once
#ifndef _MATH_VECTORS_H_
#define _MATH_VECTORS_H_

#include "MathHelper.h"

namespace Divide {

template <size_t alignment = 16>
struct alligned_base {
    void *operator new (size_t size)
    {
        return _mm_malloc(size, alignment);
    }

        void *operator new[](size_t size)
    {
        return _mm_malloc(size, alignment);
    }

        void operator delete (void *mem)
    {
        _mm_free(mem);
    }

    void operator delete[](void *mem)
    {
        _mm_free(mem);
    }
};

struct non_aligned_base {
};

template<typename T, typename Enable = void>
class simd_vector;

template<typename T>
class simd_vector<T, std::enable_if_t<std::is_same<T, F32>::value>> {
public:
    simd_vector()  noexcept : simd_vector(0)
    {
    }

    simd_vector(T reg)  noexcept : _reg(_mm_set_ps(reg, reg, reg, reg))
    {
    }

    simd_vector(T reg0, T reg1, T reg2, T reg3)  noexcept :
        _reg(_mm_set_ps(reg3, reg2, reg1, reg0))
    {
    }

    simd_vector(T reg[4])  noexcept : _reg(_mm_set_ps(reg[3], reg[2], reg[1], reg[0]))
    {
    }

    simd_vector(__m128 reg)  noexcept : _reg(reg)
    {
    }

    inline bool operator==(const simd_vector& other) const {
        return !fneq128(_reg, other._reg, EPSILON_F32);
    }

    inline bool operator!=(const simd_vector& other) const {
        return fneq128(_reg, other._reg, EPSILON_F32);
    }

    __m128 _reg;
};

template<typename T>
class simd_vector<T, std::enable_if_t<!std::is_same<T, F32>::value>> {
  public:
    simd_vector()  noexcept : simd_vector(0)
    {
    }

    simd_vector(T val) noexcept : 
        _reg { val, val, val, val }
    {
    }


    simd_vector(T reg0, T reg1, T reg2, T reg3)  noexcept : 
        _reg{reg0, reg1, reg2, reg3}
    {
    }

    simd_vector(T reg[4])  noexcept : _reg(reg)
    {
    }

    inline bool operator==(const simd_vector& other) const {
        for (U8 i = 0; i < 4; ++i) {
            if (_reg[i] != other._reg[i]) {
                return false;
            }
        }
        return true;
    }

    inline bool operator!=(const simd_vector& other) const {
        for (U8 i = 0; i < 4; ++i) {
            if (_reg[i] != other._reg[i]) {
                return true;
            }
        }
        return false;
    }

    T _reg[4];
};

/***********************************************************************
 * vec2 -  A 2-tuple used to represent things like a vector in 2D space,
 * a point in 2D space or just 2 values linked together
 ***********************************************************************/
template <typename T>
class vec2 {
    static_assert(std::is_arithmetic<T>::value ||
                  std::is_same<T, bool>::value,
                  "non-arithmetic vector type");
   public:
    vec2() noexcept : vec2((T)0)
    {
    }
    vec2(T value) noexcept : vec2(value, value)
    {
    }

    template<typename U>
    vec2(U value) noexcept : vec2(value, value)
    {
    }

    vec2(T _x, T _y) noexcept : x(_x), y(_y)
    {
    }

    template <typename U>
    vec2(U _x, U _y) noexcept 
        : x(static_cast<T>(_x)),
          y(static_cast<T>(_y))
    {
    }
    template <typename U, typename V>
    vec2(U _x, V _y) noexcept 
        : x(static_cast<T>(_x)),
          y(static_cast<T>(_y))
    {
    }
    vec2(const T *_v) noexcept : vec2(_v[0], _v[1])
    {
    }

    vec2(const vec2 &_v) noexcept : vec2(_v._v)
    {
    }

    vec2(const vec3<T> &_v) noexcept : vec2(_v.xy())
    {
    }

    vec2(const vec4<T> &_v) noexcept : vec2(_v.xy())
    {
    }

    template<typename U>
    vec2(const vec2<U> &v) noexcept : vec2(v.x, v.y)
    {
    }

    template<typename U>
    vec2(const vec3<U> &v) noexcept : vec2(v.x, v.y)
    {
    }

    template<typename U>
    vec2(const vec4<U> &v) noexcept : vec2(v.x, v.y)
    {
    }

    bool operator>(const vec2 &v) const { return x > v.x && y > v.y; }
    bool operator<(const vec2 &v) const { return x < v.x && y < v.y; }
    bool operator<=(const vec2 &v) const { return *this < v || *this == v; }
    bool operator>=(const vec2 &v) const { return *this > v || *this == v; }

    bool operator==(const vec2 &v) const { return this->compare(v); }
    bool operator!=(const vec2 &v) const { return !this->compare(v); }

    template<typename U>
    bool operator!=(const vec2<U> &v) const { return !this->compare(v); }
    template<typename U>
    bool operator==(const vec2<U> &v) const { return this->compare(v); }

    const vec2 operator-(T _f) const {
        return vec2(this->x - _f, this->y - _f);
    }

    template<typename U>
    vec2 &operator=(U _f) noexcept {
        this->set(_f);
        return (*this);
    }

    template<typename U>
    vec2 &operator=(const vec2<U>& other) noexcept {
        this->set(other);
        return (*this);
    }

    template<typename U>
    const vec2 operator+(U _f) const {
        return vec2(this->x + _f, this->y + _f);
    }

    template<typename U>
    const vec2 operator-(U _i) const {
        return vec2(this->x - _i, this->y - _i);
    }

    template<typename U>
    const vec2 operator*(U _f) const {
        return vec2(this->x * _f, this->y * _f);
    }

    template<typename U>
    const vec2 operator/(U _i) const {
        return vec2(this->x / _i, this->y / _i);
    }

    template<typename U>
    const vec2 operator+(const vec2<U> &v) const {
        return vec2(this->x + v.x, this->y + v.y);
    }

    const vec2 operator-() const { return vec2(-this->x, -this->y); }
    const vec2 operator-(const vec2 &v) const {
        return vec2(this->x - v.x, this->y - v.y);
    }
 
    template<typename U>
    vec2 &operator+=(U _f) {
        this->set(*this + _f);
        return *this;
    }

    template<typename U>
    vec2 &operator-=(U _f) {
        this->set(*this - _f);
        return *this;
    }

    template<typename U>
    vec2 &operator*=(U _f) {
        this->set(*this * _f);
        return *this;
    }

    template<typename U>
    vec2 &operator/=(U _f) {
        this->set(*this / _f);
        return *this;
    }

    template<typename U>
    vec2 &operator*=(const vec2<U> &v) {
        this->set(*this * v);
        return *this;
    }

    template<typename U>
    vec2 &operator/=(const vec2<U> &v) {
        this->set(*this / v);
        return *this;
    }

    template<typename U>
    vec2 &operator+=(const vec2<U> &v) {
        this->set(*this + v);
        return *this;
    }

    template<typename U>
    vec2 &operator-=(const vec2<U> &v) {
        this->set(*this - v);
        return *this;
    }

    template<typename U>
    vec2 operator*(const vec2<U> &v) const {
        return vec2(this->x * v.x, this->y * v.y);
    }

    T &operator[](I32 i) { return this->_v[i]; }
    const T &operator[](I32 i) const { return this->_v[i]; }
    const vec2 operator/(const vec2 &v) const {
        return vec2(IS_ZERO(v.x) ? this->x : this->x / v.x,
                    IS_ZERO(v.y) ? this->y : this->y / v.y);
    }
    operator T *() { return this->_v; }
    operator const T *() const { return this->_v; }
    /// swap the components  of this vector with that of the specified one
    inline void swap(vec2 *iv) {
        std::swap(this->x, iv->x);
        std::swap(this->x, iv->x);
    }
    /// swap the components  of this vector with that of the specified one
    inline void swap(vec2 &iv) {
        std::swap(this->x, iv.x);
        std::swap(this->x, iv.x);
    }
    /// set the 2 components of the vector manually using a source pointer to a (large enough) array
    inline void set(const T* v) { std::memcpy(&_v[0], &v[0], sizeof(T) * 2); }
    /// set the 2 components of the vector manually
    inline void set(T value) { this->set(value, value); }
    /// set the 2 components of the vector manually
    inline void set(T _x, T _y) noexcept {
        this->x = _x;
        this->y = _y;
    }
    template <typename U> 
    inline void set(U _x, U _y) {
        this->x = static_cast<T>(_x);
        this->y = static_cast<T>(_y);
    }
    /// set the 2 components of the vector using a source vector
    inline void set(const vec2 &v) { this->set(&v._v[0]); }
    /// set the 2 components of the vector using the first 2 components of the
    /// source vector
    inline void set(const vec3<T> &v) { this->set(v.x, v.y); }
    /// set the 2 components of the vector using the first 2 components of the
    /// source vector
    inline void set(const vec4<T> &v) { this->set(v.x, v.y); }
    /// set the 2 components of the vector back to 0
    inline void reset() { this->set((T)0); }
    /// return the vector's length
    inline T length() const { return Divide::Sqrt(lengthSquared()); }
    inline T lengthSquared() const;
    /// return the angle defined by the 2 components
    inline T angle() const { return (T)std::atan2(this->y, this->x); }
    /// return the angle defined by the 2 components
    inline T angle(const vec2 &v) const {
        return (T)std::atan2(v.y - this->y, v.x - this->x);
    }
    /// compute the vector's distance to another specified vector
    inline T distance(const vec2 &v, bool absolute = true) const;
    /// compute the vector's squared distance to another specified vector
    inline T distanceSquared(const vec2 &v) const;
    /// convert the vector to unit length
    inline vec2& normalize();
    /// round both values
    inline void round();
    /// lerp between this and the specified vector by the specified amount
    inline void lerp(const vec2 &v, T factor);
    /// lerp between this and the specified vector by the specified amount for
    /// each component
    inline void lerp(const vec2 &v, const vec2 &factor);
    /// calculate the dot product between this vector and the specified one
    inline T dot(const vec2 &v) const;
    /// project this vector on the line defined by the 2 points(A, B)
    inline T projectionOnLine(const vec2 &vA, const vec2 &vB) const;
    /// return the closest point on the line defined by the 2 points (A, B) and this
    /// vector
    inline vec2 closestPointOnLine(const vec2 &vA, const vec2 &vB);
    /// return the closest point on the line segment defined between the 2 points
    /// (A, B) and this vector
    inline vec2 closestPointOnSegment(const vec2 &vA, const vec2 &vB);
    /// compare 2 vectors
    template<typename U>
    inline bool compare(const vec2<U> &_v) const;
    /// compare 2 vectors within the specified tolerance
    template<typename U>
    inline bool compare(const vec2<U> &_v, U epsi) const;
    /// export the vector's components in the first 2 positions of the specified array
    inline void get(T *v) const;

    union {
        struct {
            T x, y;
        };
        struct {
            T s, t;
        };
        struct {
            T width, height;
        };
        struct {
            T w, h;
        };
        struct {
            T min, max;
        };
        T _v[2];
    };
};

/// lerp between the 2 specified vectors by the specified amount
template <typename T, typename U>
inline vec2<T> Lerp(const vec2<T> &u, const vec2<T> &v, U factor);

/// lerp between the 2 specified vectors by the specified amount for each component
template <typename T>
inline vec2<T> Lerp(const vec2<T> &u, const vec2<T> &v, const vec2<T> &factor);
template <typename T>
inline vec2<T> Cross(const vec2<T> &v1, const vec2<T> &v2);
template <typename T>
inline vec2<T> Inverse(const vec2<T> &v);
template <typename T>
inline vec2<T> Normalize(vec2<T> &vector);
template <typename T>
inline vec2<T> Normalized(const vec2<T> &vector);
template <typename T>
inline T Dot(const vec2<T> &a, const vec2<T> &b);
template <typename T>
inline void OrthoNormalize(vec2<T> &v1, vec2<T> &v2);

/// multiply a vector by a value
template <typename T>
inline vec2<T> operator*(T fl, const vec2<T> &v);
/***********************************************************************
 * vec3 -  A 3-tuple used to represent things like a vector in 3D space,
 * a point in 3D space or just 3 values linked together
 ***********************************************************************/
template <typename T>
class vec3 {
    static_assert(std::is_arithmetic<T>::value ||
                  std::is_same<T, bool>::value,
                  "non-arithmetic vector type");
   public:
    vec3() noexcept : vec3((T)0)
    {
    }
    vec3(T value) noexcept : vec3(value, value, value)
    {
    }
    template<typename U>
    vec3(U value) noexcept : vec3(value, value, value)
    {
    }

    vec3(T _x, T _y, T _z) noexcept : x(_x), y(_y), z(_z)
    {
    }

    template <typename U>
    vec3(U _x, U _y, U _z)  noexcept 
        : x(static_cast<T>(_x)),
          y(static_cast<T>(_y)),
          z(static_cast<T>(_z))
    {
    }
    template <typename U, typename V>
    vec3(U _x, U _y, V _z) noexcept
        : x(static_cast<T>(_x)),
          y(static_cast<T>(_y)),
          z(static_cast<T>(_z))
    {
    }
    template <typename U, typename V>
    vec3(U _x, V _y, V _z) noexcept
        : x(static_cast<T>(_x)),
          y(static_cast<T>(_y)),
          z(static_cast<T>(_z))
    {
    }
    template <typename U, typename V, typename W>
    vec3(U _x, V _y, W _z) noexcept
        : x(static_cast<T>(_x)),
          y(static_cast<T>(_y)),
          z(static_cast<T>(_z))
    {
    }
    vec3(const T *v) noexcept : vec3(v[0], v[1], v[2])
    {
    }
    vec3(const vec2<T> &v) noexcept : vec3(v, static_cast<T>(0))
    {
    }
    vec3(const vec2<T> &v, T _z) noexcept : vec3(v.x, v.y, _z)
    {
    }
    vec3(const vec3 &v) noexcept : vec3(v._v)
    {
    }
    vec3(const vec4<T> &v) noexcept : vec3(v.x, v.y, v.z)
    {
    }

    template<typename U>
    vec3(const vec2<U> &v) noexcept : vec3(v.x, v.y, static_cast<U>(0))
    {
    }

    template<typename U>
    vec3(const vec3<U> &v) noexcept : vec3(v.x, v.y, v.z)
    {
    }

    template<typename U>
    vec3(const vec4<U> &v) noexcept : vec3(v.x, v.y, v.z)
    {
    }

    bool operator>(const vec3 &v) const noexcept { return x > v.x && y > v.y && z > v.z; }
    bool operator<(const vec3 &v) const noexcept { return x < v.x && y < v.y && z < v.z; }
    bool operator<=(const vec3 &v) const noexcept { return *this < v || *this == v; }
    bool operator>=(const vec3 &v) const noexcept { return *this > v || *this == v; }
    
    bool operator!=(const vec3 &v) const noexcept { return !this->compare(v); }
    bool operator==(const vec3 &v) const noexcept { return this->compare(v); }

    template<typename U>
    bool operator!=(const vec3<U> &v) const { return !this->compare(v); }
    template<typename U>
    bool operator==(const vec3<U> &v) const { return this->compare(v); }

    template <typename U>
    vec3 &operator=(U _f) noexcept {
        this->set(_f);
        return (*this);
    }

    template <typename U>
    vec3 &operator=(const vec3<U>& other) noexcept {
        this->set(other);
        return (*this);
    }

    template <typename U>
    const vec3 operator+(U _f) const {
        return vec3(this->x + _f, this->y + _f, this->z + _f);
    }

    template <typename U>
    const vec3 operator-(U _f) const {
        return vec3(this->x - _f, this->y - _f, this->z - _f);
    }

    template <typename U>
    const vec3 operator*(U _f) const {
        return vec3(this->x * _f, this->y * _f, this->z * _f);
    }

    template <typename U>
    const vec3 operator/(U _f) const {
        if (IS_ZERO(_f)) {
            return *this;
        }

        return (*this) * (1.0f / _f);
    }

    template <typename U>
    const vec3 operator+(const vec3<U> &v) const {
        return vec3(this->x + v.x, this->y + v.y, this->z + v.z);
    }

    const vec3 operator-() const  noexcept { return vec3(-this->x, -this->y, -this->z); }
    const vec3 operator-(const vec3 &v) const  noexcept {
        return vec3(this->x - v.x, this->y - v.y, this->z - v.z);
    }
    template<typename U>
    const vec3 operator*(const vec3<U> &v) const noexcept {
        return vec3(this->x * v.x, this->y * v.y, this->z * v.z);
    }
    
    template<typename U>
    vec3 &operator+=(U _f) noexcept {
        this->set(*this + _f);
        return *this;
    }

    template<typename U>
    vec3 &operator-=(U _f) noexcept {
        this->set(*this - _f);
        return *this;
    }

    template<typename U>
    vec3 &operator*=(U _f) noexcept {
        this->set(*this * _f);
        return *this;
    }

    template<typename U>
    vec3 &operator/=(U _f) {
        this->set(*this / _f);
        return *this;
    }

    template<typename U>
    vec3 &operator*=(const vec3<U> &v) noexcept {
        this->set(*this * v);
        return *this;
    }

    template<typename U>
    vec3 &operator/=(const vec3<U> &v) noexcept {
        this->set(*this / v);
        return *this;
    }

    template<typename U>
    vec3 &operator+=(const vec3<U> &v) noexcept {
        this->set(*this + v);
        return *this;
    }

    template<typename U>
    vec3 &operator-=(const vec3<U> &v) noexcept {
        this->set(*this - v);
        return *this;
    }
    //    T     operator*(const vec3 &v)   const { return this->x * v.x +
    //    this->y * v.y + this->z * v.z; }
    T &operator[](const I32 i)  noexcept { return this->_v[i]; }
    const vec3 operator/(const vec3 &v) const {
        return vec3(IS_ZERO(v.x) ? this->x : this->x / v.x,
                    IS_ZERO(v.y) ? this->y : this->y / v.y,
                    IS_ZERO(v.z) ? this->z : this->z / v.z);
    }
    operator T *()  noexcept { return this->_v; }
    operator const T *() const  noexcept { return this->_v; }

    /// GLSL like accessors (const to prevent erroneous usage like .xy() += n)
    inline const vec2<T> rg() const { return vec2<T>(this->r, this->g); }
    inline const vec2<T> xy() const { return this->rg(); }
    inline const vec2<T> rb() const { return vec2<T>(this->r, this->b); }
    inline const vec2<T> xz() const { return this->rb(); }
    inline const vec2<T> gb() const { return vec2<T>(this->g, this->b); }
    inline const vec2<T> yz() const { return this->gb(); }

    inline void rg(const vec2<T> &rg) { this->set(rg); }
    inline void xy(const vec2<T> &xy) { this->set(xy); }
    inline void rb(const vec2<T> &rb) {
        this->r = rb.x;
        this->b = rb.y;
    }
    inline void xz(const vec2<T> &xz) {
        this->x = xz.x;
        this->z = xz.y;
    }
    inline void gb(const vec2<T> &gb) {
        this->g = gb.x;
        this->b = gb.y;
    }
    inline void yz(const vec2<T> &yz) {
        this->y = yz.x;
        this->z = yz.y;
    }

    /// set the 3 components of the vector manually using a source pointer to a (large enough) array
    inline void set(const T* v) noexcept { std::memcpy(&_v[0], &v[0], 3 * sizeof(T)); }
    /// set the 3 components of the vector manually
    inline void set(T value)  noexcept { x = value; y = value; z = value; }
    /// set the 3 components of the vector manually
    inline void set(T _x, T _y, T _z)  noexcept {
        this->x = _x;
        this->y = _y;
        this->z = _z;
    }
    template <typename U> 
    inline void set(U _x, U _y, U _z) noexcept {
        this->x = static_cast<T>(_x);
        this->y = static_cast<T>(_y);
        this->z = static_cast<T>(_z);
    }
    /// set the 3 components of the vector using a smaller source vector
    inline void set(const vec2<T> &v) { std::memcpy(_v, v._v, 2 * sizeof(T)); }
    /// set the 3 components of the vector using a source vector
    inline void set(const vec3 &v) { std::memcpy(_v, v._v, 3 * sizeof(T)); }
    /// set the 3 components of the vector using the first 3 components of the source vector
    inline void set(const vec4<T> &v) { std::memcpy(_v, v._v, 3 * sizeof(T)); }
    /// set all the components back to 0
    inline void reset() { std::memset(_v, 0, 3 * sizeof(T)); }
    /// return the vector's length
    inline T length() const { return Divide::Sqrt(lengthSquared()); }
    /// return true if length is zero
    inline bool isZeroLength() const { return lengthSquared() < EPSILON_F32; }
    /// compare 2 vectors
    template<typename U>
    inline bool compare(const vec3<U> &v) const;
    /// compare 2 vectors within the specified tolerance
    template<typename U>
    inline bool compare(const vec3<U> &v, U epsi) const;
    /// uniform vector: x = y = z
    inline bool isUniform() const;
    /// return the squared distance of the vector
    inline T lengthSquared() const noexcept;
    /// calculate the dot product between this vector and the specified one
    inline T dot(const vec3 &v) const;
    /// returns the angle in radians between '*this' and 'v'
    inline T angle(vec3 &v) const;
    /// compute the vector's distance to another specified vector
    inline T distance(const vec3 &v, bool absolute = true) const;
    /// compute the vector's squared distance to another specified vector
    inline T distanceSquared(const vec3 &v) const noexcept;
    /// transform the vector to unit length
    inline vec3& normalize();
    /// round all three values
    inline void round();
    /// project this vector on the line defined by the 2 points(A, B)
    inline T projectionOnLine(const vec3 &vA, const vec3 &vB) const;
    /// return the closest point on the line defined by the 2 points (A, B) and this
    /// vector
    inline vec3 closestPointOnLine(const vec3 &vA, const vec3 &vB);
    /// return the closest point on the line segment created between the 2 points
    /// (A, B) and this vector
    inline vec3 closestPointOnSegment(const vec3 &vA, const vec3 &vB);
    /// get the direction vector to the specified point
    inline vec3 direction(const vec3 &u) const;
    /// lerp between this and the specified vector by the specified amount
    inline void lerp(const vec3 &v, T factor);
    /// lerp between this and the specified vector by the specified amount for
    /// each component
    inline void lerp(const vec3 &v, const vec3 &factor);
    /// this calculates a vector between the two specified points and returns
    /// the result
    inline vec3 vector(const vec3 &vp1, const vec3 &vp2) const;
    /// set this vector to be equal to the cross of the 2 specified vectors
    inline void cross(const vec3 &v1, const vec3 &v2);
    /// set this vector to be equal to the cross between itself and the
    /// specified vector
    inline void cross(const vec3 &v2);
    /// rotate this vector on the X axis
    inline void rotateX(D64 radians);
    /// rotate this vector on the Y axis
    inline void rotateY(D64 radians);
    /// rotate this vector on the Z axis
    inline void rotateZ(D64 radians);
    /// swap the components  of this vector with that of the specified one
    inline void swap(vec3 &iv);
    /// swap the components  of this vector with that of the specified one
    inline void swap(vec3 *iv);
    /// export the vector's components in the first 3 positions of the specified
    /// array
    inline void get(T *v) const;

    union {
        struct {
            T x, y, z;
        };
        struct {
            T s, t, p;
        };
        struct {
            T r, g, b;
        };
        struct {
            T pitch, yaw, roll;
        };
        struct {
            T turn, move, zoom;
        };
        struct {
            T width, height, depth;
        };
        T _v[3];
    };
};

/// lerp between the 2 specified vectors by the specified amount
template <typename T, typename U>
inline vec3<T> Lerp(const vec3<T> &u, const vec3<T> &v, U factor);
/// lerp between the 2 specified vectors by the specified amount for each
/// component
template <typename T>
inline vec3<T> Lerp(const vec3<T> &u, const vec3<T> &v, const vec3<T> &factor);
template <typename T>
inline vec3<T> Normalize(vec3<T> &vector);
template <typename T>
inline vec3<T> Normalized(const vec3<T> &vector);
/// general vec3 dot product
template <typename T>
inline T Dot(const vec3<T> &a, const vec3<T> &b);
/// general vec3 cross function
template <typename T>
inline vec3<T> Cross(const vec3<T> &v1, const vec3<T> &v2);
template <typename T>
inline vec3<T> Inverse(const vec3<T> &v);
template <typename T>
inline vec3<T> operator*(T fl, const vec3<T> &v);
template <typename T>
inline void OrthoNormalize(vec3<T> &v1, vec3<T> &v2);

/*************************************************************************************
 * vec4 -  A 4-tuple used to represent things like a vector in 4D space
(w-component)
 * or just 4 values linked together
 ************************************************************************************/
#pragma pack(push)
#pragma pack(1)
//__declspec(align(alignment))
template <typename T>
class vec4 : public std::conditional<std::is_same<T, F32>::value, alligned_base<16>, non_aligned_base>::type {
    static_assert(std::is_arithmetic<T>::value ||
                  std::is_same<T, bool>::value,
                  "non-arithmetic vector type");
   public:
    vec4() noexcept : x(0), y(0), z(0), w(1)
    {
    }

    vec4(T _x, T _y, T _z, T _w) noexcept 
        : x(_x), y(_y), z(_z), w(_w)
    {
    }
    vec4(T _x, T _y, T _z) noexcept
        : x(_x), y(_y), z(_z), w(static_cast<T>(1))
    {
    }

    template<typename U>
    vec4(U _x, U _y, U _z, U _w) noexcept 
        : x(static_cast<T>(_x)),
          y(static_cast<T>(_y)),
          z(static_cast<T>(_z)),
          w(static_cast<T>(_w))
    {
    }

    template<typename U>
    vec4(U _x, U _y, U _z) noexcept
        : vec4(_x, _y, _z, static_cast<T>(1))
    {
    }

    vec4(__m128 reg) noexcept: _reg(reg)
    {
    }

    vec4(const simd_vector<T>& reg) noexcept: _reg(reg)
    {
    }

    vec4(T value) noexcept : vec4(value, value, value, value)
    {
    }

    template<typename U>
    vec4(U value) noexcept : vec4(value, value, value, value)
    {
    }

    vec4(const T *v) noexcept : vec4(v[0], v[1], v[2], v[3])
    {
    }

    vec4(const vec2<T> &v) noexcept : vec4(v, static_cast<T>(0))
    {
    }

    vec4(const vec2<T> &v, T _z) noexcept : vec4(v, _z, static_cast<T>(1))
    {
    }

    vec4(const vec2<T> &v, T _z, T _w) noexcept : vec4(v.x, v.y, _z, _w)
    {
    }

    vec4(const vec3<T> &v) noexcept : vec4(v, static_cast<T>(1))
    {
    }

    vec4(const vec3<T> &v, T _w) noexcept : vec4(v.x, v.y, v.z, _w)
    {
    }

    vec4(const vec4 &v) noexcept : vec4(v._v)
    {
    }

    template<typename U>
    vec4(const vec2<U> &v) noexcept 
        : vec4(v.x, v.y, static_cast<U>(0), static_cast<U>(0))
    {
    }

    template<typename U>
    vec4(const vec3<U> &v) noexcept 
        : vec4(v.x, v.y, v.z, static_cast<U>(0))
    {
    }

    template<typename U>
    vec4(const vec4<U> &v) noexcept 
        : vec4(v.x, v.y, v.z, v.w)
    {
    }

    bool operator>(const vec4 &v) const { return x > v.x && y > v.y && z > v.z && w > v.w; }
    bool operator<(const vec4 &v) const { return x < v.x && y < v.y && z < v.z && w < v.w; }
    bool operator<=(const vec4 &v) const { return *this < v || *this == v; }
    bool operator>=(const vec4 &v) const { return *this > v || *this == v; }

    bool operator==(const vec4 &v) const { return this->compare(v); }
    bool operator!=(const vec4 &v) const { return !this->compare(v); }

    template<typename U>
    bool operator!=(const vec4<U> &v) const { return !this->compare(v); }
    template<typename U>
    bool operator==(const vec4<U> &v) const { return this->compare(v); }

    vec4 &operator=(T _f) noexcept { this->set(_f); return *this; }

    vec4 &operator=(const vec4& other) noexcept { this->set(other); return *this; }

    template<typename U>
    vec4 &operator=(U _f) noexcept { this->set(_f); return *this; }

    template<typename U>
    vec4 &operator=(const vec4& other) noexcept { this->set(other); return *this; }

    template<typename U>
    const vec4 operator-(U _f) const {
        return vec4(this->x - _f, this->y - _f, this->z - _f, this->w - _f);
    }

    template<typename U>
    const vec4 operator+(U _f) const {
        return vec4(this->x + _f, this->y + _f, this->z + _f, this->w + _f);
    }

    template<typename U>
    const vec4 operator*(U _f) const {
        return vec4(this->x * _f, this->y * _f, this->z * _f, this->w * _f);
    }

    template<typename U>
    const vec4 operator/(U _f) const {
        if (IS_ZERO(_f)) {
            return *this;
        }

        return vec4(static_cast<T>(this->x * (1.0f / _f)),
            static_cast<T>(this->y * (1.0f / _f)),
            static_cast<T>(this->z * (1.0f / _f)),
            static_cast<T>(this->w * (1.0f / _f)));
    }

    const vec4 operator-() const noexcept { return vec4(-x, -y, -z, -w); }

    template<typename U>
    const vec4 operator+(const vec4<U> &v) const noexcept {
        return vec4(this->x + v.x, this->y + v.y, this->z + v.z, this->w + v.w);
    }

    template<typename U>
    const vec4 operator-(const vec4<U> &v) const noexcept {
        return vec4(this->x - v.x, this->y - v.y, this->z - v.z, this->w - v.w);
    }

    template<typename U>
    const vec4 operator/(const vec4<U> &v) const noexcept {
        return vec4(IS_ZERO(v.x) ? this->x : this->x / v.x,
            IS_ZERO(v.y) ? this->y : this->y / v.y,
            IS_ZERO(v.z) ? this->z : this->z / v.z,
            IS_ZERO(v.w) ? this->w : this->w / v.w);
    }

    template<typename U>
    const vec4 operator*(const vec4<U>& v) const noexcept {
        return vec4(this->x * v.x,
                    this->y * v.y,
                    this->z * v.z,
                    this->w * v.w);
    }

    template<typename U>
    vec4 &operator+=(U _f) {
        this->set(*this + _f);
        return *this;
    }

    template<typename U>
    vec4 &operator-=(U _f) {
        this->set(*this - _f);
        return *this;
    }

    template<typename U>
    vec4 &operator*=(U _f) {
        this->set(*this * _f);
        return *this;
    }

    template<typename U>
    vec4 &operator/=(U _f) {
        this->set(*this / _f);
        return *this;
    }

    template<typename U>
    vec4 &operator*=(const vec4<U> &v) {
        this->set(*this * v);
        return *this;
    }

    template<typename U>
    vec4 &operator/=(const vec4<U> &v) {
        this->set(*this / v);
        return *this;
    }

    template<typename U>
    vec4 &operator+=(const vec4<U> &v) {
        this->set(*this + v);
        return *this;
    }

    template<typename U>
    vec4 &operator-=(const vec4<U> &v) {
        this->set(*this - v);
        return *this;
    }

    operator T *() { return this->_v; }
    operator const T *() const { return this->_v; }

    T &operator[](I32 i) { return this->_v[i]; }
    const T &operator[](I32 _i) const { return this->_v[_i]; }

    /// GLSL like accessors (const to prevent erroneous usage like .xyz() += n)
    inline const vec2<T> rg() const { return vec2<T>(this->r, this->g); }
    inline const vec2<T> xy() const { return this->rg(); }
    inline const vec2<T> rb() const { return vec2<T>(this->r, this->b); }
    inline const vec2<T> xz() const { return this->rb(); }
    inline const vec2<T> gb() const { return vec2<T>(this->g, this->b); }
    inline const vec2<T> yz() const { return this->gb(); }
    inline const vec2<T> ra() const { return vec2<T>(this->r, this->a); }
    inline const vec2<T> xw() const { return this->ra(); }
    inline const vec2<T> ga() const { return vec2<T>(this->g, this->a); }
    inline const vec2<T> yw() const { return this->ga(); }
    inline const vec2<T> ba() const { return vec2<T>(this->b, this->a); }
    inline const vec2<T> zw() const { return this->ba(); }
    inline const vec3<T> rgb() const { return vec3<T>(this->r, this->g, this->b); }
    inline const vec3<T> xyz() const { return this->rgb(); }
    inline const vec3<T> bgr() const { return vec3<T>(this->b, this->g, this->r); }
    inline const vec3<T> zyx() const { return this->bgr(); }
    inline const vec3<T> rga() const { return vec3<T>(this->r, this->g, this->a); }
    inline const vec3<T> xyw() const { return this->rga(); }
    inline const vec3<T> gba() const { return vec3<T>(this->g, this->b, this->a); }
    inline const vec3<T> yzw() const { return this->gba(); }

    inline void rg(const vec2<T> &rg) { this->set(rg); }
    inline void xy(const vec2<T> &xy) { this->set(xy); }
    inline void rb(const vec2<T> &rb) {
        this->xz(rb);
    }
    inline void xz(const vec2<T> &xz) {
        this->xz(xz.x, xz.y);
    }
    inline void gb(const vec2<T> &gb) {
        this->yz(gb);
    }
    inline void yz(const vec2<T> &yz) {
        this->yz(yz.x, yz.y);
    }
    inline void ra(const vec2<T> &ra) {
        this->xw(ra);
    }
    inline void xw(const vec2<T> &xw) {
        this->xw(xw.x, xw.y);
    }
    inline void ga(const vec2<T> &ga) {
        this->yw(ga);
    }
    inline void yw(const vec2<T> &yw) {
        this->yw(yw.x, yw.y);
    }
    inline void ba(const vec2<T> &ba) {
        this->zw(ba);
    }
    inline void zw(const vec2<T> &zw) {
        this->zw(zw.x, zw.y);
    }
    inline void rgb(const vec3<T> &rgb) {
        this->xyz(rgb);
    }
    inline void xyz(const vec3<T> &xyz) {
        this->xyz(xyz.x, xyz.y, xyz.z);
    }
    inline void bgr(const vec3<T> &bgr) {
        this->zyx(bgr);
    }
    inline void zyx(const vec3<T> &zyx) {
        this->z = zyx.x;
        this->y = zyx.y;
        this->x = zyx.z;
    }
    inline void rga(const vec3<T> &rga) {
        this->xyw(rga);
    }
    inline void xyw(const vec3<T> &xyw) {
        this->xyw(xyw.x, xyw.y, xyw.z);
    }
    inline void gba(const vec3<T> &gba) {
        this->yzw(gba);
    }
    inline void yzw(const vec3<T> &yzw) {
        this->yzw(yzw.x, yzw.y, yzw.z);
    }

    inline void xy(T _x, T _y) {
        this->x = _x;
        this->y = _y;
    }

    inline void xz(T _x, T _z) {
        this->x = _x;
        this->z = _z;
    }

    inline void xw(T _x, T _w) {
        this->x = _x;
        this->w = _w;
    }

    inline void yz(T _y, T _z) {
        this->y = _y;
        this->z = _z;
    }

    inline void yw(T _y, T _w) {
        this->y = _y;
        this->w = _w;
    }

    inline void zw(T _z, T _w) {
        this->z = _z;
        this->w = _w;
    }

    inline void xyz(T _x, T _y, T _z) {
        xy(_x, _y);
        this->z = _z;
    }

    inline void xyw(T _x, T _y, T _w) {
        xy(_x, _y);
        this->w = _w;
    }

    inline void xzw(T _x, T _z, T _w) {
        xz(_x, _z);
        this->w = _w;
    }

    inline void yzw(T _y, T _z, T _w) {
        yz(_y, _z);
        this->w = _w;
    }

    // special common case
    inline void xyz(const vec4<T> &xyzw) {
        this->xyz(xyzw.x, xyzw.y, xyzw.z);
    }
    /// set the 4 components of the vector manually using a source pointer to a (large enough) array
    inline void set(const T* v) noexcept { std::memcpy(_v, v, 4 * sizeof(T)); }
    /// set the 4 components of the vector manually
    inline void set(T value) { _reg = simd_vector<T>(value); }
    /// set the 4 components of the vector manually
    inline void set(T _x, T _y, T _z, T _w) noexcept {
        _reg = simd_vector<T>(_x, _y, _z, _w);
    }

    template <typename U>
    inline void set(U _x, U _y, U _z, U _w) {
        set(static_cast<T>(_x),
            static_cast<T>(_y),
            static_cast<T>(_z),
            static_cast<T>(_w));
    }

    /// set the 4 components of the vector using a source vector
    inline void set(const vec4 &v) { _reg = v._reg; }
    /// set the 4 components of the vector using a smaller source vector
    inline void set(const vec3<T> &v) { std::memcpy(&_v[0], &v._v[0], 3 * sizeof(T)); }
    /// set the 4 components of the vector using a smaller source vector
    inline void set(const vec3<T> &v, T _w) { std::memcpy(&_v[0], &v._v[0], 3 * sizeof(T)); w = _w; }
    /// set the 4 components of the vector using a smaller source vector
    inline void set(const vec2<T> &v) { std::memcpy(&_v[0], &v._v[0], 2 * sizeof(T)); }
    /// set the 4 components of the vector using smallers source vectors
    inline void set(const vec2<T> &v1, const vec2<T> &v2) {
        this->set(v1.x, v1.y, v2.x, v2.y);
    }
    /// set all the components back to 0
    inline void reset() { this->set((T)0); }
    /// compare 2 vectors
    template<typename U>
    inline bool compare(const vec4<U> &v) const;
    /// compare 2 vectors within the specified tolerance
    template<typename U>
    inline bool compare(const vec4<U> &v, U epsi) const;
    /// swap the components  of this vector with that of the specified one
    inline void swap(vec4 *iv) noexcept;
    /// swap the components  of this vector with that of the specified one
    inline void swap(vec4 &iv) noexcept;
    /// transform the vector to unit length
    inline vec4& normalize();
    /// calculate the dot product between this vector and the specified one
    inline T dot(const vec4 &v) const;
    /// return the vector's length
    inline T length() const { return Divide::Sqrt(lengthSquared()); }
    /// return the squared distance of the vector
    inline T lengthSquared() const;
    /// round all four values
    inline void round();
    /// lerp between this and the specified vector by the specified amount
    inline void lerp(const vec4 &v, T factor);
    /// lerp between this and the specified vector by the specified amount for
    /// each component
    inline void lerp(const vec4 &v, const vec4 &factor);
    union {
        struct {
            T x, y, z, w;
        };
        struct {
            T s, t, p, q;
        };
        struct {
            T r, g, b, a;
        };
        struct {
            T pitch, yaw, roll, _pad;
        };
        struct {
            T turn, move, zoom, _pad;
        };
        struct {
            T width, height, depth, _pad;
        };
        struct {
            T left, right, bottom, top;
        };
        struct {
            T fov, ratio, znear, zfar;
        };
        struct {
            T width, height, depth, key;
        };
        struct {
            T offsetX, offsetY, sizeX, sizeY;
        };
        T _v[4];
        simd_vector<T> _reg;
    };
};
#pragma pack(pop)

/// lerp between the 2 specified vectors by the specified amount
template <typename T>
inline vec4<T> Lerp(const vec4<T> &u, const vec4<T> &v, T factor);
/// lerp between the 2 specified vectors by the specified amount for each
/// component
template <typename T>
inline vec4<T> Lerp(const vec4<T> &u, const vec4<T> &v, const vec4<T> &factor);
/// min/max functions
template <typename T>
inline vec4<T> Min(const vec4<T> &v1, const vec4<T> &v2);
template <typename T>
inline vec4<T> Max(const vec4<T> &v1, const vec4<T> &v2);
template <typename T>
inline vec4<T> Normalize(vec4<T> &vector);
template <typename T>
inline vec4<T> Normalized(const vec4<T> &vector);
/// multiply a vector by a value
template <typename T>
inline vec4<T> operator*(T fl, const vec4<T> &v);

/// Quaternion multiplications require these to be floats
extern vec2<F32> VECTOR2_ZERO;
extern vec3<F32> VECTOR3_ZERO;
extern vec4<F32> VECTOR4_ZERO;
extern vec2<F32> VECTOR2_UNIT;
extern vec3<F32> VECTOR3_UNIT;
extern vec4<F32> VECTOR4_UNIT;
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

//ToDo: Move this to its own file
template<typename T>
class Rect : public vec4<T> {
  public:
    using vec4<T>::vec4;

    inline bool contains(const vec2<T>& coords) const {
        return contains(coords.x, coords.y);
    }

    inline bool contains(T _x, T _y) const {
        return COORDS_IN_RECT(_x, _y, *this);
    }

    template<typename U>
    inline bool contains(U _x, U _y) const {
        return COORDS_IN_RECT(static_cast<T>(_x), static_cast<T>(_y), *this);
    }

    inline vec2<T> clamp(T _x, T _y) const {
        CLAMP_IN_RECT(_x, _y, *this);
        return vec2<T>(_x, _y);
    }

    inline vec2<T> clamp(const vec2<T>& coords) const {
        return clamp(coords.x, coords.y);
    }
};

};  // namespace Divide

// Inline definitions
#include "MathVectors.inl"

#endif
