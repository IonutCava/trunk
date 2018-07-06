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

/*
   Copyright (c) 2014 DIVIDE-Studio
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

#ifndef _MATH_CLASSES_H_
#define _MATH_CLASSES_H_

#include "config.h"
#ifndef M_PI
#define M_PI				3.141592653589793238462643383279f		//  PI
#endif

#include "Plane.h"
#include "MathVectors.h"

/******************************//**
/* mat3
/*********************************/
template<class T>
class mat3 {
public:
    mat3() 
    {
        identity();
    }

    mat3(T m0, T m1, T m2,
         T m3, T m4, T m5,
         T m6, T m7, T m8) 
    {
        mat[0] = m0; mat[3] = m3; mat[6] = m6;
        mat[1] = m1; mat[4] = m4; mat[7] = m7;
        mat[2] = m2; mat[5] = m5; mat[8] = m8;
    }

    mat3(const T *m) 
    {
        set(m);
    }

    mat3(const mat3 &m) : mat3(m.mat)
    {
    }

    mat3(const mat4<T> &m) {
        set(m);
    }

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

    bool operator == (mat3& B) const {
        for (I32 i = 0; i < 9; i++){
            if (!FLOAT_COMPARE(this->m[i] + EPSILON , B[i] + EPSILON)) return false;
        }
        return true;
    }

    bool operator != (mat3& B) const { return !(*this == B);}

    operator T*()             { return this->mat; }
    operator const T*() const { return this->mat; }

    T &operator[](I8 i)             { return this->mat[i]; }
    const T operator[](I32 i) const { return this->mat[i]; }

    inline void set(T m0, T m1, T m2,
                    T m3, T m4, T m5,
                    T m6, T m7, T m8) {
        this->mat[0] = m0; this->mat[3] = m3; this->mat[6] = m6;
        this->mat[1] = m1; this->mat[4] = m4; this->mat[7] = m7;
        this->mat[2] = m2; this->mat[5] = m5; this->mat[8] = m8;
    }

    inline void set(const T *m) {
        memcpy(this->mat, m, sizeof(T) * 9);
    }

    inline void set(const mat4<T> &matrix) {
		this->mat[0] = matrix[0]; this->mat[3] = matrix[4]; this->mat[6] = matrix[8];
        this->mat[1] = matrix[1]; this->mat[4] = matrix[5]; this->mat[7] = matrix[9];
        this->mat[2] = matrix[2]; this->mat[5] = matrix[6]; this->mat[8] = matrix[10];
    }

    inline mat3 getInverse()                const { mat3 ret; this->inverse(ret); return ret;}
    inline void inverse(mat3& ret)          const { ret.set(mat); ret.inverse(); }
    inline mat3 getInverseTranspose()       const { mat3 ret; this->inverse(ret); return ret.transpose(); }
    inline void inverseTranspose(mat3& ret) const { ret.set(mat); ret.inverseTranspose(); }
    inline void inverseTranspose()                { this->inverse(); this->transpose(); }

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
        return mat3( (mat[4] * mat[8] - mat[7] * mat[5]) * idet,
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
        memset(this->mat, 0.0, 9 * sizeof(T));
    }

    inline void identity() {
        zero();
        mat[0] = 1.0;
        mat[4] = 1.0;
        mat[8] = 1.0;
    }

    inline bool isIdentity() const{
        return (FLOAT_COMPARE(mat[0],1.0) && IS_ZERO(mat[1])           && IS_ZERO(mat[2])            && IS_ZERO(mat[3])  &&
                IS_ZERO(mat[4])           && FLOAT_COMPARE(mat[5],1.0) && IS_ZERO(mat[6])            && IS_ZERO(mat[7])  &&
                IS_ZERO(mat[8])           && IS_ZERO(mat[9])           && FLOAT_COMPARE(mat[10],1.0) && IS_ZERO(mat[11]) &&
                IS_ZERO(mat[12])          && IS_ZERO(mat[13])          && IS_ZERO(mat[14])           && FLOAT_COMPARE(mat[15],1.0));
    }

    inline void rotate(const vec3<T> &v,T angle, bool inDegrees = true) {
        rotate(v.x,v.y,v.z,angle,inDegrees);
    }

