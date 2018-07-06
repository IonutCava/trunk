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
 * vec2, vec3 & vec4 data : added texture coords (s,t,p,q) and color enums
 *(r,g,b,a)
 * mat3 & mat4 : added multiple float constructor ad modified methods returning
 *mat3 or mat4
 * optimisations like "x / 2.0f" replaced by faster "x * 0.5f"
 * defines of multiples usefull maths values and radian/degree conversions
 * vec2 : added methods : set, reset, compare, dot, closestPointOnLine,
 *closestPointOnSegment,
 *                        projectionOnLine, lerp, angle
 * vec3 : added methods : set, reset, compare, dot, cross, closestPointOnLine,
 *closestPointOnSegment,
 *                        projectionOnLine, lerp, angle
 * vec4 : added methods : set, reset, compare
 ***************************************************************************
 */
 /*
 *
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

 * If there are any concerns or questions about the code, please e-mail
smasherprog@gmail.com or visit www.nolimitsdesigns.com
 */

/*
 * Author: Scott Lee
 */

/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _MATH_MATRICES_H_
#define _MATH_MATRICES_H_

#include "config.h"
#include "Plane.h"

namespace Divide {

/*********************************
* mat2
*********************************/
template <typename T>
class mat2 {
    // m0  m1
    // m2  m3
    static_assert(std::is_arithmetic<T>::value &&
        !std::is_same<T, bool>::value,
        "non-arithmetic matrix type");
public:
    mat2();
    template<typename U>
    mat2(U m0, U m1,
         U m2, U m3);
    template<typename U>
    mat2(const U *m);
    template<typename U>
    mat2(const mat2<U> &m);
    template<typename U>
    mat2(const mat3<U> &m);
    template<typename U>
    mat2(const mat4<U> &m);

    template<typename U>
    vec2<T> operator*(const vec2<U> &v) const;
    template<typename U>
    vec3<T> operator*(const vec3<U> &v) const;
    template<typename U>
    vec4<T> operator*(const vec4<U> &v) const;

    template<typename U>
    mat2 operator*(const mat2<U> &m) const;
    template<typename U>
    mat2 operator/(const mat2<U> &m) const;
    template<typename U>
    mat2 operator+(const mat2<U> &m) const;
    template<typename U>
    mat2 operator-(const mat2<U> &m) const;

    template<typename U>
    mat2 &operator*=(const mat2<U> &m);
    template<typename U>
    mat2 &operator/=(const mat2<U> &m);
    template<typename U>
    mat2 &operator+=(const mat2<U> &m);
    template<typename U>
    mat2 &operator-=(const mat2<U> &m);

    template<typename U>
    mat2 operator*(U f) const;
    template<typename U>
    mat2 operator/(U f) const;
    template<typename U>
    mat2 operator+(U f) const;
    template<typename U>
    mat2 operator-(U f) const;

    template<typename U>
    mat2 &operator*=(U f);
    template<typename U>
    mat2 &operator/=(U f);
    template<typename U>
    mat2 &operator+=(U f);
    template<typename U>
    mat2 &operator-=(U f);

    template<typename U>
    bool operator==(const mat2<U> &B) const;
    template<typename U>
    bool operator!=(const mat2<U> &B) const;

    operator T *();
    operator const T *() const;

    T &operator[](I32 i);
    const T operator[](I32 i) const;

    T &element(I8 row, I8 column);
    const T &element(I8 row, I8 column) const;

    template<typename U>
    void set(U m0, U m1, U m2, U m3);
    template<typename U>
    void set(const U *matrix);
    template<typename U>
    void set(const mat2<U> &matrix);
    template<typename U>
    void set(const mat3<U> &matrix);
    template<typename U>
    void set(const mat4<U> &matrix);

    void zero();
    void identity();
    bool isIdentity() const;
    void swap(mat2 &B);

    T det() const;
    void inverse();
    void transpose();
    void inverseTranspose();

    mat2 getInverse() const;
    void getInverse(mat2 &ret) const;

    mat2 getTranspose() const;
    void getTranspose(mat2 &ret) const;

    mat2 getInverseTranspose() const;
    void getInverseTranspose(mat2 &ret) const;

    union {
        struct {
            T _11, _12;
            T _21, _22;
        };
        T mat[4];
        T m[2][2];
    };
};

/*********************************
 * mat3
 *********************************/
template <typename T>
class mat3 {
    // m0 m1 m2
    // m3 m4 m5
    // m7 m7 m8
    static_assert(std::is_arithmetic<T>::value && 
                  !std::is_same<T, bool>::value,
                  "non-arithmetic matrix type");
   public:
    mat3();
    template<typename U>
    mat3(U m0, U m1, U m2,
         U m3, U m4, U m5,
         U m6, U m7, U m8);
    template<typename U>
    mat3(const U *m);
    template<typename U>
    mat3(const mat2<U> &m);
    template<typename U>
    mat3(const mat3<U> &m);
    template<typename U>
    mat3(const mat4<U> &m);

