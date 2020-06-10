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

template <size_t Alignment = 16>
struct AlignedBase {
    void *operator new (size_t size)
    {
        return _mm_malloc(size, Alignment);
    }

    void *operator new[](size_t size)
    {
        return _mm_malloc(size, Alignment);
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

struct NonAlignedBase {
};

template<typename T, typename Enable = void>
class SimdVector;

template<typename T>
class SimdVector<T, std::enable_if_t<std::is_same<T, F32>::value>> {
public:
    SimdVector() noexcept : SimdVector(0) {}
    SimdVector(T reg0, T reg1, T reg2, T reg3)  noexcept : _reg(_mm_set_ps(reg3, reg2, reg1, reg0)) {}
    SimdVector(T reg)  noexcept : _reg(_mm_set_ps(reg, reg, reg, reg)) {}
    SimdVector(T reg[4])  noexcept : _reg(_mm_set_ps(reg[3], reg[2], reg[1], reg[0])) {}
    SimdVector(__m128 reg)  noexcept : _reg(reg) {}

    bool operator==(const SimdVector& other) const noexcept {
        return !AVX::Fneq128(_reg, other._reg, EPSILON_F32);
    }

    bool operator!=(const SimdVector& other) const noexcept {
        return AVX::Fneq128(_reg, other._reg, EPSILON_F32);
    }

    __m128 _reg;
};

template<typename T>
class SimdVector<T, std::enable_if_t<!std::is_same<T, F32>::value>> {
  public:
    SimdVector()  noexcept : SimdVector(0) {}
    SimdVector(T reg0, T reg1, T reg2, T reg3)  noexcept :  _reg{reg0, reg1, reg2, reg3} {}
    SimdVector(T val) noexcept : _reg{ val, val, val, val } {}
    SimdVector(T reg[4])  noexcept : _reg(reg) {}

    bool operator==(const SimdVector& other) const noexcept {
        for (U8 i = 0; i < 4; ++i) {
            if (_reg[i] != other._reg[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const SimdVector& other) const noexcept {
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
    static_assert(std::is_arithmetic<T>::value || std::is_same<T, bool>::value, "non-arithmetic vector type");

   public:
    vec2() noexcept : vec2(static_cast<T>(0)) {}
    vec2(T value) noexcept : vec2(value, value) {}
    template<typename U>
    vec2(U value) noexcept : vec2(value, value) {}
    vec2(T _x, T _y) noexcept : x(_x), y(_y) {}
    template <typename U>
    vec2(U _x, U _y) noexcept  : x(static_cast<T>(_x)), y(static_cast<T>(_y)) {}
    template <typename U, typename V>
    vec2(U _x, V _y) noexcept : x(static_cast<T>(_x)), y(static_cast<T>(_y)) {}
    vec2(const T *_v) noexcept : vec2(_v[0], _v[1]) {}
    vec2(const vec2 &_v) noexcept : vec2(_v._v) {}
    vec2(const vec3<T> &_v) noexcept : vec2(_v.xy()) {}
    vec2(const vec4<T> &_v) noexcept : vec2(_v.xy()) {}
    template<typename U>
    vec2(const vec2<U> &v) noexcept : vec2(v.x, v.y) {}
    template<typename U>
    vec2(const vec3<U> &v) noexcept : vec2(v.x, v.y) {}
    template<typename U>
    vec2(const vec4<U> &v) noexcept : vec2(v.x, v.y) {}

    template<typename U>
    vec2 &operator=(U _f) noexcept { this->set(_f); return (*this); }
    template<typename U>
    vec2 &operator=(const vec2<U>& other) noexcept { this->set(other); return (*this); }

    bool operator> (const vec2 &v) const noexcept { return x > v.x && y > v.y; }
    bool operator< (const vec2 &v) const noexcept { return x < v.x && y < v.y; }
    bool operator<=(const vec2 &v) const noexcept { return *this < v || *this == v; }
    bool operator>=(const vec2 &v) const noexcept { return *this > v || *this == v; }
    bool operator==(const vec2 &v) const noexcept { return this->compare(v); }
    bool operator!=(const vec2 &v) const noexcept { return !this->compare(v); }
    template<typename U>
    bool operator!=(const vec2<U> &v) const noexcept { return !this->compare(v); }
    template<typename U>
    bool operator==(const vec2<U> &v) const noexcept { return this->compare(v); }

    template<typename U>
    [[nodiscard]] vec2 operator-(U _f) const noexcept { return vec2(this->x - _f, this->y - _f); }
    template<typename U>
    [[nodiscard]] vec2 operator+(U _f) const noexcept { return vec2(this->x + _f, this->y + _f); }
    template<typename U>
    [[nodiscard]] vec2 operator*(U _f) const noexcept { return vec2(this->x * _f, this->y * _f); }
    template<typename U>
    [[nodiscard]] vec2 operator/(U _i) const noexcept { if (!IS_ZERO(_i)) { return vec2(this->x / _i, this->y / _i); } return *this; }

    template<typename U>
    [[nodiscard]] vec2 operator+(const vec2<U> &v) const noexcept { return vec2(this->x + v.x, this->y + v.y); }
    template<typename U>
    [[nodiscard]] vec2 operator-(const vec2<U> &v) const noexcept { return vec2(this->x - v.x, this->y - v.y); }
    template<typename U>
    [[nodiscard]] vec2 operator*(const vec2<U> &v) const noexcept { return vec2(this->x * v.x, this->y * v.y); }

    [[nodiscard]] vec2 operator-() const noexcept { return vec2(-this->x, -this->y); }
 
    template<typename U>
    vec2 &operator+=(U _f) noexcept { this->set(*this + _f); return *this; }
    template<typename U>
    vec2 &operator-=(U _f) noexcept { this->set(*this - _f); return *this; }
    template<typename U>
    vec2 &operator*=(U _f) noexcept { this->set(*this * _f); return *this; }
    template<typename U>
    vec2 &operator/=(U _f) noexcept { this->set(*this / _f); return *this; }

    template<typename U>
    vec2 &operator*=(const vec2<U> &v) noexcept { this->set(*this * v); return *this; }
    template<typename U>
    vec2 &operator+=(const vec2<U> &v) noexcept { this->set(*this + v); return *this; }
    template<typename U>
    vec2 &operator-=(const vec2<U> &v) noexcept { this->set(*this - v); return *this; }
    template<typename U>
    vec2 &operator/=(const vec2<U> &v) noexcept { this->set(*this / v); return *this; }

    [[nodiscard]]       T &operator[](I32 i)       noexcept { return this->_v[i]; }
    [[nodiscard]] const T &operator[](I32 i) const noexcept { return this->_v[i]; }

    [[nodiscard]] vec2 operator/(const vec2 &v) const noexcept {
        return vec2(IS_ZERO(v.x) ? this->x : this->x / v.x,
                    IS_ZERO(v.y) ? this->y : this->y / v.y);
    }

    [[nodiscard]] operator       T *()       { return this->_v; }
    [[nodiscard]] operator const T *() const { return this->_v; }

    /// swap the components  of this vector with that of the specified one
    void swap(vec2 *iv) noexcept { std::swap(this->x, iv->x); std::swap(this->x, iv->x); }
    /// swap the components  of this vector with that of the specified one
    void swap(vec2 &iv) noexcept { std::swap(this->x, iv.x); std::swap(this->x, iv.x); }
    /// set the 2 components of the vector manually using a source pointer to a (large enough) array
    void set(const T* v) noexcept { std::memcpy(&_v[0], &v[0], sizeof(T) * 2); }
    /// set the 2 components of the vector manually
    void set(T value) noexcept { this->set(value, value); }
    /// set the 2 components of the vector manually
    void set(T _x, T _y) noexcept { this->x = _x; this->y = _y; }
    template <typename U> 
    void set(U _x, U _y) noexcept { this->x = static_cast<T>(_x); this->y = static_cast<T>(_y); }
    /// set the 2 components of the vector using a source vector
    void set(const vec2 &v) noexcept { this->set(&v._v[0]); }
    /// set the 2 components of the vector using the first 2 components of the source vector
    void set(const vec3<T> &v) noexcept { this->set(v.x, v.y); }
    /// set the 2 components of the vector using the first 2 components of the source vector
    void set(const vec4<T> &v) noexcept { this->set(v.x, v.y); }
    /// set the 2 components of the vector back to 0
    void reset() { this->set(static_cast<T>(0)); }
    /// return the vector's length
    [[nodiscard]] T length()        const          { return Divide::Sqrt(lengthSquared()); }
    [[nodiscard]] T lengthSquared() const noexcept;
    /// return the angle defined by the 2 components
    [[nodiscard]] T angle() const { return static_cast<T>(std::atan2(this->y, this->x)); }
    /// return the angle defined by the 2 components
    [[nodiscard]] T angle(const vec2 &v) const { return static_cast<T>(std::atan2(v.y - this->y, v.x - this->x)); }
    /// compute the vector's distance to another specified vector
    [[nodiscard]] T distance(const vec2 &v) const;
    /// compute the vector's squared distance to another specified vector
    [[nodiscard]] T distanceSquared(const vec2 &v) const noexcept;
    /// convert the vector to unit length
    [[nodiscard]] vec2& normalize();
    /// get the smallest value of X or Y
    [[nodiscard]] T minComponent() const noexcept;
    /// get the largest value of X or Y
    [[nodiscard]] T maxComponent() const noexcept;
    /// round both values
    void round();
    /// lerp between this and the specified vector by the specified amount
    void lerp(const vec2 &v, T factor) noexcept;
    /// lerp between this and the specified vector by the specified amount for each component
    void lerp(const vec2 &v, const vec2 &factor) noexcept;
    /// calculate the dot product between this vector and the specified one
    [[nodiscard]] T dot(const vec2 &v) const noexcept;
    /// project this vector on the line defined by the 2 points(A, B)
    [[nodiscard]] T projectionOnLine(const vec2 &vA, const vec2 &vB) const;
    /// return the closest point on the line defined by the 2 points (A, B) and this vector
    [[nodiscard]] vec2 closestPointOnLine(const vec2 &vA, const vec2 &vB);
    /// return the closest point on the line segment defined between the 2 points (A, B) and this vector
    [[nodiscard]] vec2 closestPointOnSegment(const vec2 &vA, const vec2 &vB);
    /// compare 2 vectors
    template<typename U>
    [[nodiscard]] bool compare(const vec2<U> &_v) const noexcept;
    /// compare 2 vectors within the specified tolerance
    template<typename U>
    [[nodiscard]] bool compare(const vec2<U> &_v, U epsi) const noexcept;
    /// export the vector's components in the first 2 positions of the specified array
    [[nodiscard]] void get(T *v) const;

    union {
        struct { T x, y; };
        struct { T s, t; };
        struct { T width, height; };
        struct { T min, max; };
        T _v[2];
    };
};

/// lerp between the 2 specified vectors by the specified amount
template <typename T, typename U>
[[nodiscard]] vec2<T> Lerp(const vec2<T> &u, const vec2<T> &v, U factor) noexcept;
/// lerp between the 2 specified vectors by the specified amount for each component
template <typename T>
[[nodiscard]] vec2<T> Lerp(const vec2<T> &u, const vec2<T> &v, const vec2<T> &factor) noexcept;
template <typename T>
[[nodiscard]] vec2<T> Cross(const vec2<T> &v1, const vec2<T> &v2) noexcept;
template <typename T>
[[nodiscard]] vec2<T> Inverse(const vec2<T> &v) noexcept;
template <typename T>
[[nodiscard]] vec2<T> Normalize(vec2<T> &vector);
template <typename T>
[[nodiscard]] vec2<T> Normalized(const vec2<T> &vector);
template <typename T>
[[nodiscard]] T Dot(const vec2<T> &a, const vec2<T> &b) noexcept;
template <typename T>
[[nodiscard]] void OrthoNormalize(vec2<T> &v1, vec2<T> &v2);
/// multiply a vector by a value
template <typename T>
[[nodiscard]] vec2<T> operator*(T fl, const vec2<T> &v) noexcept;


/***********************************************************************
 * vec3 -  A 3-tuple used to represent things like a vector in 3D space,
 * a point in 3D space or just 3 values linked together
 ***********************************************************************/
template <typename T>
class vec3 {
    static_assert(std::is_arithmetic<T>::value || std::is_same<T, bool>::value, "non-arithmetic vector type");
   public:
    vec3() noexcept : vec3(static_cast<T>(0)) {}
    vec3(T value) noexcept : vec3(value, value, value) {}
    template<typename U>
    vec3(U value) noexcept : vec3(value, value, value) {}
    vec3(T _x, T _y, T _z) noexcept : x(_x), y(_y), z(_z) {}
    template <typename U>
    vec3(U _x, U _y, U _z)  noexcept  : x(static_cast<T>(_x)), y(static_cast<T>(_y)), z(static_cast<T>(_z)) {}
    template <typename U, typename V>
    vec3(U _x, U _y, V _z) noexcept : x(static_cast<T>(_x)), y(static_cast<T>(_y)), z(static_cast<T>(_z)) {}
    template <typename U, typename V>
    vec3(U _x, V _y, V _z) noexcept : x(static_cast<T>(_x)), y(static_cast<T>(_y)), z(static_cast<T>(_z)) {}
    template <typename U, typename V, typename W>
    vec3(U _x, V _y, W _z) noexcept : x(static_cast<T>(_x)), y(static_cast<T>(_y)), z(static_cast<T>(_z)) {}
    vec3(const T *v) noexcept : vec3(v[0], v[1], v[2]) {}
    vec3(const vec2<T> &v) noexcept : vec3(v, static_cast<T>(0)) {}
    vec3(const vec2<T> &v, T _z) noexcept : vec3(v.x, v.y, _z) {}
    vec3(const vec3 &v) noexcept : vec3(v._v) {}
    vec3(const vec4<T> &v) noexcept : vec3(v.x, v.y, v.z) {}
    template<typename U>
    vec3(const vec2<U> &v) noexcept : vec3(v.x, v.y, static_cast<U>(0)) {}
    template<typename U>
    vec3(const vec3<U> &v) noexcept : vec3(v.x, v.y, v.z) {}
    template<typename U>
    vec3(const vec4<U> &v) noexcept : vec3(v.x, v.y, v.z) {}

    [[nodiscard]] bool operator> (const vec3 &v) const noexcept { return x > v.x && y > v.y && z > v.z; }
    [[nodiscard]] bool operator< (const vec3 &v) const noexcept { return x < v.x && y < v.y && z < v.z; }
    [[nodiscard]] bool operator<=(const vec3 &v) const noexcept { return *this < v || *this == v; }
    [[nodiscard]] bool operator>=(const vec3 &v) const noexcept { return *this > v || *this == v; }
    [[nodiscard]] bool operator!=(const vec3 &v) const noexcept { return !this->compare(v); }
    [[nodiscard]] bool operator==(const vec3 &v) const noexcept { return this->compare(v); }
    template<typename U>
    [[nodiscard]] bool operator!=(const vec3<U> &v) const noexcept { return !this->compare(v); }
    template<typename U>
    [[nodiscard]] bool operator==(const vec3<U> &v) const noexcept { return this->compare(v); }

    template <typename U>
    vec3 &operator=(U _f) noexcept { this->set(_f); return (*this); }
    template <typename U>
    vec3 &operator=(const vec3<U>& other) noexcept { this->set(other); return (*this); }
    template <typename U>
    [[nodiscard]] vec3 operator+(U _f) const noexcept { return vec3(this->x + _f, this->y + _f, this->z + _f); }
    template <typename U>
    [[nodiscard]] vec3 operator-(U _f) const noexcept { return vec3(this->x - _f, this->y - _f, this->z - _f); }
    template <typename U>
    [[nodiscard]] vec3 operator*(U _f) const noexcept { return vec3(this->x * _f, this->y * _f, this->z * _f); }
    template <typename U>
    [[nodiscard]] vec3 operator/(U _f) const noexcept { if (IS_ZERO(_f)) { return *this; } return vec3(this->x / _f, this->y / _f, this->z / _f); }

    [[nodiscard]] vec3 operator-() const noexcept { return vec3(-this->x, -this->y, -this->z); }

    template <typename U>
    [[nodiscard]] vec3 operator+(const vec3<U> &v) const noexcept { return vec3(this->x + v.x, this->y + v.y, this->z + v.z); }
    template <typename U>
    [[nodiscard]] vec3 operator-(const vec3<U> &v) const noexcept { return vec3(this->x - v.x, this->y - v.y, this->z - v.z); }
    template<typename U>
    [[nodiscard]] vec3 operator*(const vec3<U> &v) const noexcept { return vec3(this->x * v.x, this->y * v.y, this->z * v.z); }

    template<typename U>
    vec3 &operator+=(U _f) noexcept { this->set(*this + _f); return *this; }
    template<typename U>
    vec3 &operator-=(U _f) noexcept { this->set(*this - _f); return *this; }
    template<typename U>
    vec3 &operator*=(U _f) noexcept { this->set(*this * _f); return *this; }
    template<typename U>
    vec3 &operator/=(U _f) noexcept { this->set(*this / _f); return *this; }

    template<typename U>
    vec3 &operator*=(const vec3<U> &v) noexcept { this->set(*this * v); return *this; }
    template<typename U>
    vec3 &operator+=(const vec3<U> &v) noexcept { this->set(*this + v); return *this; }
    template<typename U>
    vec3 &operator-=(const vec3<U> &v) noexcept { this->set(*this - v); return *this; }
    template<typename U>
    vec3 &operator/=(const vec3<U> &v) noexcept { this->set(*this / v); return *this; }

    [[nodiscard]] T &operator[](const I32 i) noexcept { return this->_v[i]; }

    template<typename U>
    [[nodiscard]] vec3 operator/(const vec3<U> &v) const noexcept {
        return vec3(IS_ZERO(v.x) ? this->x : this->x / v.x,
                    IS_ZERO(v.y) ? this->y : this->y / v.y,
                    IS_ZERO(v.z) ? this->z : this->z / v.z);
    }

    [[nodiscard]] operator T *()  noexcept { return this->_v; }
    [[nodiscard]] operator const T *() const  noexcept { return this->_v; }

    /// GLSL like accessors (const to prevent erroneous usage like .xy() += n)
    [[nodiscard]] vec2<T> rg() const noexcept { return vec2<T>(this->r, this->g); }
    [[nodiscard]] vec2<T> xy() const noexcept { return this->rg(); }
    [[nodiscard]] vec2<T> rb() const noexcept { return vec2<T>(this->r, this->b); }
    [[nodiscard]] vec2<T> xz() const noexcept { return this->rb(); }
    [[nodiscard]] vec2<T> gb() const noexcept { return vec2<T>(this->g, this->b); }
    [[nodiscard]] vec2<T> yz() const noexcept { return this->gb(); }

    void rg(const vec2<T> &rg) noexcept { this->set(rg); }
    void xy(const vec2<T> &xy) noexcept { this->set(xy); }
    void rb(const vec2<T> &rb) noexcept { this->r = rb.x; this->b = rb.y; }
    void xz(const vec2<T> &xz) noexcept { this->x = xz.x; this->z = xz.y; }
    void gb(const vec2<T> &gb) noexcept { this->g = gb.x; this->b = gb.y; }
    void yz(const vec2<T> &yz) noexcept { this->y = yz.x; this->z = yz.y; }

    /// set the 3 components of the vector manually using a source pointer to a (large enough) array
    void set(const T* v) noexcept { std::memcpy(&_v[0], &v[0], 3 * sizeof(T)); }
    /// set the 3 components of the vector manually
    void set(T value)  noexcept { x = value; y = value; z = value; }
    /// set the 3 components of the vector manually
    void set(T _x, T _y, T _z) noexcept { this->x = _x; this->y = _y; this->z = _z; }
    template <typename U> 
    void set(U _x, U _y, U _z) noexcept { this->x = static_cast<T>(_x); this->y = static_cast<T>(_y); this->z = static_cast<T>(_z); }
    /// set the 3 components of the vector using a smaller source vector
    void set(const vec2<T> &v)  noexcept { std::memcpy(_v, v._v, 2 * sizeof(T)); }
    /// set the 3 components of the vector using a source vector
    void set(const vec3 &v)  noexcept { std::memcpy(_v, v._v, 3 * sizeof(T)); }
    /// set the 3 components of the vector using the first 3 components of the source vector
    void set(const vec4<T> &v)  noexcept { std::memcpy(_v, v._v, 3 * sizeof(T)); }
    /// set all the components back to 0
    void reset()  noexcept { std::memset(_v, 0, 3 * sizeof(T)); }
    /// return the vector's length
    [[nodiscard]] T length() const  noexcept { return Divide::Sqrt(lengthSquared()); }
    /// return true if length is zero
    [[nodiscard]] bool isZeroLength() const  noexcept { return lengthSquared() < EPSILON_F32; }
    /// compare 2 vectors
    template<typename U>
    [[nodiscard]] bool compare(const vec3<U> &v) const noexcept;
    /// compare 2 vectors within the specified tolerance
    template<typename U>
    [[nodiscard]] bool compare(const vec3<U> &v, U epsi) const noexcept;
    /// uniform vector: x = y = z
    [[nodiscard]] bool isUniform() const noexcept;
    /// return the squared distance of the vector
    [[nodiscard]] T lengthSquared() const noexcept;
    /// calculate the dot product between this vector and the specified one
    [[nodiscard]] T dot(const vec3 &v) const noexcept;
    /// returns the angle in radians between '*this' and 'v'
    [[nodiscard]] T angle(vec3 &v) const;
    /// compute the vector's distance to another specified vector
    [[nodiscard]] T distance(const vec3 &v) const noexcept;
    /// compute the vector's squared distance to another specified vector
    [[nodiscard]] T distanceSquared(const vec3 &v) const noexcept;
    /// transform the vector to unit length
    vec3& normalize();
    /// get the smallest value of X,Y or Z
    [[nodiscard]] T minComponent() const  noexcept;
    /// get the largest value of X,Y or Z
    [[nodiscard]] T maxComponent() const  noexcept;
    /// round all three values
    void round();
    /// project this vector on the line defined by the 2 points(A, B)
    [[nodiscard]] T projectionOnLine(const vec3 &vA, const vec3 &vB) const;
    /// return the closest point on the line defined by the 2 points (A, B) and this
    /// vector
    [[nodiscard]] vec3 closestPointOnLine(const vec3 &vA, const vec3 &vB);
    /// return the closest point on the line segment created between the 2 points
    /// (A, B) and this vector
    [[nodiscard]] vec3 closestPointOnSegment(const vec3 &vA, const vec3 &vB);
    /// get the direction vector to the specified point
    [[nodiscard]] vec3 direction(const vec3 &u) const;
    /// lerp between this and the specified vector by the specified amount
    void lerp(const vec3 &v, T factor) noexcept;
    /// lerp between this and the specified vector by the specified amount for
    /// each component
    void lerp(const vec3 &v, const vec3 &factor) noexcept;
    /// this calculates a vector between the two specified points and returns
    /// the result
    [[nodiscard]] vec3 vector(const vec3 &vp1, const vec3 &vp2) const noexcept;
    /// set this vector to be equal to the cross of the 2 specified vectors
    void cross(const vec3 &v1, const vec3 &v2) noexcept;
    /// set this vector to be equal to the cross between itself and the
    /// specified vector
    void cross(const vec3 &v2) noexcept;
    /// rotate this vector on the X axis
    void rotateX(D64 radians);
    /// rotate this vector on the Y axis
    void rotateY(D64 radians);
    /// rotate this vector on the Z axis
    void rotateZ(D64 radians);
    /// swap the components  of this vector with that of the specified one
    void swap(vec3 &iv) noexcept;
    /// swap the components  of this vector with that of the specified one
    void swap(vec3 *iv) noexcept;
    /// export the vector's components in the first 3 positions of the specified
    /// array
    void get(T *v) const noexcept;

    union {
        struct { T x, y, z; };
        struct { T s, t, p; };
        struct { T r, g, b; };
        struct { T pitch, yaw, roll; };
        struct { T turn, move, zoom; };
        struct { T width, height, depth; };
        T _v[3];
    };
};

/// lerp between the 2 specified vectors by the specified amount
template <typename T, typename U>
[[nodiscard]] vec3<T> Lerp(const vec3<T> &u, const vec3<T> &v, U factor) noexcept;
/// lerp between the 2 specified vectors by the specified amount for each component
template <typename T>
[[nodiscard]] vec3<T> Lerp(const vec3<T> &u, const vec3<T> &v, const vec3<T> &factor) noexcept;
template <typename T>
[[nodiscard]] vec3<T> Abs(const vec3<T> &vector) noexcept;
template <typename T>
[[nodiscard]] vec3<T> Min(const vec3<T> &v1, const vec3<T> &v2) noexcept;
template <typename T>
[[nodiscard]] vec3<T> Max(const vec3<T> &v1, const vec3<T> &v2) noexcept;
template <typename T>
[[nodiscard]] vec3<T> Normalize(vec3<T> &vector);
template <typename T>
[[nodiscard]] vec3<T> Normalized(const vec3<T> &vector);
/// general vec3 dot product
template <typename T>
[[nodiscard]] T Dot(const vec3<T> &a, const vec3<T> &b) noexcept;
/// general vec3 cross function
template <typename T>
[[nodiscard]] vec3<T> Cross(const vec3<T> &v1, const vec3<T> &v2) noexcept;
template <typename T>
[[nodiscard]] vec3<T> Inverse(const vec3<T> &v) noexcept;
template <typename T>
[[nodiscard]] vec3<T> operator*(T fl, const vec3<T> &v) noexcept;
template <typename T>
void OrthoNormalize(vec3<T> &v1, vec3<T> &v2);
template<typename T>
[[nodiscard]] vec3<T> Perpendicular(const vec3<T> &v) noexcept;

/*************************************************************************************
 * vec4 -  A 4-tuple used to represent things like a vector in 4D space
(w-component)
 * or just 4 values linked together
 ************************************************************************************/
#pragma pack(push)
#pragma pack(1)
//__declspec(align(alignment))
template <typename T>
class vec4 : public std::conditional<std::is_same<T, F32>::value, AlignedBase<16>, NonAlignedBase>::type {
    static_assert(std::is_arithmetic<T>::value || std::is_same<T, bool>::value, "non-arithmetic vector type");

   public:
    vec4() noexcept : x(0), y(0), z(0), w(1) {}
    vec4(T _x, T _y, T _z, T _w) noexcept  : x(_x), y(_y), z(_z), w(_w) {}
    vec4(T _x, T _y, T _z) noexcept : x(_x), y(_y), z(_z), w(static_cast<T>(1)) {}
    template<typename U>
    vec4(U _x, U _y, U _z, U _w) noexcept  : x(static_cast<T>(_x)), y(static_cast<T>(_y)), z(static_cast<T>(_z)), w(static_cast<T>(_w)) {}
    template<typename U>
    vec4(U _x, U _y, U _z) noexcept : vec4(_x, _y, _z, static_cast<T>(1)) {}
    vec4(__m128 reg) noexcept: _reg(reg) {}
    vec4(const SimdVector<T>& reg) noexcept: _reg(reg) {}
    vec4(T value) noexcept : vec4(value, value, value, value) {}
    template<typename U>
    vec4(U value) noexcept : vec4(value, value, value, value) {}
    vec4(const T *v) noexcept : vec4(v[0], v[1], v[2], v[3]) {}
    vec4(const vec2<T> &v) noexcept : vec4(v, static_cast<T>(0)) {}
    vec4(const vec2<T> &v, T _z) noexcept : vec4(v, _z, static_cast<T>(1)) {}
    vec4(const vec2<T> &v, T _z, T _w) noexcept : vec4(v.x, v.y, _z, _w) {}
    vec4(const vec3<T> &v) noexcept : vec4(v, static_cast<T>(1)) {}
    vec4(const vec3<T> &v, T _w) noexcept : vec4(v.x, v.y, v.z, _w) {}
    vec4(const vec4 &v) noexcept : vec4(v._v) {}
    template<typename U>
    vec4(const vec2<U> &v) noexcept : vec4(v.x, v.y, static_cast<U>(0), static_cast<U>(0)) {}
    template<typename U>
    vec4(const vec3<U> &v) noexcept : vec4(v.x, v.y, v.z, static_cast<U>(0)) {}
    template<typename U>
    vec4(const vec4<U> &v) noexcept : vec4(v.x, v.y, v.z, v.w) {}

    [[nodiscard]] bool operator> (const vec4 &v) const noexcept { return x > v.x && y > v.y && z > v.z && w > v.w; }
    [[nodiscard]] bool operator< (const vec4 &v) const noexcept { return x < v.x && y < v.y && z < v.z && w < v.w; }
    [[nodiscard]] bool operator<=(const vec4 &v) const noexcept { return *this < v || *this == v; }
    [[nodiscard]] bool operator>=(const vec4 &v) const noexcept { return *this > v || *this == v; }
    [[nodiscard]] bool operator==(const vec4 &v) const noexcept { return this->compare(v); }
    [[nodiscard]] bool operator!=(const vec4 &v) const noexcept { return !this->compare(v); }
    template<typename U>
    [[nodiscard]] bool operator!=(const vec4<U> &v) const noexcept { return !this->compare(v); }
    template<typename U>
    [[nodiscard]] bool operator==(const vec4<U> &v) const noexcept { return this->compare(v); }

    vec4 &operator=(T _f) noexcept { this->set(_f); return *this; }
    vec4 &operator=(const vec4& other) noexcept { this->set(other); return *this; }
    template<typename U>
    vec4 &operator=(U _f) noexcept { this->set(_f); return *this; }
    template<typename U>
    vec4 &operator=(const vec4& other) noexcept { this->set(other); return *this; }

    template<typename U>
    [[nodiscard]] vec4 operator-(U _f) const noexcept { return vec4(this->x - _f, this->y - _f, this->z - _f, this->w - _f); }
    template<typename U>
    [[nodiscard]] vec4 operator+(U _f) const noexcept { return vec4(this->x + _f, this->y + _f, this->z + _f, this->w + _f); }
    template<typename U>
    [[nodiscard]] vec4 operator*(U _f) const noexcept { return vec4(this->x * _f, this->y * _f, this->z * _f, this->w * _f); }
    template<typename U>
    [[nodiscard]] vec4 operator/(U _f) const noexcept {
        if (!IS_ZERO(_f)) {
            return vec4(static_cast<T>(this->x / _f),
                        static_cast<T>(this->y / _f),
                        static_cast<T>(this->z / _f),
                        static_cast<T>(this->w / _f));
        }

        return *this;
    }

    [[nodiscard]] vec4 operator-() const noexcept { return vec4(-x, -y, -z, -w); }

    template<typename U>
    [[nodiscard]] vec4 operator+(const vec4<U> &v) const noexcept { return vec4(this->x + v.x, this->y + v.y, this->z + v.z, this->w + v.w); }
    template<typename U>
    [[nodiscard]] vec4 operator-(const vec4<U> &v) const noexcept { return vec4(this->x - v.x, this->y - v.y, this->z - v.z, this->w - v.w); }
    template<typename U>
    [[nodiscard]] vec4 operator*(const vec4<U>& v) const noexcept { return vec4(this->x * v.x, this->y * v.y, this->z * v.z, this->w * v.w); }
    template<typename U>
    [[nodiscard]] vec4 operator/(const vec4<U> &v) const noexcept {
        return vec4(IS_ZERO(v.x) ? this->x : this->x / v.x,
                    IS_ZERO(v.y) ? this->y : this->y / v.y,
                    IS_ZERO(v.z) ? this->z : this->z / v.z,
                    IS_ZERO(v.w) ? this->w : this->w / v.w);
    }

    template<typename U>
    vec4 &operator+=(U _f) noexcept { this->set(*this + _f); return *this; }
    template<typename U>
    vec4 &operator-=(U _f) noexcept { this->set(*this - _f); return *this; }
    template<typename U>
    vec4 &operator*=(U _f) noexcept { this->set(*this * _f); return *this; }
    template<typename U>
    vec4 &operator/=(U _f) noexcept { if (!IS_ZERO(_f)) { this->set(*this / _f); } return *this; }

    template<typename U>
    vec4 &operator*=(const vec4<U> &v) noexcept { this->set(*this * v); return *this; }
    template<typename U>
    vec4 &operator/=(const vec4<U> &v) noexcept { this->set(*this / v); return *this; }
    template<typename U>
    vec4 &operator+=(const vec4<U> &v) noexcept { this->set(*this + v); return *this; }
    template<typename U>
    vec4 &operator-=(const vec4<U> &v) noexcept { this->set(*this - v); return *this; }

    [[nodiscard]] operator T *() { return this->_v; }
    [[nodiscard]] operator const T *() const { return this->_v; }

    [[nodiscard]] T &operator[](I32 i) noexcept { return this->_v[i]; }
    [[nodiscard]] const T &operator[](I32 _i) const noexcept { return this->_v[_i]; }

    /// GLSL like accessors (const to prevent erroneous usage like .xyz() += n)
    [[nodiscard]] vec2<T> rg()  const noexcept { return vec2<T>(this->r, this->g); }
    [[nodiscard]] vec2<T> xy()  const noexcept { return this->rg(); }
    [[nodiscard]] vec2<T> rb()  const noexcept { return vec2<T>(this->r, this->b); }
    [[nodiscard]] vec2<T> xz()  const noexcept { return this->rb(); }
    [[nodiscard]] vec2<T> gb()  const noexcept { return vec2<T>(this->g, this->b); }
    [[nodiscard]] vec2<T> yz()  const noexcept { return this->gb(); }
    [[nodiscard]] vec2<T> ra()  const noexcept { return vec2<T>(this->r, this->a); }
    [[nodiscard]] vec2<T> xw()  const noexcept { return this->ra(); }
    [[nodiscard]] vec2<T> ga()  const noexcept { return vec2<T>(this->g, this->a); }
    [[nodiscard]] vec2<T> yw()  const noexcept { return this->ga(); }
    [[nodiscard]] vec2<T> ba()  const noexcept { return vec2<T>(this->b, this->a); }
    [[nodiscard]] vec2<T> zw()  const noexcept { return this->ba(); }
    [[nodiscard]] vec3<T> rgb() const noexcept { return vec3<T>(this->r, this->g, this->b); }
    [[nodiscard]] vec3<T> xyz() const noexcept { return this->rgb(); }
    [[nodiscard]] vec3<T> bgr() const noexcept { return vec3<T>(this->b, this->g, this->r); }
    [[nodiscard]] vec3<T> zyx() const noexcept { return this->bgr(); }
    [[nodiscard]] vec3<T> rga() const noexcept { return vec3<T>(this->r, this->g, this->a); }
    [[nodiscard]] vec3<T> xyw() const noexcept { return this->rga(); }
    [[nodiscard]] vec3<T> gba() const noexcept { return vec3<T>(this->g, this->b, this->a); }
    [[nodiscard]] vec3<T> yzw() const noexcept { return this->gba(); }

    void rg(const vec2<T> &rg)  noexcept { this->set(rg); }
    void xy(const vec2<T> &xy)  noexcept { this->set(xy); }
    void rb(const vec2<T> &rb)  noexcept { this->xz(rb); }
    void xz(const vec2<T> &xz)  noexcept { this->xz(xz.x, xz.y);}
    void gb(const vec2<T> &gb)  noexcept { this->yz(gb); }
    void yz(const vec2<T> &yz)  noexcept { this->yz(yz.x, yz.y); }
    void ra(const vec2<T> &ra)  noexcept { this->xw(ra); }
    void xw(const vec2<T> &xw)  noexcept { this->xw(xw.x, xw.y); }
    void ga(const vec2<T> &ga)  noexcept { this->yw(ga); }
    void yw(const vec2<T> &yw)  noexcept { this->yw(yw.x, yw.y); }
    void ba(const vec2<T> &ba)  noexcept { this->zw(ba); }
    void zw(const vec2<T> &zw)  noexcept { this->zw(zw.x, zw.y); }
    void rgb(const vec3<T> &rgb) noexcept { this->xyz(rgb); }
    void xyz(const vec3<T> &xyz) noexcept { this->xyz(xyz.x, xyz.y, xyz.z); }
    void bgr(const vec3<T> &bgr) noexcept { this->zyx(bgr); }
    void zyx(const vec3<T> &zyx) noexcept { this->z = zyx.x; this->y = zyx.y; this->x = zyx.z; }
    void rga(const vec3<T> &rga) noexcept { this->xyw(rga); }
    void xyw(const vec3<T> &xyw) noexcept { this->xyw(xyw.x, xyw.y, xyw.z); }
    void gba(const vec3<T> &gba) noexcept { this->yzw(gba); }
    void yzw(const vec3<T> &yzw) noexcept { this->yzw(yzw.x, yzw.y, yzw.z); }
    void xy(T _x, T _y) noexcept { this->x = _x; this->y = _y; }
    void xz(T _x, T _z) noexcept { this->x = _x; this->z = _z; }
    void xw(T _x, T _w) noexcept { this->x = _x; this->w = _w; }
    void yz(T _y, T _z) noexcept { this->y = _y; this->z = _z; }
    void yw(T _y, T _w) noexcept { this->y = _y; this->w = _w; }
    void zw(T _z, T _w) noexcept { this->z = _z; this->w = _w; }
    void xyz(T _x, T _y, T _z) noexcept { xy(_x, _y); this->z = _z; }
    void xyw(T _x, T _y, T _w) noexcept { xy(_x, _y); this->w = _w; }
    void xzw(T _x, T _z, T _w) noexcept { xz(_x, _z); this->w = _w; }
    void yzw(T _y, T _z, T _w) noexcept { yz(_y, _z); this->w = _w; }
    // special common case
    void xyz(const vec4<T> &xyzw) noexcept { this->xyz(xyzw.x, xyzw.y, xyzw.z); }
    /// set the 4 components of the vector manually using a source pointer to a (large enough) array
    void set(const T* v) noexcept { std::memcpy(_v, v, 4 * sizeof(T)); }
    /// set the 4 components of the vector manually
    void set(T value) noexcept { _reg = SimdVector<T>(value); }
    /// set the 4 components of the vector manually
    void set(T _x, T _y, T _z, T _w) noexcept { _reg = SimdVector<T>(_x, _y, _z, _w); }
    template <typename U>
    void set(U _x, U _y, U _z, U _w) noexcept { set(static_cast<T>(_x), static_cast<T>(_y), static_cast<T>(_z), static_cast<T>(_w)); }
    /// set the 4 components of the vector using a source vector
    void set(const vec4 &v) noexcept { _reg = v._reg; }
    /// set the 4 components of the vector using a smaller source vector
    void set(const vec3<T> &v) noexcept { std::memcpy(&_v[0], &v._v[0], 3 * sizeof(T)); }
    /// set the 4 components of the vector using a smaller source vector
    void set(const vec3<T> &v, T _w) noexcept { std::memcpy(&_v[0], &v._v[0], 3 * sizeof(T)); w = _w; }
    /// set the 4 components of the vector using a smaller source vector
    void set(const vec2<T> &v) noexcept { std::memcpy(&_v[0], &v._v[0], 2 * sizeof(T)); }
    /// set the 4 components of the vector using smaller source vectors
    void set(const vec2<T> &v1, const vec2<T> &v2) noexcept { this->set(v1.x, v1.y, v2.x, v2.y); }
    /// set all the components back to 0
    void reset() noexcept { this->set(static_cast<T>(0)); }
    /// compare 2 vectors
    template<typename U>
    [[nodiscard]] bool compare(const vec4<U> &v) const noexcept;
    /// compare 2 vectors within the specified tolerance
    template<typename U>
    [[nodiscard]] bool compare(const vec4<U> &v, const U epsi) const noexcept;
    /// swap the components  of this vector with that of the specified one
    void swap(vec4 *iv) noexcept;
    /// swap the components  of this vector with that of the specified one
    void swap(vec4 &iv) noexcept;
    /// transform the vector to unit length
    vec4& normalize();
    /// get the smallest value of X,Y,Z or W
    [[nodiscard]] T minComponent() const noexcept;
    /// get the largest value of X,Y,Z or W
    [[nodiscard]] T maxComponent() const noexcept;
    /// calculate the dot product between this vector and the specified one
    [[nodiscard]] T dot(const vec4 &v) const noexcept;
    /// return the vector's length
    [[nodiscard]] T length() const  noexcept { return Divide::Sqrt(lengthSquared()); }
    /// return the squared distance of the vector
    [[nodiscard]] T lengthSquared() const noexcept;
    /// round all four values
    void round();
    /// lerp between this and the specified vector by the specified amount
    void lerp(const vec4 &v, T factor) noexcept;
    /// lerp between this and the specified vector by the specified amount for each component
    void lerp(const vec4 &v, const vec4 &factor) noexcept;
    union {
        struct { T x, y, z, w; };
        struct { T s, t, p, q; };
        struct { T r, g, b, a; };
        struct { T pitch, yaw, roll, _pad; };
        struct { T turn, move, zoom, _pad; };
        struct { T width, height, depth, _pad; };
        struct { T left, right, bottom, top; };
        struct { T fov, ratio, znear, zfar; };
        struct { T width, height, depth, key; };
        struct { T offsetX, offsetY, sizeX, sizeY; };
        T _v[4];
        SimdVector<T> _reg;
    };
};
#pragma pack(pop)

/// lerp between the 2 specified vectors by the specified amount
template <typename T>
[[nodiscard]] vec4<T> Lerp(const vec4<T> &u, const vec4<T> &v, T factor) noexcept;
/// lerp between the 2 specified vectors by the specified amount for each component
template <typename T>
[[nodiscard]] vec4<T> Lerp(const vec4<T> &u, const vec4<T> &v, const vec4<T> &factor) noexcept;
template <typename T>
[[nodiscard]] vec4<T> Abs(const vec4<T> &vector) noexcept;
/// min/max functions
template <typename T>
[[nodiscard]] vec4<T> Min(const vec4<T> &v1, const vec4<T> &v2) noexcept;
template <typename T>
[[nodiscard]] vec4<T> Max(const vec4<T> &v1, const vec4<T> &v2) noexcept;
template <typename T>
[[nodiscard]] vec4<T> Normalize(vec4<T> &vector);
template <typename T>
[[nodiscard]] vec4<T> Normalized(const vec4<T> &vector);
/// multiply a vector by a value
template <typename T>
[[nodiscard]] vec4<T> operator*(T fl, const vec4<T> &v) noexcept;

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

    template<typename U>
    [[nodiscard]] bool contains(U _x, U _y) const noexcept { return COORDS_IN_RECT(static_cast<T>(_x), static_cast<T>(_y), *this); }
    [[nodiscard]] bool contains(const vec2<T>& coords) const noexcept { return contains(coords.x, coords.y); }
    [[nodiscard]] bool contains(T _x, T _y) const noexcept { return COORDS_IN_RECT(_x, _y, *this); }
    [[nodiscard]] vec2<T> clamp(T _x, T _y) const noexcept { CLAMP_IN_RECT(_x, _y, *this); return vec2<T>(_x, _y); }
    [[nodiscard]] vec2<T> clamp(const vec2<T>& coords) const noexcept { return clamp(coords.x, coords.y); }
};

};  // namespace Divide

// Inline definitions
#include "MathVectors.inl"

#endif