    void rotate(T x,T y,T z,T angle, bool inDegrees = true) {
        if(inDegrees) DegToRad(angle);

        T c = (T)cos(angle);
        T s = (T)sin(angle);
        T l = square_root_tpl(x * x + y * y + z * z);
        if (l < EPSILON) l = 1;
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

    inline void rotate_x(T angle, bool inDegrees = true) {
        if(inDegrees) DegToRad(angle);

        T c = (T)cos(angle);
        T s = (T)sin(angle);
        mat[0] = 1; mat[3] = 0; mat[6] = 0;
        mat[1] = 0; mat[4] = c; mat[7] = -s;
        mat[2] = 0; mat[5] = s; mat[8] = c;
    }

    inline void rotate_y(T angle, bool inDegrees = true) {
        if(inDegrees) DegToRad(angle);

        T c = (T)cos(angle);
        T s = (T)sin(angle);
        mat[0] = c;  mat[3] = 0; mat[6] = s;
        mat[1] = 0;  mat[4] = 1; mat[7] = 0;
        mat[2] = -s; mat[5] = 0; mat[8] = c;
    }

    inline void rotate_z(T angle, bool inDegrees = true) {
        if(inDegrees) DegToRad(angle);

        T c = (T)cos(angle);
        T s = (T)sin(angle);
        mat[0] = c; mat[3] = -s; mat[6] = 0;
        mat[1] = s; mat[4] = c;  mat[7] = 0;
        mat[2] = 0; mat[5] = 0;  mat[8] = 1;
    }

    inline void setScale(T x,T y,T z) {
        mat[0] = x;
        mat[4] = y;
        mat[8] = z;
    }

    inline void setScale(const vec3<T> &v) {
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
        struct {
            T _11, _12, _13;   // standard names for components
            T _21, _22, _23;   // standard names for components
            T _31, _32, _33;   // standard names for components
        };
        T mat[9];
        T m[3][3];
    };
};

/***************
/* mat4
/***************/
template<class T>
class mat4 {
public:
    mat4() 
    {
        identity();
    }

    mat4(T m0, T m1, T m2, T m3,
         T m4, T m5, T m6, T m7,
         T m8, T m9, T m10,T m11,
         T m12,T m13,T m14,T m15) 
    {
        mat[0] = m0; mat[4] = m4; mat[8]  = m8;  mat[12] = m12;
        mat[1] = m1; mat[5] = m5; mat[9]  = m9;  mat[13] = m13;
        mat[2] = m2; mat[6] = m6; mat[10] = m10; mat[14] = m14;
        mat[3] = m3; mat[7] = m7; mat[11] = m11; mat[15] = m15;
    }

    mat4(const T *m)
    {
        set(m);
    }

    mat4(const mat4 &m) : mat4(m.mat)
    {
    }

    mat4(const vec4<T> &col1,const vec4<T> &col2,const vec4<T> &col3,const vec4<T> &col4)
    {
        setCol(0, col1); setCol(1, col2); setCol(2, col3); setCol(3, col4);
    }

    mat4(const vec3<T> &translation, const vec3<T> &scale) : mat4(translation)
    {
        setScale(scale);
    }

    mat4(const vec3<T> &translation) : mat4(translation.x, translation.y, translation.z)
    {
    }

    mat4(T translationX, T translationY, T translationZ) : mat4()
    {
        setTranslation(translationX, translationY, translationZ);
    }

    mat4(const vec3<T> &axis, T angle, bool inDegrees = true) : mat4(axis.x, axis.y, axis.z, angle, inDegrees)
    {
    }

    mat4(T x, T y, T z, T angle, bool inDegrees = true) : mat4()
    {
        rotate(x,y,z,angle,inDegrees);
    }

    mat4(const vec3<T>& eye, const vec3<T>& target, const vec3<T>& up) : mat4()
    {
        lookAt(eye, target, up);
    }

    mat4(const mat3<T> &m) 
    {
       set(m);
    }

    /*Transforms the given 3-D vector by the matrix, projecting the result back into <i>w</i> = 1. (OGRE reference)*/
    inline vec3<T> transform( const vec3<T> &v ) const {
        vec3<T> r;

        T fInvW = 1.0f / ( m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] );