    template<typename U>
    vec2<U> operator*(const vec2<U> &v) const;
    template<typename U>
    vec3<U> operator*(const vec3<U> &v) const;
    template<typename U>
    vec4<U> operator*(const vec4<U> &v) const; 

    template<typename U>
    mat3 operator*(const mat3<U> &m) const;
    template<typename U>
    mat3 operator/(const mat3<U> &m) const;
    template<typename U>
    mat3 operator+(const mat3<U> &m) const;
    template<typename U>
    mat3 operator-(const mat3<U> &m) const;

    template<typename U>
    mat3 &operator*=(const mat3<U> &m);
    template<typename U>
    mat3 &operator/=(const mat3<U> &m);
    template<typename U>
    mat3 &operator+=(const mat3<U> &m);
    template<typename U>
    mat3 &operator-=(const mat3<U> &m);

    template<typename U>
    mat3 operator*(U f) const;
    template<typename U>
    mat3 operator/(U f) const;
    template<typename U>
    mat3 operator+(U f) const;
    template<typename U>
    mat3 operator-(U f) const;

    template<typename U>
    mat3 &operator*=(U f);
    template<typename U>
    mat3 &operator/=(U f);
    template<typename U>
    mat3 &operator+=(U f);
    template<typename U>
    mat3 &operator-=(U f);

    template<typename U>
    bool operator==(const mat3<U> &B) const;

    template<typename U>
    bool operator!=(const mat3<U> &B) const;

    operator T *();
    operator const T *() const;

    T &operator[](I32 i);
    const T operator[](I32 i) const;

    T &element(I8 row, I8 column);
    const T &element(I8 row, I8 column) const;

    template<typename U>
    void set(U m0, U m1, U m2, U m3, U m4, U m5, U m6, U m7, U m8);
    template<typename U>
    void set(const U *matrix);
    template<typename U>
    void set(const mat2<U> &matrix);
    template<typename U>
    void set(const mat3<U> &matrix);
    template<typename U>
    void set(const mat4<U> &matrix);

    void zero();
    void identity();
    bool isIdentity() const;
    void swap(mat3 &B);

    T det() const;
    void inverse();
    void transpose();
    void inverseTranspose();

    mat3 getInverse() const;
    void getInverse(mat3 &ret) const;

    mat3 getTranspose() const;
    void getTranspose(mat3 &ret) const;

    mat3 getInverseTranspose() const;
    void getInverseTranspose(mat3 &ret) const;

    template<typename U>
    void fromRotation(const vec3<U> &v, U angle, bool inDegrees = true);
    template<typename U>
    void fromRotation(U x, U y, U z, U angle, bool inDegrees = true);
    template<typename U>
    void fromXRotation(U angle, bool inDegrees = true);
    template<typename U>
    void fromYRotation(U angle, bool inDegrees = true);
    template<typename U>
    void fromZRotation(U angle, bool inDegrees = true);

    // setScale replaces the main diagonal!
    template<typename U>
    inline void setScale(U x, U y, U z);
    template<typename U>
    void setScale(const vec3<U> &v);

    void orthonormalize();

    union {
        struct {
            T _11, _12, _13;  // standard names for components
            T _21, _22, _23;  // standard names for components
            T _31, _32, _33;  // standard names for components
        };
        T mat[9];
        T m[3][3];
    };
};

/***************
 * mat4
 ***************/
template <typename T>
class mat4 : public std::conditional<std::is_same<T, F32>::value, alligned_base<16>, non_aligned_base>::type {
    // m0  m1  m2  m3
    // m4  m5  m6  m7
    // m8  m9  m10 m11
    // m12 m13 m14 m15
    static_assert(std::is_arithmetic<T>::value &&
                  !std::is_same<T, bool>::value,
                  "non-arithmetic matrix type");
   public:
    mat4() 
        : mat4(1, 0, 0, 0,
               0, 1, 0, 0,
               0, 0, 1, 0,
               0, 0, 0, 1)
    {
    }

    template<typename U>
    mat4(U m0,  U m1,  U m2,  U m3,
         U m4,  U m5,  U m6,  U m7,
         U m8,  U m9,  U m10, U m11,
         U m12, U m13, U m14, U m15)
        : mat{ static_cast<T>(m0),  static_cast<T>(m1),  static_cast<T>(m2),  static_cast<T>(m3),
               static_cast<T>(m4),  static_cast<T>(m5),  static_cast<T>(m6),  static_cast<T>(m7),
               static_cast<T>(m8),  static_cast<T>(m9),  static_cast<T>(m10), static_cast<T>(m11),
               static_cast<T>(m12), static_cast<T>(m13), static_cast<T>(m14), static_cast<T>(m15)}
    {
    }

    template<typename U>
    mat4(const U *m)
        : mat4(m[0],  m[1],  m[2],  m[3],
               m[4],  m[5],  m[6],  m[7],
               m[8],  m[9],  m[10], m[11],
               m[12], m[13], m[14], m[15])
    {
    }

    template<typename U>
    mat4(const mat2<U> &m)
        : mat4(m[0],              m[1],              static_cast<U>(0), static_cast<U>(0),
               m[2],              m[3],              static_cast<U>(0), static_cast<U>(0),
               static_cast<U>(0), static_cast<U>(0), static_cast<U>(0), static_cast<U>(0),
               static_cast<U>(0), static_cast<U>(0), static_cast<U>(0), static_cast<U>(0)) //maybe m[15] should be 1?
    {
    }

    template<typename U>
    mat4(const mat3<U> &m)
        : mat4(m[0],              m[1],              m[2],              static_cast<U>(0),
               m[3],              m[4],              m[5]               static_cast<U>(0),
               m[6],              m[7],              m[8],              static_cast<U>(0),
               static_cast<U>(0), static_cast<U>(0), static_cast<U>(0), static_cast<U>(0)) //maybe m[15] should be 1?
    {
    }

    mat4(const mat4 &m) : mat4(m.mat)
    {
    }

    template<typename U>
    mat4(const mat4<U> &m) : mat4(m.mat)
    {
    }

    template<typename U>
    mat4(const vec3<U> &translation, const vec3<U> &scale)
        : mat4(scale.x,           static_cast<U>(0), static_cast<U>(0), static_cast<U>(0),
               static_cast<U>(0), scale.y,           static_cast<U>(0), static_cast<U>(0), 
               static_cast<U>(0), static_cast<U>(0), scale.z,           static_cast<U>(0), 
               translation.x,     translation.y,     translation.z,     static_cast<U>(1))
    {
    }

    template<typename U>
    mat4(const vec3<U> &translation, const vec3<U> &scale, const mat4<U>& rotation) 
        : mat4(scale.x, static_cast<U>(0), static_cast<U>(0),           static_cast<U>(0),
               static_cast<U>(0), scale.y, static_cast<U>(0),           static_cast<U>(0),
               static_cast<U>(0), static_cast<U>(0), scale.z,           static_cast<U>(0),
               static_cast<U>(0), static_cast<U>(0), static_cast<U>(0), static_cast<U>(1))
    {
        set(*this * rotation);
        setTranslation(translation);
    }

    template<typename U>
    mat4(const vec3<U> &translation)
        : mat4(translation.x, translation.y, translation.z)
    {
    }

    template<typename U>
    mat4(U translationX, U translationY, U translationZ)
        : mat4(static_cast<U>(0), static_cast<U>(0), static_cast<U>(0), static_cast<U>(0),
               static_cast<U>(0), static_cast<U>(0), static_cast<U>(0), static_cast<U>(0),
               static_cast<U>(0), static_cast<U>(0), static_cast<U>(0), static_cast<U>(0),
               translation.x,     translation.y,     translation.z,     static_cast<U>(1))
    {
    }

    template<typename U>
    mat4(const vec3<U> &axis, U angle, bool inDegrees = true)
        : mat4(axis.x, axis.y, axis.z, angle, inDegrees)
    {
    }

    template<typename U>
    mat4(U x, U y, U z, U angle, bool inDegrees = true) 
        : mat4()
    {
        rotate(x, y, z, angle, inDegrees);
    }

    template<typename U>
    mat4(const vec3<U> &eye, const vec3<U> &target, const vec3<U> &up)
        : mat4()
    {
        lookAt(eye, target, up);
    }

    template<typename U>
    inline vec3<U> transform(const vec3<U> &v, bool homogeneous) const {
        return  homogeneous ? transformHomogeneous(v)
                            : transformNonHomogeneous(v);
    }

    /*Transforms the given 3-D vector by the matrix, projecting the result back into <i>w</i> = 1. (OGRE reference)*/
    template<typename U>
    inline vec3<U> transformHomogeneous(const vec3<U> &v) const {
        F32 fInvW = 1.0f / (this->m[0][3] * v.x + this->m[1][3] * v.y +
                            this->m[2][3] * v.z + this->m[3][3]);

        return vec3<U>((this->m[0][0] * v.x + this->m[1][1] * v.y + this->m[2][0] * v.z + this->m[3][0]) * fInvW,
                       (this->m[0][1] * v.x + this->m[1][1] * v.y + this->m[2][1] * v.z + this->m[3][1]) * fInvW,
                       (this->m[0][2] * v.x + this->m[1][2] * v.y + this->m[2][2] * v.z + this->m[3][2]) * fInvW);
    }

    template<typename U>
    inline vec3<U> transformNonHomogeneous(const vec3<U> &v) const {
        return *this * vec4<U>(v, static_cast<U>(0));
    }

    template<typename U>
    vec3<U> operator*(const vec3<U> &v) const {
        return vec3<U>(this->mat[0]  * v.x + this->mat[4] * v.y +
                       this->mat[8]  * v.z + this->mat[12],
                       this->mat[1]  * v.x + this->mat[5] * v.y +
                       this->mat[9]  * v.z + this->mat[13],
                       this->mat[2]  * v.x + this->mat[6] * v.y +
                       this->mat[10] * v.z + this->mat[14]);
    }