        r.x = ( m[0][0] * v.x + m[1][1] * v.y + m[2][0] * v.z + m[3][0] ) * fInvW;
        r.y = ( m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] ) * fInvW;
        r.z = ( m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] ) * fInvW;

        return r;
    }

    vec3<T> operator*(const vec3<T> &v) const {
        return vec3<T>(this->mat[0] * v.x + this->mat[4] * v.y + this->mat[8]  * v.z + this->mat[12],
                       this->mat[1] * v.x + this->mat[5] * v.y + this->mat[9]  * v.z + this->mat[13],
                       this->mat[2] * v.x + this->mat[6] * v.y + this->mat[10] * v.z + this->mat[14]);
    }

    vec4<T> operator*(const vec4<T> &v) const {
        return vec4<T>(this->mat[0] * v.x + this->mat[4] * v.y + this->mat[8]  * v.z + this->mat[12] * v.w,
                       this->mat[1] * v.x + this->mat[5] * v.y + this->mat[9]  * v.z + this->mat[13] * v.w,
                       this->mat[2] * v.x + this->mat[6] * v.y + this->mat[10] * v.z + this->mat[14] * v.w,
                       this->mat[3] * v.x + this->mat[7] * v.y + this->mat[11] * v.z + this->mat[15] * v.w);
    }

    mat4<T> operator*(T f) const {
        return mat4<T>(this->mat[0]  * f, this->mat[1]  * f, this->mat[2]  * f, this->mat[3]  * f,
                       this->mat[4]  * f, this->mat[5]  * f, this->mat[6]  * f, this->mat[7]  * f,
                       this->mat[8]  * f, this->mat[9]  * f, this->mat[10] * f, this->mat[11] * f,
                       this->mat[12] * f, this->mat[13] * f, this->mat[14] * f, this->mat[15] * f);
    }

    mat4<T> operator*(const mat4<T> &m) const {
        return mat4<T>(Util::Mat4::Multiply(this->mat, m.mat));
    }

    inline T elementSum() const { return this->mat[0]  + this->mat[1]  + this->mat[2]  + this->mat[3]  + 
                                         this->mat[4]  + this->mat[5]  + this->mat[6]  + this->mat[7]  + 
                                         this->mat[8]  + this->mat[9]  + this->mat[10] + this->mat[11] + 
                                         this->mat[12] + this->mat[13] + this->mat[14] + this->mat[15]; }

    inline bool operator == (const mat4& B) const {
        if (!FLOAT_COMPARE(elementSum(), B.elementSum()))
            return false;

        const F32* mat2 = &B.mat[0];
        U8 i = 0;
        for (; i < 16; ++i)
            if (!FLOAT_COMPARE(mat[i] + EPSILON_F32, mat2[i] + EPSILON_F32)) //< add a small epsilon value to avoid 0.0 != 0.0
                return false;

        return true;
    }

    inline bool operator != (const mat4& B) const {
        return !(*this == B);
    }

    inline void set(T m0, T m1, T m2, T m3,
                    T m4, T m5, T m6, T m7,
                    T m8, T m9, T m10,T m11,
                    T m12,T m13,T m14,T m15){
        this->mat[0] = m0; this->mat[4] = m4; this->mat[8]  = m8;  this->mat[12] = m12;
        this->mat[1] = m1; this->mat[5] = m5; this->mat[9]  = m9;  this->mat[13] = m13;
        this->mat[2] = m2; this->mat[6] = m6; this->mat[10] = m10; this->mat[14] = m14;
        this->mat[3] = m3; this->mat[7] = m7; this->mat[11] = m11; this->mat[15] = m15;
    }

    inline void set(T const *m) {
        memcpy(this->mat, m, sizeof(T) * 16);
    }

    inline void set(const mat4& matrix) {
        this->set(matrix.mat);
    }

    inline void set(const mat3<T>& matrix) {
        mat[0] = matrix.mat[0]; mat[4] = matrix.mat[3]; mat[8]  = matrix.mat[6]; mat[12] = 0.0;
        mat[1] = matrix.mat[1]; mat[5] = matrix.mat[4]; mat[9]  = matrix.mat[7]; mat[13] = 0.0;
        mat[2] = matrix.mat[2]; mat[6] = matrix.mat[5]; mat[10] = matrix.mat[8]; mat[14] = 0.0;
        mat[3] = 0.0;           mat[7] = 0.0;           mat[11] = 0.0;           mat[15] = 1.0;
    }

    inline F32* getRow(I32 index) {
        return m[index];
    }
    
    inline void setRow(I32 index, const vec4<T>& value){
        m[index][0] = value.x;
        m[index][1] = value.y;
        m[index][2] = value.z;
        m[index][3] = value.w;
    }

    inline vec4<T> getCol(I32 index) const {
        return vec4<T>(this->mat[0 + (index*4)],
                       this->mat[1 + (index*4)],
                       this->mat[2 + (index*4)],
                       this->mat[3 + (index*4)]);
    }

    inline void setCol(I32 index, const vec4<T>& value){
        this->mat[0 + (index*4)] = value.x;
        this->mat[1 + (index*4)] = value.y;
        this->mat[2 + (index*4)] = value.z;
        this->mat[3 + (index*4)] = value.w;
    }

    /// premultiply the matrix by the given matrix
    void multmatrix(const mat4& matrix) {
        T tmp[4];
        for (I8 j=0; j<4; j++) {
            tmp[0] = this->mat[j];
            tmp[1] = this->mat[4+j];
            tmp[2] = this->mat[8+j];
            tmp[3] = this->mat[12+j];
            for (I8 i=0; i<4; i++) {
                this->mat[4*i+j] = matrix[4*i]*tmp[0] + matrix[4*i+1]*tmp[1] + matrix[4*i+2]*tmp[2] + matrix[4*i+3]*tmp[3];
            }
        }
    }

    mat4 operator+(const mat4 &matrix) const {
        return mat4(this->mat[0]  + matrix[0],  this->mat[1]  + matrix[1],  this->mat[2]  + matrix[2],  this->mat[3]  + matrix[3],
                    this->mat[4]  + matrix[4],  this->mat[5]  + matrix[5],  this->mat[6]  + matrix[6],  this->mat[7]  + matrix[7],
                    this->mat[8]  + matrix[8],  this->mat[9]  + matrix[9],  this->mat[10] + matrix[10], this->mat[11] + matrix[11],
                    this->mat[12] + matrix[12], this->mat[13] + matrix[13], this->mat[14] + matrix[14], this->mat[15] + matrix[15]);
    }

    mat4 operator-(const mat4 &matrix) const {
        return mat4(this->mat[0]  - matrix[0],  this->mat[1]  - matrix[1],  this->mat[2]  - matrix[2],  this->mat[3]  - matrix[3],
                    this->mat[4]  - matrix[4],  this->mat[5]  - matrix[5],  this->mat[6]  - matrix[6],  this->mat[7]  - matrix[7],
                    this->mat[8]  - matrix[8],  this->mat[9]  - matrix[9],  this->mat[10] - matrix[10], this->mat[11] - matrix[11],
                    this->mat[12] - matrix[12], this->mat[13] - matrix[13], this->mat[14] - matrix[14], this->mat[15] - matrix[15]);
    }

    inline T &element(I8 row, I8 column, bool rowMajor = false)	{
        //return (rowMajor ? this->m[column][row] : this->m[row][column]);
        return (rowMajor ? this->mat[column+row*4] : this->mat[row+column*4]);
    }

    inline const T &element(I8 row, I8 column, bool rowMajor = false) const {
        //return (rowMajor ? this->m[column][row] : this->m[row][column]);
        return (rowMajor ? this->mat[column+row*4] : this->mat[row+column*4]);
    }

    mat4 &operator*=(T f)                { return *this = *this * f; }
    mat4 &operator*=(const mat4 &matrix) { return *this = *this * matrix; }
    mat4 &operator+=(const mat4 &matrix) { return *this = *this + matrix; }
    mat4 &operator-=(const mat4 &matrix) { return *this = *this - matrix; }

    operator T*()             { return this->mat; }
    operator const T*() const { return this->mat; }

    T &operator[](I32 i)             { return this->mat[i]; }
    const T &operator[](I32 i) const { return this->mat[i]; }

    inline mat4 rotation(void) const {
        return mat4(this->mat[0], this->mat[1], this->mat[2],  0,
                    this->mat[4], this->mat[5], this->mat[6],  0,
                    this->mat[8], this->mat[9], this->mat[10], 0,
                    0,            0,            0,             1);
    }
   
    inline void transpose(mat4& out) const {
        out.set(this->transpose());
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

    inline T det(void)                      const { return Util::Mat4::det(this->mat); }
    inline mat4 getInverse()                const { mat4 ret; this->inverse(ret); return ret;}
    inline void inverse(mat4& ret)          const { Util::Mat4::Inverse(this->mat, ret.mat); }
    inline void inverse()                         { mat4 ret; this->inverse(ret); this->set(ret.mat); }
    inline void zero()                            { memset(this->mat, 0.0, sizeof(T) * 16); }
    inline mat4 getInverseTranspose()       const { mat4 ret; this->inverse(ret); return ret.transpose(); }
    inline void inverseTranspose(mat4& ret) const { ret.set(mat); ret.inverseTranspose(); }
    inline void inverseTranspose()                { this->inverse(); this->transpose(); }

    inline void identity() {
        zero();
        this->mat[0]  = 1.0;
        this->mat[5]  = 1.0;
        this->mat[10] = 1.0;
        this->mat[15] = 1.0;
    }
    
    inline void bias() {
        set(0.5, 0.0, 0.0, 0.0,
            0.0, 0.5, 0.0, 0.0,
            0.0, 0.0, 0.5, 0.0,
            0.5, 0.5, 0.5, 1.0);
    }

    inline void setTranslation(const vec3<T> &v)                            { setTranslation(v.x, v.y, v.z); }
    inline void setScale(const vec3<T> &v)                                  { setScale(v.x, v.y, v.z); }
    inline void scale(const vec3<T> &v)                                     { scale(v.x, v.y, v.z); }
    inline void translate(const vec3<T> &v)                                 { translate(v.x, v.y, v.z); }
    inline void rotate(const vec3<T> &axis, T angle, bool inDegrees = true) { rotate(axis.x, axis.y, axis.z, angle, inDegrees); }

    inline void rotate(T x,T y,T z,T angle, bool inDegrees = true) {
        if (inDegrees) DegToRad(angle);

        T c = (T)cos(angle);
        T s = (T)sin(angle);
        vec3<T> v(x,y,z);
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
        this->mat[0] = (1 - c) * xx + c;  this->mat[4] = (1 - c) * xy - zs; this->mat[8] = (1 - c) * zx + ys;
        this->mat[1] = (1 - c) * xy + zs; this->mat[5] = (1 - c) * yy + c;  this->mat[9] = (1 - c) * yz - xs;
        this->mat[2] = (1 - c) * zx - ys; this->mat[6] = (1 - c) * yz + xs; this->mat[10] = (1 - c) * zz + c;
    }

    inline void rotate_x(T angle, bool inDegrees = true) {
        if(inDegrees) DegToRad(angle);

        T c = (T)cos(angle);
        T s = (T)sin(angle);
        this->mat[5] =  c;  this->mat[9]  = -s;
        this->mat[6] =  s;  this->mat[10] =  c;
    }

    inline void rotate_y(T angle, bool inDegrees = true) {
        if(inDegrees) DegToRad(angle);

        T c = (T)cos(angle);
        T s = (T)sin(angle);
        this->mat[0] =  c; this->mat[8]  =  s;
        this->mat[2] = -s; this->mat[10] =  c;
    }

    inline void rotate_z(T angle, bool inDegrees = true) {
        if(inDegrees) DegToRad(angle);

        T c = (T)cos(angle);
        T s = (T)sin(angle);
        this->mat[0] =  c;  this->mat[4] = -s;
        this->mat[1] =  s;  this->mat[5] =  c;
    }

    inline void setScale(T x, T y, T z) {
        this->mat[0]  = x;
        this->mat[5]  = y;
        this->mat[10] = z;
    }

    inline void scale(T x,T y,T z) {
        this->mat[0] *= x; this->mat[4] *= y; this->mat[8]  *= z;
        this->mat[1] *= x; this->mat[5] *= y; this->mat[9]  *= z;
        this->mat[2] *= x; this->mat[6] *= y; this->mat[10] *= z;
        this->mat[3] *= x; this->mat[7] *= y; this->mat[11] *= z;
    }

    inline void setTranslation(T x, T y, T z) {
        this->mat[12] = x;
        this->mat[13] = y;
        this->mat[14] = z;
    }

    inline void translate(T x,T y,T z) {
        this->mat[12] += x;
        this->mat[13] += y;
        this->mat[14] += z;
    }

    void reflect(const Plane<T>& plane) {
        const vec4<T>& eq = plane.getEquation();
        T x = eq.x;
        T y = eq.y;
        T z = eq.z;
        T x2 = x * 2.0f;
        T y2 = y * 2.0f;
        T z2 = z * 2.0f;
        this->mat[0] = 1 - x * x2; this->mat[4] = -y * x2;    this->mat[8] = -z * x2;      this->mat[12] = -eq.w * x2;
        this->mat[1] = -x * y2;    this->mat[5] = 1 - y * y2; this->mat[9] = -z * y2;      this->mat[13] = -eq.w * y2;
        this->mat[2] = -x * z2;    this->mat[6] = -y * z2;    this->mat[10] = 1 - z * z2;  this->mat[14] = -eq.w * z2;
        this->mat[3] = 0;          this->mat[7] = 0;          this->mat[11] = 0;           this->mat[15] = 1;
    }

    void lookAt(const vec3<T>& eye, const vec3<T>& target, const vec3<T>& up) {
        vec3<T> xAxis, yAxis, zAxis, viewDir;
        zAxis = eye - target;
        zAxis.normalize();
        xAxis.cross(up, zAxis);
        xAxis.normalize();
        yAxis.cross(zAxis, xAxis);
        yAxis.normalize();
        viewDir = -zAxis;

        this->m[0][0] = xAxis.x;
        this->m[1][0] = xAxis.y;
        this->m[2][0] = xAxis.z;
        this->m[3][0] = -xAxis.dot(eye);

        this->m[0][1] = yAxis.x;
        this->m[1][1] = yAxis.y;
        this->m[2][1] = yAxis.z;
        this->m[3][1] = -yAxis.dot(eye);

        this->m[0][2] = zAxis.x;
        this->m[1][2] = zAxis.y;
        this->m[2][2] = zAxis.z;
        this->m[3][2] = -zAxis.dot(eye);
    }

	void ortho(T left, T right, T bottom, T top, T zNear, T zFar ) {
		identity();

		m[0][0] = static_cast<T>(2) / (right - left);
		m[1][1] = static_cast<T>(2) / (top - bottom);
		m[2][2] = - T(2) / (zFar - zNear);
		m[3][0] = - (right + left) / (right - left);
		m[3][1] = - (top + bottom) / (top - bottom);
		m[3][2] = - (zFar + zNear) / (zFar - zNear);
	}

	void perspective(T fovyRad, T aspect, T zNear,T zFar) {
		assert(!IS_ZERO(aspect));
		assert(zFar > zNear);

		T tanHalfFovy = tan(fovyRad / static_cast<T>(2));

		zero();

		m[0][0] =    static_cast<T>(1) / (aspect * tanHalfFovy);
		m[1][1] =    static_cast<T>(1) / (tanHalfFovy);
		m[2][2] = - (zFar + zNear) / (zFar - zNear);
		m[2][3] = - static_cast<T>(1);
		m[3][2] = - (static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
	}

	void frustum(T left, T right, T bottom, T top, T nearVal, T farVal) {
		zero();

		m[0][0] = (static_cast<T>(2) * nearVal) / (right - left);
		m[1][1] = (static_cast<T>(2) * nearVal) / (top - bottom);
		m[2][0] = (right + left) / (right - left);
		m[2][1] = (top + bottom) / (top - bottom);
		m[2][2] = -(farVal + nearVal) / (farVal - nearVal);
		m[2][3] = static_cast<T>(-1);
		m[3][2] = -(static_cast<T>(2) * farVal * nearVal) / (farVal - nearVal);
	}

    inline void reflect(T x,T y,T z,T w) {
        reflect(Plane<T>(x,y,z,w));
    }

    inline void extractMat3(mat3<T>& matrix3) const {
        matrix3.m[0][0] = this->m[0][0];
        matrix3.m[0][1] = this->m[0][1];
        matrix3.m[0][2] = this->m[0][2];
        matrix3.m[1][0] = this->m[1][0];
        matrix3.m[1][1] = this->m[1][1];
        matrix3.m[1][2] = this->m[1][2];
        matrix3.m[2][0] = this->m[2][0];
        matrix3.m[2][1] = this->m[2][1];
        matrix3.m[2][2] = this->m[2][2];
    }

    inline void swap(mat4& B){
        for(U8 i = 0; i < 4; i++){
            std::swap(this->m[i][0], B.m[i][0]);
            std::swap(this->m[i][1], B.m[i][1]);
            std::swap(this->m[i][2], B.m[i][2]);
            std::swap(this->m[i][3], B.m[i][3]);
        }
    }

    union {
        struct {
             T _11, _12, _13, _14;   // standard names for components
             T _21, _22, _23, _24;   // standard names for components
             T _31, _32, _33, _34;   // standard names for components
             T _41, _42, _43, _44;   // standard names for components
         };
         T mat[16];
         T m[4][4];
     };
};

/// Converts a point from world coordinates to projection coordinates
///(from Y = depth, Z = up to Y = up, Z = depth)
template <class T>
inline void projectPoint(const vec3<T>& position,vec3<T>& output){
    output.x = position.x;
    output.y = position.z;
    output.z = position.y;
}

struct Line {
    vec3<F32> _startPoint;
    vec3<F32> _endPoint;
    vec4<U8 > _color;
    Line()
    {
    }
    Line(const vec3<F32>& startPoint, const vec3<F32>& endPoint, const vec4<U8>& color) : _startPoint(startPoint),
                                                                                          _endPoint(endPoint),
                                                                                          _color(color)
    {
    }
};

namespace Util{
    inline F32 Lerp(const F32 v1, const F32 v2, const F32 t)
    {
        return v1 + (v2 - v1*t);
    }
    namespace Mat4 {
                // ----------------------------------------------------------------------------------------
            template <typename T>
            inline void decompose(const mat4<T>& matrix, vec3<T>& scale, Quaternion<T>& rotation, vec3<T>& position) {
    
                // extract translation
                position.x = matrix.m[0][3];
                position.y = matrix.m[1][3];
                position.z = matrix.m[2][3];

                // extract the rows of the matrix
                vec3<T> vRows[3] = {
                    vec3<T>(matrix.m[0][0],matrix.m[1][0],matrix.m[2][0]),
                    vec3<T>(matrix.m[0][1],matrix.m[1][1],matrix.m[2][1]),
                    vec3<T>(matrix.m[0][2],matrix.m[1][2],matrix.m[2][2])
                };

                // extract the scaling factors
                scale.x = vRows[0].length();
                scale.y = vRows[1].length();
                scale.z = vRows[2].length();

                // and the sign of the scaling
                if (matrix.det() < 0) {
                    scale.x = -scale.x;
                    scale.y = -scale.y;
                    scale.z = -scale.z;
                }

                // and remove all scaling from the matrix
                if(!IS_ZERO(scale.x))
                {
                    vRows[0] /= scale.x;
                }
                if(!IS_ZERO(scale.y))
                {
                    vRows[1] /= scale.y;
                }
                if(!IS_ZERO(scale.z))
                {
                    vRows[2] /= scale.z;
                }

                // build a 3x3 rotation matrix
                mat3<T> m(vRows[0].x,vRows[1].x,vRows[2].x,
                          vRows[0].y,vRows[1].y,vRows[2].y,
                          vRows[0].z,vRows[1].z,vRows[2].z);

                // and generate the rotation quaternion from it
                rotation = Quaternion<T>(m);
            }

            // ----------------------------------------------------------------------------------------
            template <typename T>
            inline void decomposeNoScaling(const mat4<T>& matrix, Quaternion<T>& rotation,	vec3<T>& position) {
                // extract translation
                position.x = matrix.m[0][3];
                position.y = matrix.m[1][3];
                position.z = matrix.m[2][3];

                // extract rotation
                rotation = Quaterion<T>(mat3<T>(matrix));
            }
    };//Mat4
};//Util
#endif