    template<typename U>
    vec4<U> operator*(const vec4<U> &v) const {
        return vec4<U>(this->mat[0]  * v.x + this->mat[4]  * v.y +
                       this->mat[8]  * v.z + this->mat[12] * v.w,
                       this->mat[1]  * v.x + this->mat[5]  * v.y +
                       this->mat[9]  * v.z + this->mat[13] * v.w,
                       this->mat[2]  * v.x + this->mat[6]  * v.y +
                       this->mat[10] * v.z + this->mat[14] * v.w,
                       this->mat[3]  * v.x + this->mat[7]  * v.y +
                       this->mat[11] * v.z + this->mat[15] * v.w);
    }

    template<typename U>
    mat4<T> operator*(U f) const {
        mat4<T> retValue;
        Util::Mat4::MultiplyScalar(this->mat, f, retValue.mat);
        return retValue;
    }

    template<typename U>
    mat4<T> operator/(U f) const {
        mat4<T> retValue;
        Util::Mat4::DivideScalar(this->mat, f, retValue.mat);
        return retValue;
    }

    template<typename U>
    mat4<T> operator*(const mat4<U>& matrix) const {
        mat4<T> retValue;
        Util::Mat4::Multiply(this->mat, matrix.mat, retValue.mat);
        return retValue;
    }
    
    inline T elementSum() const {
        return this->mat[0] + this->mat[1] + this->mat[2] + this->mat[3] +
               this->mat[4] + this->mat[5] + this->mat[6] + this->mat[7] +
               this->mat[8] + this->mat[9] + this->mat[10] + this->mat[11] +
               this->mat[12] + this->mat[13] + this->mat[14] + this->mat[15];
    }

    template<typename U>
    inline bool operator==(const mat4<U>& B) const {
        // Add a small epsilon value to avoid 0.0 != 0.0
        if (!COMPARE(this->elementSum() + EPSILON_F32,
                     B.elementSum() + EPSILON_F32)) {
            return false;
        }

        if (!COMPARE(this->m[0][0], B.m[0][0]) ||
            !COMPARE(this->m[0][1], B.m[0][1]) ||
            !COMPARE(this->m[0][2], B.m[0][2]) ||
            !COMPARE(this->m[0][3], B.m[0][3])) {
            return false;
        }
        if (!COMPARE(this->m[1][0], B.m[1][0]) ||
            !COMPARE(this->m[1][1], B.m[1][1]) ||
            !COMPARE(this->m[1][2], B.m[1][2]) ||
            !COMPARE(this->m[1][3], B.m[1][3])) {
            return false;
        }
        if (!COMPARE(this->m[2][0], B.m[2][0]) ||
            !COMPARE(this->m[2][1], B.m[2][1]) ||
            !COMPARE(this->m[2][2], B.m[2][2]) ||
            !COMPARE(this->m[2][3], B.m[2][3])) {
            return false;
        }
        if (!COMPARE(this->m[3][0], B.m[3][0]) ||
            !COMPARE(this->m[3][1], B.m[3][1]) ||
            !COMPARE(this->m[3][2], B.m[3][2]) ||
            !COMPARE(this->m[3][3], B.m[3][3])) {
            return false;
        }

        return true;
    }

    template<typename U>
    inline bool operator!=(const mat4<U> &B) const { return !(*this == B); }

    template<typename U>
    inline void set(U m0,  U m1,  U m2,  U m3,
                    U m4,  U m5,  U m6,  U m7,
                    U m8,  U m9,  U m10, U m11,
                    U m12, U m13, U m14, U m15) {
        this->mat[0] = static_cast<T>(m0);
        this->mat[4] = static_cast<T>(m4);
        this->mat[8] = static_cast<T>(m8);
        this->mat[12] = static_cast<T>(m12);

        this->mat[1] = static_cast<T>(m1);
        this->mat[5] = static_cast<T>(m5);
        this->mat[9] = static_cast<T>(m9);
        this->mat[13] = static_cast<T>(m13);

        this->mat[2] = static_cast<T>(m2);
        this->mat[6] = static_cast<T>(m6);
        this->mat[10] = static_cast<T>(m10);
        this->mat[14] = static_cast<T>(m14);

        this->mat[3] = static_cast<T>(m3);
        this->mat[7] = static_cast<T>(m7);
        this->mat[11] = static_cast<T>(m11);
        this->mat[15] = static_cast<T>(m15);
    }

    template<typename U>
    inline void set(U const *matrix) {
        static_assert(sizeof(T) == sizeof(U), "unsupported data type");

        std::memcpy(this->mat, matrix, sizeof(U) * 16);
    }

    template<typename U>
    inline void set(const mat2<U> &matrix) {
        const U zero = static_cast<U>(0);

        set(matrix[0], matrix[1], zero, zero,
            matrix[2], matrix[3], zero, zero,
            zero,      zero,      zero, zero,
            zero,      zero,      zero, zero);
    }

    template<typename U>
    inline void set(const mat3<U> &matrix) {
        const U zero = static_cast<U>(0);

        set(matrix[0], matrix[1], matrix[2], zero,
            matrix[3], matrix[4], matrix[5], zero,
            matrix[6], matrix[7], matrix[8], zero,
            zero,      zero,      zero,      zero);
    }

    template<typename U>
    inline void set(const mat4<U> &matrix) {
        this->set(matrix.mat);
    }

    template<typename U>
    inline void setRow(I32 index, const U value) {
        this->_vec[index].set(value);
    }

    template<typename U>
    inline void setRow(I32 index, const vec4<U> &value) {
        this->_vec[index].set(value);
    }

    template<typename U>
    inline void setRow(I32 index, const U x, const U y, const U z, const U w) {
        this->_vec[index].set(x, y, z, w);
    }

    template<typename U>
    inline void setCol(I32 index, const vec4<U> &value) {
        this->setCol(index, value.x, value.y, value.z, value.w);
    }

    template<typename U>
    inline void setCol(I32 index, const U value) {
        this->m[0][index] = static_cast<T>(value);
        this->m[1][index] = static_cast<T>(value);
        this->m[2][index] = static_cast<T>(value);
        this->m[3][index] = static_cast<T>(value);
    }

    template<typename U>
    inline void setCol(I32 index, const U x, const U y, const U z, const U w) {
        this->m[0][index] = static_cast<T>(x);
        this->m[1][index] = static_cast<T>(y);
        this->m[2][index] = static_cast<T>(z);
        this->m[3][index] = static_cast<T>(w);
    }

    template<typename U>
    mat4 operator+(const mat4<U> &matrix) const {
        mat4<T> retValue;
        Util::Mat4::Add(this->mat, matrix.mat, retValue.mat);
        return retValue;
    }

    template<typename U>
    mat4 operator-(const mat4<U> &matrix) const {
        mat4<T> retValue;
        Util::Mat4::Subtract(this->mat, matrix.mat, retValue.mat);
        return retValue;
    }

    inline T &element(I8 row, I8 column) {
        return this->m[row][column];
    }

    inline const T &element(I8 row, I8 column) const {
        return this->m[row][column];
    }

    template<typename U>
    mat4 &operator*=(U f) {
        Util::Mat4::MultiplyScalar(this->mat, f, this->mat);
        return *this;
    }

    template<typename U>
    mat4 &operator/=(U f) {
        Util::Mat4::MultiplyScalar(this->mat, 1/f, this->mat);
        return *this;
    }

    template<typename U>
    mat4 &operator*=(const mat4<U> &matrix) {
        this->set(*this * matrix);
        return *this;
    }

    template<typename U>
    mat4 &operator+=(const mat4<U> &matrix) {
        Util::Mat4::Add(this->mat, matrix.mat, this->mat);
        return *this;
    }

    template<typename U>
    mat4 &operator-=(const mat4<U> &matrix) {
        Util::Mat4::Subtract(this->mat, matrix.mat, this->mat);
        return *this;
    }

    operator T *() { return this->mat; }
    operator const T *() const { return this->mat; }

    T &operator[](I32 i) { return this->mat[i]; }
    const T &operator[](I32 i) const { return this->mat[i]; }

    mat4 &operator=(const mat4& other) { this->set(other); return *this; }

    template<typename U>
    mat4 &operator=(const mat4<U>& other) { this->set(other); return *this; }

    template<typename U>
    inline vec3<U> getTranslation() const {
        return vec3<U>(mat[12], mat[13], mat[14]);
    }

    inline mat4 getRotation(void) const {
        return mat4(this->mat[0], this->mat[1], this->mat[2], 0, this->mat[4],
                    this->mat[5], this->mat[6], 0, this->mat[8], this->mat[9],
                    this->mat[10], 0, 0, 0, 0, 1);
    }

    inline void transpose() {
        this->set(this->mat[0], this->mat[4], this->mat[8], this->mat[12],
                  this->mat[1], this->mat[5], this->mat[9], this->mat[13],
                  this->mat[2], this->mat[6], this->mat[10], this->mat[14],
                  this->mat[3], this->mat[7], this->mat[11], this->mat[15]);
    }

    inline mat4 transposeRotation() const {
        this->set(this->mat[0], this->mat[4], this->mat[8], this->mat[3],
                  this->mat[1], this->mat[5], this->mat[9], this->mat[7],
                  this->mat[2], this->mat[6], this->mat[10], this->mat[11],
                  this->mat[12], this->mat[13], this->mat[14], this->mat[15]);
    }

    template<typename U>
    inline void getTranspose(mat4<U> &out) const {
        out.set(this->getTranspose());
    }

    inline mat4 getTranspose() const {
        return mat4(this->mat[0], this->mat[4], this->mat[8], this->mat[12],
                    this->mat[1], this->mat[5], this->mat[9], this->mat[13],
                    this->mat[2], this->mat[6], this->mat[10], this->mat[14],
                    this->mat[3], this->mat[7], this->mat[11], this->mat[15]);
    }

    inline mat4 getTransposeRotation() const {
        return mat4(this->mat[0], this->mat[4], this->mat[8], this->mat[3],
                    this->mat[1], this->mat[5], this->mat[9], this->mat[7],
                    this->mat[2], this->mat[6], this->mat[10], this->mat[11],
                    this->mat[12], this->mat[13], this->mat[14], this->mat[15]);
    }

    inline T det(void) const {
        return Util::Mat4::Det(this->mat);
    }

    inline mat4 getInverse() const {
        mat4 ret(this->mat);
        ret.inverse();
        return ret;
    }

    inline void getInverse(mat4 &ret) const {
        Util::Mat4::Inverse(this->mat, ret.mat);
    }

    inline void inverse() {
        Util::Mat4::Inverse(this->mat, this->mat);
    }

    inline void zero() {
        memset(this->mat, 0, sizeof(T) * 16);
    }

    inline mat4 getInverseTranspose() const {
        mat4 ret(getInverse());
        ret.transpose();
        return ret;
    }

    inline void inverseTranspose(mat4 &ret) const {
        ret.set(getInverse());
        ret.transpose();
    }

    inline void inverseTranspose() {
        this->inverse();
        this->transpose();
    }

    inline void identity() {
        set(1, 0, 0, 0,
            0, 1, 0, 0, 
            0, 0, 1, 0,
            0, 0, 0, 1);
    }

    inline void bias() {
        set(0.5, 0.0, 0.0, 0.0,
            0.0, 0.5, 0.0, 0.0,
            0.0, 0.0, 0.5, 0.0,
            0.5, 0.5, 0.5, 1.0);
    }

    template<typename U>
    inline void setTranslation(const vec3<U> &v) {
        this->setTranslation(v.x, v.y, v.z);
    }

    template<typename U>
    inline void setScale(const vec3<U> &v) { this->setScale(v.x, v.y, v.z); }

    template<typename U>
    inline void scale(const vec3<U> &v) { this->scale(v.x, v.y, v.z); }

    template<typename U>
    inline void translate(const vec3<U> &v) { this->translate(v.x, v.y, v.z); }

    template<typename U>
    inline void rotate(const vec3<U> &axis, U angle, bool inDegrees = true) {
        this->rotate(axis.x, axis.y, axis.z, angle, inDegrees);
    }

    template<typename U>
    inline void rotate(U x, U y, U z, U angle, bool inDegrees = true) {
        if (inDegrees) {
            angle = Angle::DegreesToRadians(angle);
        }

        vec3<U> v(x, y, z);
        v.normalize();

        U c = std::cos(angle);
        U s = std::sin(angle);

        U xx = v.x * v.x;
        U yy = v.y * v.y;
        U zz = v.z * v.z;
        U xy = v.x * v.y;
        U yz = v.y * v.z;
        U zx = v.z * v.x;
        U xs = v.x * s;
        U ys = v.y * s;
        U zs = v.z * s;

        set((1 - c) * xx + c,        (1 - c) * xy + zs,       (1 - c) * zx - ys,       static_cast<U>(mat[3]),
            (1 - c) * xy - zs,       (1 - c) * yy + c,        (1 - c) * yz + xs,       static_cast<U>(mat[7]),
            (1 - c) * zx + ys,       (1 - c) * yz - xs,       (1 - c) * zz + c,        static_cast<U>(mat[11]),
            static_cast<U>(mat[12]), static_cast<U>(mat[13]), static_cast<U>(mat[14]), static_cast<U>(mat[15]))
    }

    template<typename U>
    inline void rotate_x(U angle, bool inDegrees = true) {
        if (inDegrees) {
            angle = Angle::DegreesToRadians(angle);
        }

        U c = std::cos(angle);
        U s = std::sin(angle);

        this->mat[5] = static_cast<T>(c);
        this->mat[9] = static_cast<T>(-s);
        this->mat[6] = static_cast<T>(s);
        this->mat[10] = static_cast<T>(c);
    }

    template<typename U>
    inline void rotate_y(U angle, bool inDegrees = true) {
        if (inDegrees) {
            angle = Angle::DegreesToRadians(angle);
        }

        U c = std::cos(angle);
        U s = std::sin(angle);

        this->mat[0] = static_cast<T>(c);
        this->mat[8] = static_cast<T>(s);
        this->mat[2] = static_cast<T>(-s);
        this->mat[10] = static_cast<T>(c);
    }

    template<typename U>
    inline void rotate_z(U angle, bool inDegrees = true) {
        if (inDegrees) {
            angle = Angle::DegreesToRadians(angle);
        }

        U c = std::cos(angle);
        U s = std::sin(angle);

        this->mat[0] = static_cast<T>(c);
        this->mat[4] = static_cast<T>(-s);
        this->mat[1] = static_cast<T>(s);
        this->mat[5] = static_cast<T>(c);
    }

    template<typename U>
    inline void setScale(U x, U y, U z) {
        this->mat[0] = static_cast<T>(x);
        this->mat[5] = static_cast<T>(y);
        this->mat[10] = static_cast<T>(z);
    }

    template<typename U>
    inline void scale(U x, U y, U z) {
        this->mat[0] *= static_cast<T>(x);
        this->mat[4] *= static_cast<T>(y);
        this->mat[8] *= static_cast<T>(z);
        this->mat[1] *= static_cast<T>(x);
        this->mat[5] *= static_cast<T>(y);
        this->mat[9] *= static_cast<T>(z);
        this->mat[2] *= static_cast<T>(x);
        this->mat[6] *= static_cast<T>(y);
        this->mat[10] *= static_cast<T>(z);
        this->mat[3] *= static_cast<T>(x);
        this->mat[7] *= static_cast<T>(y);
        this->mat[11] *= static_cast<T>(z);
    }

    template<typename U>
    inline void setTranslation(U x, U y, U z) {
        this->mat[12] = static_cast<T>(x);
        this->mat[13] = static_cast<T>(y);
        this->mat[14] = static_cast<T>(z);
    }

    template<typename U>
    inline void translate(U x, U y, U z) {
        this->mat[12] += static_cast<T>(x);
        this->mat[13] += static_cast<T>(y);
        this->mat[14] += static_cast<T>(z);
    }

    template<typename U>
    void reflect(const Plane<U> &plane) {
        const U zero = static_cast<U>(0);
        const U one = static_cast<U>(1);

        const vec4<U> &eq = plane.getEquation();

        U x = eq.x;
        U y = eq.y;
        U z = eq.z;
        U w = eq.w;
        U x2 = x * 2.0f;
        U y2 = y * 2.0f;
        U z2 = z * 2.0f;

        set( 1 - x * x2, -x * y2,     -x * z2,     zero,
            -y * x2,      1 - y * y2, -y * z2,     zero,
            -z * x2,     -z * y2,      1 - z * z2, zero,
            -w * x2,     -w * y2,     -w * z2,     one);
    }

    template<typename U>
    void lookAt(const vec3<U> &eye, const vec3<U> &target, const vec3<U> &up) {
        vec3<U> zAxis(eye - target);
        zAxis.normalize();
        vec3<U> xAxis(Cross(up, zAxis));
        xAxis.normalize();
        vec3<U> yAxis(Cross(zAxis, xAxis));
        yAxis.normalize();

        this->m[0][0] = static_cast<T>(xAxis.x);
        this->m[1][0] = static_cast<T>(xAxis.y);
        this->m[2][0] = static_cast<T>(xAxis.z);
        this->m[3][0] = static_cast<T>(-xAxis.dot(eye));

        this->m[0][1] = static_cast<T>(yAxis.x);
        this->m[1][1] = static_cast<T>(yAxis.y);
        this->m[2][1] = static_cast<T>(yAxis.z);
        this->m[3][1] = static_cast<T>(-yAxis.dot(eye));

        this->m[0][2] = static_cast<T>(zAxis.x);
        this->m[1][2] = static_cast<T>(zAxis.y);
        this->m[2][2] = static_cast<T>(zAxis.z);
        this->m[3][2] = static_cast<T>(-zAxis.dot(eye));
    }

    template<typename U>
    void ortho(U left, U right, U bottom, U top, U zNear, U zFar) {
        this->m[0][0] = static_cast<T>(2 / (right - left));
        this->m[1][1] = static_cast<T>(2 / (top - bottom));
        this->m[2][2] = -static_cast<T>(2 / (zFar - zNear));
        this->m[3][0] = -static_cast<T>((right + left) / (right - left));
        this->m[3][1] = -static_cast<T>((top + bottom) / (top - bottom));
        this->m[3][2] = -static_cast<T>((zFar + zNear) / (zFar - zNear));
    }

    template<typename U>
    void perspective(U fovyRad, U aspect, U zNear, U zFar) {
        assert(!IS_ZERO(aspect));
        assert(zFar > zNear);

        U tanHalfFovy = static_cast<U>(std::tan(fovyRad * 0.5f));

        this->zero();

        this->m[0][0] = static_cast<T>(1 / (aspect * tanHalfFovy));
        this->m[1][1] = static_cast<T>(1 / (tanHalfFovy));
        this->m[2][2] = -static_cast<T>((zFar + zNear) / (zFar - zNear));
        this->m[2][3] = -static_cast<T>(1);
        this->m[3][2] = -static_cast<T>((2 * zFar * zNear) / (zFar - zNear));
    }

    template<typename U>
    void frustum(U left, U right, U bottom, U top, U nearVal, U farVal) {
        this->zero();

        this->m[0][0] = static_cast<T>((2 * nearVal) / (right - left));
        this->m[1][1] = static_cast<T>((2 * nearVal) / (top - bottom));
        this->m[2][0] = static_cast<T>((right + left) / (right - left));
        this->m[2][1] = static_cast<T>((top + bottom) / (top - bottom));
        this->m[2][2] = -static_cast<T>((farVal + nearVal) / (farVal - nearVal));
        this->m[2][3] = static_cast<T>(-1);
        this->m[3][2] = -static_cast<T>((2 * farVal * nearVal) / (farVal - nearVal));
    }

    template<typename U>
    inline void reflect(U x, U y, U z, U w) {
        this->reflect(Plane<U>(x, y, z, w));
    }

    template<typename U>
    inline void extractMat3(mat3<U> &matrix3) const {
        matrix3.m[0][0] = static_cast<U>(this->m[0][0]);
        matrix3.m[0][1] = static_cast<U>(this->m[0][1]);
        matrix3.m[0][2] = static_cast<U>(this->m[0][2]);
        matrix3.m[1][0] = static_cast<U>(this->m[1][0]);
        matrix3.m[1][1] = static_cast<U>(this->m[1][1]);
        matrix3.m[1][2] = static_cast<U>(this->m[1][2]);
        matrix3.m[2][0] = static_cast<U>(this->m[2][0]);
        matrix3.m[2][1] = static_cast<U>(this->m[2][1]);
        matrix3.m[2][2] = static_cast<U>(this->m[2][2]);
    }

    inline void swap(mat4 &B) {
        std::swap(this->m[0][0], B.m[0][0]);
        std::swap(this->m[0][1], B.m[0][1]);
        std::swap(this->m[0][2], B.m[0][2]);
        std::swap(this->m[0][3], B.m[0][3]);

        std::swap(this->m[1][0], B.m[1][0]);
        std::swap(this->m[1][1], B.m[1][1]);
        std::swap(this->m[1][2], B.m[1][2]);
        std::swap(this->m[1][3], B.m[1][3]);

        std::swap(this->m[2][0], B.m[2][0]);
        std::swap(this->m[2][1], B.m[2][1]);
        std::swap(this->m[2][2], B.m[2][2]);
        std::swap(this->m[2][3], B.m[2][3]);

        std::swap(this->m[3][0], B.m[3][0]);
        std::swap(this->m[3][1], B.m[3][1]);
        std::swap(this->m[3][2], B.m[3][2]);
        std::swap(this->m[3][3], B.m[3][3]);
    }

    union {
        struct {
            T _11, _12, _13, _14;  // standard names for components
            T _21, _22, _23, _24;  // standard names for components
            T _31, _32, _33, _34;  // standard names for components
            T _41, _42, _43, _44;  // standard names for components
        };
        T mat[16];
        T m[4][4];
        simd_vector<T> _reg[4];
        vec4<T> _vec[4];
    };
};

struct Line {
    vec3<F32> _startPoint;
    vec3<F32> _endPoint;
    vec4<U8> _colorStart;
    vec4<U8> _colorEnd;
    F32      _widthStart;
    F32      _widthEnd;

    Line()
    {
    }

    Line(const vec3<F32> &startPoint, const vec3<F32> &endPoint,
         const vec4<U8> &color)
        : Line(startPoint, endPoint, color, color)
    {
    }

    Line(const vec3<F32> &startPoint, const vec3<F32> &endPoint,
        const vec4<U8> &color, F32 width)
        : Line(startPoint, endPoint, color, color, width)
    {
    }

    Line(const vec3<F32> &startPoint, const vec3<F32> &endPoint,
        const vec4<U8> &colorStart, const vec4<U8>& colorEnd,
        F32 width)
        : Line(startPoint, endPoint, colorStart, colorEnd)
    {
        _widthStart = _widthEnd = width;
    }

    Line(const vec3<F32> &startPoint, const vec3<F32> &endPoint,
         const vec4<U8> &colorStart, const vec4<U8>& colorEnd)
        : _startPoint(startPoint),
          _endPoint(endPoint),
          _colorStart(colorStart),
          _colorEnd(colorEnd),
          _widthStart(1.0f),
          _widthEnd(1.0f)
    {
    }

    void color(U8 r, U8 g, U8 b, U8 a) {
        _colorStart.set(r,g,b,a);
        _colorEnd.set(r,g,b,a);
    }

    void width(F32 width) {
        _widthStart = _widthEnd = width;
    }

    void segment(F32 startX, F32 startY, F32 startZ, 
                 F32 endX, F32 endY, F32 endZ) {
        _startPoint.set(startX, startY, startZ);
        _endPoint.set(endX, endY, endZ);
    }
};

/// Converts a point from world coordinates to projection coordinates
///(from Y = depth, Z = up to Y = up, Z = depth)
template <typename T>
inline void ProjectPoint(const vec3<T> &position, vec3<T> &output) {
    output.set(position.x, position.z, position.y);
}
template <typename T, typename U>
inline T Lerp(const T v1, const T v2, const U t) {
    return v1 + (v2 - v1 * t);
}

namespace Util {
    namespace Mat4 {
        FORCE_INLINE void Multiply(const mat4<F32>& matrixA, const mat4<F32>& matrixB, mat4<F32>& ret) {
            for (U8 i = 0; i < 4; ++i) {
                ret._vec[i].set(matrixB._vec[0] * matrixA._vec[i][0]
                              + matrixB._vec[1] * matrixA._vec[i][1]
                              + matrixB._vec[2] * matrixA._vec[i][2]
                              + matrixB._vec[3] * matrixA._vec[i][3]);
            }
        }
    };
};

};  // namespace Divide

#endif //_MATH_MATRICES_H_

#include "MathMatrices.inl"