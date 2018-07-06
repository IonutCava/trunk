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
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _MATH_MATRICES_H_
#define _MATH_MATRICES_H_

#include "config.h"
#include "Plane.h"

namespace Divide {

/******************************//**
/* mat3
/*********************************/
template<typename T>
class mat3 {
public:
    mat3()
    {
        this->identity();
    }

    mat3( T m0, T m1, T m2,
          T m3, T m4, T m5,
          T m6, T m7, T m8 ) 
    {
        this->mat[0] = m0; this->mat[3] = m3; this->mat[6] = m6;
        this->mat[1] = m1; this->mat[4] = m4; this->mat[7] = m7;
        this->mat[2] = m2; this->mat[5] = m5; this->mat[8] = m8;
    }

    mat3( const T *m ) 
    {
        this->set( m );
    }

    mat3( const mat3 &m ) : mat3( m.mat ) 
    {
    }

    mat3( const mat4<T> &m ) 
    {
        this->set( m );
    }

    vec3<T> operator*( const vec3<T> &v ) const {
        return vec3<T>( this->mat[0] * v[0] + this->mat[3] * v[1] + this->mat[6] * v[2],
                        this->mat[1] * v[0] + this->mat[4] * v[1] + this->mat[7] * v[2],
                        this->mat[2] * v[0] + this->mat[5] * v[1] + this->mat[8] * v[2] );
    }

    vec4<T> operator*( const vec4<T> &v ) const {
        return vec4<T>( this->mat[0] * v[0] + this->mat[3] * v[1] + this->mat[6] * v[2],
                        this->mat[1] * v[0] + this->mat[4] * v[1] + this->mat[7] * v[2],
                        this->mat[2] * v[0] + this->mat[5] * v[1] + this->mat[8] * v[2],
                        v[3] );
    }

    mat3 operator*( T f ) const {
        return mat3( this->mat[0] * f, this->mat[1] * f, this->mat[2] * f,
                     this->mat[3] * f, this->mat[4] * f, this->mat[5] * f,
                     this->mat[6] * f, this->mat[7] * f, this->mat[8] * f );
    }

    mat3 operator*( const mat3 &m ) const {
        return mat3( this->mat[0] * m[0] + this->mat[3] * m[1] + this->mat[6] * m[2],
                     this->mat[1] * m[0] + this->mat[4] * m[1] + this->mat[7] * m[2],
                     this->mat[2] * m[0] + this->mat[5] * m[1] + this->mat[8] * m[2],
                     this->mat[0] * m[3] + this->mat[3] * m[4] + this->mat[6] * m[5],
                     this->mat[1] * m[3] + this->mat[4] * m[4] + this->mat[7] * m[5],
                     this->mat[2] * m[3] + this->mat[5] * m[4] + this->mat[8] * m[5],
                     this->mat[0] * m[6] + this->mat[3] * m[7] + this->mat[6] * m[8],
                     this->mat[1] * m[6] + this->mat[4] * m[7] + this->mat[7] * m[8],
                     this->mat[2] * m[6] + this->mat[5] * m[7] + this->mat[8] * m[8] );
    }

    mat3 operator+( const mat3 &m ) const {
        return mat3( this->mat[0] + m[0], this->mat[1] + m[1], this->mat[2] + m[2],
                     this->mat[3] + m[3], this->mat[4] + m[4], this->mat[5] + m[5],
                     this->mat[6] + m[6], this->mat[7] + m[7], this->mat[8] + m[8] );
    }

    mat3 operator-( const mat3 &m ) const {
        return mat3( this->mat[0] - m[0], this->mat[1] - m[1], this->mat[2] - m[2],
                     this->mat[3] - m[3], this->mat[4] - m[4], this->mat[5] - m[5],
                     this->mat[6] - m[6], this->mat[7] - m[7], this->mat[8] - m[8] );
    }

    mat3 &operator*=( T f )           { return *this = *this * f; }
    mat3 &operator*=( const mat3 &m ) { return *this = *this * m; }
    mat3 &operator+=( const mat3 &m ) { return *this = *this + m; }
    mat3 &operator-=( const mat3 &m ) { return *this = *this - m; }

    bool operator == ( mat3& B ) const {
        if (!FLOAT_COMPARE( this->m[0][0], B[0][0] ) ||
            !FLOAT_COMPARE( this->m[0][1], B[0][1] ) ||
            !FLOAT_COMPARE( this->m[0][2], B[0][2] ) ) {
            return false;
        }
        if (!FLOAT_COMPARE( this->m[1][0], B[1][0] ) ||
            !FLOAT_COMPARE( this->m[1][1], B[1][1] ) ||
            !FLOAT_COMPARE( this->m[1][2], B[1][2] ) ) {
            return false;
        }
        if (!FLOAT_COMPARE( this->m[2][0], B[2][0] ) ||
            !FLOAT_COMPARE( this->m[2][1], B[2][1] ) ||
            !FLOAT_COMPARE( this->m[2][2], B[2][2] ) ) {
            return false;
        }
        return true;
    }

    bool operator != ( mat3& B ) const { return !( *this == B ); }

    operator       T*( )       { return this->mat; }
    operator const T*( ) const { return this->mat; }

          T &operator[]( I32 i )       { return this->mat[i]; }
    const T  operator[]( I32 i ) const { return this->mat[i]; }

    inline void set( T m0, T m1, T m2,
                     T m3, T m4, T m5,
                     T m6, T m7, T m8 ) {
        this->mat[0] = m0; this->mat[3] = m3; this->mat[6] = m6;
        this->mat[1] = m1; this->mat[4] = m4; this->mat[7] = m7;
        this->mat[2] = m2; this->mat[5] = m5; this->mat[8] = m8;
    }

    inline void set( const T *m ) {
        memcpy( this->mat, m, sizeof( T ) * 9 );
    }

    inline void set( const mat4<T> &matrix ) {
        this->mat[0] = matrix[0]; this->mat[3] = matrix[4]; this->mat[6] = matrix[8];
        this->mat[1] = matrix[1]; this->mat[4] = matrix[5]; this->mat[7] = matrix[9];
        this->mat[2] = matrix[2]; this->mat[5] = matrix[6]; this->mat[8] = matrix[10];
    }

    inline mat3 getInverse() const {
        mat3 ret( this->mat );
        ret.inverse();
        return ret;
    }

    inline void getInverse( mat3& ret ) const { 
        ret.set( this->getInverse() );
    }

    inline mat3 getTranspose() const {
        mat3 ret( this->mat );
        ret.transpose();
        return ret;
    }

    inline void getTranspose( mat3& ret ) const {
        ret.set( this->getTranspose() );
    }

    inline mat3 getInverseTranspose() const { 
        return this->getInverse().getTranspose();
    }

    inline void getInverseTranspose( mat3& ret ) const { 
        ret.set( this );
        ret.inverseTranspose();
    }

    inline void inverseTranspose() { 
        this->inverse();
        this->transpose(); 
    }

    inline void transpose() const {
        this->set( this->mat[0], this->mat[3], this->mat[6],
                   this->mat[1], this->mat[4], this->mat[2],
                   this->mat[7], this->mat[5], this->mat[8] );
    }

    inline T det() const {
        return ( ( this->mat[0] * this->mat[4] * this->mat[8] ) +
                 ( this->mat[3] * this->mat[7] * this->mat[2] ) +
                 ( this->mat[6] * this->mat[1] * this->mat[5] ) -
                 ( this->mat[6] * this->mat[4] * this->mat[2] ) -
                 ( this->mat[3] * this->mat[1] * this->mat[8] ) -
                 ( this->mat[0] * this->mat[7] * this->mat[5] ) );
    }

    inline void inverse() const {
        T idet = this->det();
        assert( !IS_ZERO( idet ) );
        idet = static_cast<T>(1) / idet;

        this->set( ( this->mat[4] * this->mat[8] - this->mat[7] * this->mat[5] ) * idet,
                  -( this->mat[1] * this->mat[8] - this->mat[7] * this->mat[2] ) * idet,
                   ( this->mat[1] * this->mat[5] - this->mat[4] * this->mat[2] ) * idet,
                  -( this->mat[3] * this->mat[8] - this->mat[6] * this->mat[5] ) * idet,
                   ( this->mat[0] * this->mat[8] - this->mat[6] * this->mat[2] ) * idet,
                  -( this->mat[0] * this->mat[5] - this->mat[3] * this->mat[2] ) * idet,
                   ( this->mat[3] * this->mat[7] - this->mat[6] * this->mat[4] ) * idet,
                  -( this->mat[0] * this->mat[7] - this->mat[6] * this->mat[1] ) * idet,
                   ( this->mat[0] * this->mat[4] - this->mat[3] * this->mat[1] ) * idet );
    }

    inline void zero() {
        memset( this->mat, 0, 9 * sizeof( T ) );
    }

    inline void identity() {
        this->zero();
        this->mat[0] = 1.0;
        this->mat[4] = 1.0;
        this->mat[8] = 1.0;
    }

    inline bool isIdentity() const {
        return (FLOAT_COMPARE(this->mat[0], 1.0) && IS_ZERO(this->mat[1])            && IS_ZERO(this->mat[2]) &&
                IS_ZERO(this->mat[3])            && FLOAT_COMPARE(this->mat[4], 1.0) && IS_ZERO(this->mat[5]) &&
                IS_ZERO(this->mat[6])            && IS_ZERO(this->mat[7])            && FLOAT_COMPARE(this->mat[8], 1.0));
            
    }

    inline void rotate( const vec3<T> &v, T angle, bool inDegrees = true ) {
        this->rotate( v.x, v.y, v.z, angle, inDegrees );
    }

    void rotate( T x, T y, T z, T angle, bool inDegrees = true ) {
        if ( inDegrees ) {
            DegToRad( angle );
        }

        T c = static_cast<T>(std::cos( angle ));
        T s = static_cast<T>(std::sin( angle ));
        T l = static_cast<T>(std::sqrt(static_cast<D32>( x * x + y * y + z * z ) ));

        l = l < EPSILON ? 1 : 1 / l

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

        this->mat[0] = c1 * x * x + c; this->mat[3] = c1 * xy - zs;   this->mat[6] = c1 * zx + ys;
        this->mat[1] = c1 * xy + zs;   this->mat[4] = c1 * y * y + c; this->mat[7] = c1 * yz - xs;
        this->mat[2] = c1 * zx - ys;   this->mat[5] = c1 * yz + xs;   this->mat[8] = c1 * z * z + c;
    }

    inline void rotate_x( T angle, bool inDegrees = true ) {
        if ( inDegrees ) {
            DegToRad( angle );
        }

        T c = (T)std::cos( angle );
        T s = (T)std::sin( angle );

        this->mat[0] = 1; this->mat[3] = 0; this->mat[6] = 0;
        this->mat[1] = 0; this->mat[4] = c; this->mat[7] = -s;
        this->mat[2] = 0; this->mat[5] = s; this->mat[8] = c;
    }

    inline void rotate_y( T angle, bool inDegrees = true ) {
        if ( inDegrees ) {
            angle = Angle::DegreesToRadians( angle );
        }

        T c = (T)std::cos( angle );
        T s = (T)std::sin( angle );

        this->mat[0] = c;  this->mat[3] = 0; this->mat[6] = s;
        this->mat[1] = 0;  this->mat[4] = 1; this->mat[7] = 0;
        this->mat[2] = -s; this->mat[5] = 0; this->mat[8] = c;
    }

    inline void rotate_z( T angle, bool inDegrees = true ) {
        if ( inDegrees ) {
            DegToRad( angle );
        }

        T c = (T)std::cos( angle );
        T s = (T)std::sin( angle );

        this->mat[0] = c; this->mat[3] = -s; this->mat[6] = 0;
        this->mat[1] = s; this->mat[4] = c;  this->mat[7] = 0;
        this->mat[2] = 0; this->mat[5] = 0;  this->mat[8] = 1;
    }

    inline void setScale( T x, T y, T z ) {
        this->mat[0] = x;
        this->mat[4] = y;
        this->mat[8] = z;
    }

    inline void setScale( const vec3<T> &v ) {
        this->scale( v.x, v.y, v.z );
    }

    void orthonormalize( void ) {
        vec3<T> x( mat[0], mat[1], mat[2] );
        x.normalize();
        vec3<T> y( mat[3], mat[4], mat[5] );
        vec3<T> z( cross( x, y ) );
        z.normalize();
        y.cross( z, x );
        y.normalize();

        this->mat[0] = x.x; this->mat[3] = y.x; this->mat[6] = z.x;
        this->mat[1] = x.y; this->mat[4] = y.y; this->mat[7] = z.y;
        this->mat[2] = x.z; this->mat[5] = y.z; this->mat[8] = z.z;
    }

    inline void swap( mat3& B ) {
        std::swap( this->m[0][0], B.m[0][0] );
        std::swap( this->m[0][1], B.m[0][1] );
        std::swap( this->m[0][2], B.m[0][2] );

        std::swap( this->m[1][0], B.m[1][0] );
        std::swap( this->m[1][1], B.m[1][1] );
        std::swap( this->m[1][2], B.m[1][2] );

        std::swap( this->m[2][0], B.m[2][0] );
        std::swap( this->m[2][1], B.m[2][1] );
        std::swap( this->m[2][2], B.m[2][2] );
    }

    union {
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
template<typename T>
class mat4 {
public:
    mat4() 
    {
        this->identity();
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
        this->set( m );
    }

    mat4(const mat4 &m) : mat4(m.mat)
    {
    }

    mat4(const vec4<T> &col1,const vec4<T> &col2,const vec4<T> &col3,const vec4<T> &col4)
    {
        this->setCol( 0, col1 );
        this->setCol( 1, col2 );
        this->setCol( 2, col3 );
        this->setCol( 3, col4 );
    }

    mat4(const vec3<T> &translation, const vec3<T> &scale) : mat4(translation)
    {
        this->setScale( scale );
    }

    mat4(const vec3<T> &translation) : mat4(translation.x, translation.y, translation.z)
    {
    }

    mat4(T translationX, T translationY, T translationZ) : mat4()
    {
        this->setTranslation( translationX, translationY, translationZ );
    }

    mat4(const vec3<T> &axis, T angle, bool inDegrees = true) : mat4(axis.x, axis.y, axis.z, angle, inDegrees)
    {
    }

    mat4(T x, T y, T z, T angle, bool inDegrees = true) : mat4()
    {
        this->rotate( x, y, z, angle, inDegrees );
    }

    mat4(const vec3<T>& eye, const vec3<T>& target, const vec3<T>& up) : mat4()
    {
        this->lookAt( eye, target, up );
    }

    mat4(const mat3<T> &m) 
    {
        this->set( m );
    }

    /*Transforms the given 3-D vector by the matrix, projecting the result back into <i>w</i> = 1. (OGRE reference)*/
    inline vec3<T> transform( const vec3<T> &v ) const {
        T fInvW = 1.0f / ( this->m[0][3] * v.x + this->m[1][3] * v.y + this->m[2][3] * v.z + this->m[3][3] );

        return vec3<T>( ( this->m[0][0] * v.x + this->m[1][1] * v.y + this->m[2][0] * v.z + this->m[3][0] ) * fInvW,
                        ( this->m[0][1] * v.x + this->m[1][1] * v.y + this->m[2][1] * v.z + this->m[3][1] ) * fInvW,
                        ( this->m[0][2] * v.x + this->m[1][2] * v.y + this->m[2][2] * v.z + this->m[3][2] ) * fInvW );
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
        // Add a small epsilon value to avoid 0.0 != 0.0
        if (!FLOAT_COMPARE(this->elementSum() + EPSILON_F32, B.elementSum() + EPSILON_F32)) {
            return false;
        }
        
        if (!FLOAT_COMPARE(this->m[0][0], B.m[0][0]) ||
            !FLOAT_COMPARE(this->m[0][1], B.m[0][1]) ||
            !FLOAT_COMPARE(this->m[0][2], B.m[0][2]) ||
            !FLOAT_COMPARE(this->m[0][3], B.m[0][3]) ){
            return false;
        }
        if (!FLOAT_COMPARE( this->m[1][0], B.m[1][0] ) ||
            !FLOAT_COMPARE( this->m[1][1], B.m[1][1] ) ||
            !FLOAT_COMPARE( this->m[1][2], B.m[1][2] ) ||
            !FLOAT_COMPARE( this->m[1][3], B.m[1][3] ) ) {
            return false;
        }
        if (!FLOAT_COMPARE( this->m[2][0], B.m[2][0] ) ||
            !FLOAT_COMPARE( this->m[2][1], B.m[2][1] ) ||
            !FLOAT_COMPARE( this->m[2][2], B.m[2][2] ) ||
            !FLOAT_COMPARE( this->m[2][3], B.m[2][3] ) ) {
            return false;
        }
        if (!FLOAT_COMPARE( this->m[3][0], B.m[3][0] ) ||
            !FLOAT_COMPARE( this->m[3][1], B.m[3][1] ) ||
            !FLOAT_COMPARE( this->m[3][2], B.m[3][2] ) ||
            !FLOAT_COMPARE( this->m[3][3], B.m[3][3] ) ) {
            return false;
        }
        
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
        this->mat[0] = matrix.mat[0]; this->mat[4] = matrix.mat[3]; this->mat[8]  = matrix.mat[6]; this->mat[12] = 0.0;
        this->mat[1] = matrix.mat[1]; this->mat[5] = matrix.mat[4]; this->mat[9]  = matrix.mat[7]; this->mat[13] = 0.0;
        this->mat[2] = matrix.mat[2]; this->mat[6] = matrix.mat[5]; this->mat[10] = matrix.mat[8]; this->mat[14] = 0.0;
        this->mat[3] = 0.0;           this->mat[7] = 0.0;           this->mat[11] = 0.0;           this->mat[15] = 1.0;
    }

    inline F32* getRow(I32 index) {
        return this->m[index];
    }
    
    inline void setRow( I32 index, const vec4<T>& value ) {
        this->m[index][0] = value.x;
        this->m[index][1] = value.y;
        this->m[index][2] = value.z;
        this->m[index][3] = value.w;
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

    mat4 operator+(const mat4 &matrix) const {
        F32* m = this->mat;
        return mat4(m[ 0] + matrix[ 0], m[ 1] + matrix[ 1], m[ 2] + matrix[ 2], m[ 3] + matrix[ 3],
                    m[ 4] + matrix[ 4], m[ 5] + matrix[ 5], m[ 6] + matrix[ 6], m[ 7] + matrix[ 7],
                    m[ 8] + matrix[ 8], m[ 9] + matrix[ 9], m[10] + matrix[10], m[11] + matrix[11],
                    m[12] + matrix[12], m[13] + matrix[13], m[14] + matrix[14], m[15] + matrix[15]);
    }

    mat4 operator-(const mat4 &matrix) const {
        F32* m = this->mat;
        return mat4(m[ 0] - matrix[ 0], m[ 1] - matrix[ 1], m[ 2] - matrix[ 2], m[ 3] - matrix[ 3],
                    m[ 4] - matrix[ 4], m[ 5] - matrix[ 5], m[ 6] - matrix[ 6], m[ 7] - matrix[ 7],
                    m[ 8] - matrix[ 8], m[ 9] - matrix[ 9], m[10] - matrix[10], m[11] - matrix[11],
                    m[12] - matrix[12], m[13] - matrix[13], m[14] - matrix[14], m[15] - matrix[15]);
    }

    inline T &element(I8 row, I8 column, bool rowMajor = false)    {
        return ( rowMajor ? this->mat[column + row * 4] : this->mat[row + column * 4] );
    }

    inline const T &element(I8 row, I8 column, bool rowMajor = false) const {
        return ( rowMajor ? this->mat[column + row * 4] : this->mat[row + column * 4] );
    }

    mat4 &operator*=(T f)                { return *this = *this * f; }
    mat4 &operator*=(const mat4 &matrix) { return *this = *this * matrix; }
    mat4 &operator+=(const mat4 &matrix) { return *this = *this + matrix; }
    mat4 &operator-=(const mat4 &matrix) { return *this = *this - matrix; }

    operator       T*()       { return this->mat; }
    operator const T*() const { return this->mat; }

          T &operator[](I32 i)       { return this->mat[i]; }
    const T &operator[](I32 i) const { return this->mat[i]; }

    inline mat4 getRotation(void) const {
        return mat4(this->mat[0], this->mat[1], this->mat[2],  0,
                    this->mat[4], this->mat[5], this->mat[6],  0,
                    this->mat[8], this->mat[9], this->mat[10], 0,
                    0,            0,            0,             1);
    }

    inline void transpose() {
        this->set( this->mat[0], this->mat[4], this->mat[8], this->mat[12],
                   this->mat[1], this->mat[5], this->mat[9], this->mat[13],
                   this->mat[2], this->mat[6], this->mat[10], this->mat[14],
                   this->mat[3], this->mat[7], this->mat[11], this->mat[15] );
    }

    inline mat4 transposeRotation() const {
        this->set( this->mat[0], this->mat[4],   this->mat[8],  this->mat[3],
                   this->mat[1], this->mat[5],   this->mat[9],  this->mat[7],
                   this->mat[2], this->mat[6],   this->mat[10], this->mat[11],
                   this->mat[12], this->mat[13], this->mat[14], this->mat[15] );
    }

    inline void getTranspose(mat4& out) const {
        out.set(this->getTranspose());
    }

    inline mat4 getTranspose() const {
        return mat4(this->mat[0], this->mat[4], this->mat[8],  this->mat[12],
                    this->mat[1], this->mat[5], this->mat[9],  this->mat[13],
                    this->mat[2], this->mat[6], this->mat[10], this->mat[14],
                    this->mat[3], this->mat[7], this->mat[11], this->mat[15]);
    }

    inline mat4 getTransposeRotation() const {
        return mat4(this->mat[0],  this->mat[4],  this->mat[8],  this->mat[3],
                    this->mat[1],  this->mat[5],  this->mat[9],  this->mat[7],
                    this->mat[2],  this->mat[6],  this->mat[10], this->mat[11],
                    this->mat[12], this->mat[13], this->mat[14], this->mat[15]);
    }

    inline T det(void) const {
        return Util::Mat4::det(this->mat); 
    }

    inline mat4 getInverse() const {
        mat4 ret( this->mat );
        ret.inverse(); 
        return ret;
    }

    inline void getInverse(mat4& ret) const {
        Util::Mat4::Inverse( this->mat, ret.mat );
    }

    inline void inverse() {
        Util::Mat4::Inverse( this->mat, this->mat );
    }

    inline void zero() {
        memset( this->mat, 0.0, sizeof( T ) * 16 );
    }

    inline mat4 getInverseTranspose() const {
        return this->getInverse().getTranspose();
    }

    inline void inverseTranspose(mat4& ret) const {
        ret.set(this->getInverseTranspose());
    }

    inline void inverseTranspose() {
        this->inverse(); 
        this->transpose();
    }

    inline void identity() {
        this->zero();
        this->mat[0]  = 1.0;
        this->mat[5]  = 1.0;
        this->mat[10] = 1.0;
        this->mat[15] = 1.0;
    }
    
    inline void bias() {
        this->set( 0.5, 0.0, 0.0, 0.0,
                   0.0, 0.5, 0.0, 0.0,
                   0.0, 0.0, 0.5, 0.0,
                   0.5, 0.5, 0.5, 1.0 );
    }

    inline void setTranslation( const vec3<T> &v ) { 
        this->setTranslation( v.x, v.y, v.z );
    }

    inline void setScale( const vec3<T> &v ) {
        this->setScale( v.x, v.y, v.z );
    }

    inline void scale( const vec3<T> &v ) {
        this->scale( v.x, v.y, v.z );
    }

    inline void translate( const vec3<T> &v ) {
        this->translate( v.x, v.y, v.z );
    }

    inline void rotate( const vec3<T> &axis, T angle, bool inDegrees = true ) {
        this->rotate( axis.x, axis.y, axis.z, angle, inDegrees );
    }

    inline void rotate( T x, T y, T z, T angle, bool inDegrees = true ) {
        if ( inDegrees ) {
            DegToRad( angle );
        }

        vec3<T> v( x, y, z );
        v.normalize();

        T c = (T)std::cos( angle );
        T s = (T)std::sin( angle );

        T xx = v.x * v.x;
        T yy = v.y * v.y;
        T zz = v.z * v.z;
        T xy = v.x * v.y;
        T yz = v.y * v.z;
        T zx = v.z * v.x;
        T xs = v.x * s;
        T ys = v.y * s;
        T zs = v.z * s;

        this->mat[0] = ( 1 - c ) * xx + c;  this->mat[4] = ( 1 - c ) * xy - zs; this->mat[8]  = ( 1 - c ) * zx + ys;
        this->mat[1] = ( 1 - c ) * xy + zs; this->mat[5] = ( 1 - c ) * yy + c;  this->mat[9]  = ( 1 - c ) * yz - xs;
        this->mat[2] = ( 1 - c ) * zx - ys; this->mat[6] = ( 1 - c ) * yz + xs; this->mat[10] = ( 1 - c ) * zz + c;
    }

    inline void rotate_x( T angle, bool inDegrees = true ) {
        if ( inDegrees ) {
            DegToRad( angle );
        }

        T c = (T)std::cos( angle );
        T s = (T)std::sin( angle );

        this->mat[5] = c;  this->mat[9]  = -s;
        this->mat[6] = s;  this->mat[10] = c;
    }

    inline void rotate_y( T angle, bool inDegrees = true ) {
        if ( inDegrees ) {
            DegToRad( angle );
        }

        T c = (T)std::cos( angle );
        T s = (T)std::sin( angle );

        this->mat[0] = c;  this->mat[8]  = s;
        this->mat[2] = -s; this->mat[10] = c;
    }

    inline void rotate_z( T angle, bool inDegrees = true ) {
        if ( inDegrees ) {
            DegToRad( angle );
        }

        T c = (T)std::cos( angle );
        T s = (T)std::sin( angle );

        this->mat[0] = c;  this->mat[4] = -s;
        this->mat[1] = s;  this->mat[5] = c;
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
        T w = eq.w;
        T x2 = x * 2.0f;
        T y2 = y * 2.0f;
        T z2 = z * 2.0f;

        this->mat[0] = 1 - x * x2; this->mat[4] = -y * x2;    this->mat[8] = -z * x2;      this->mat[12] = -w * x2;
        this->mat[1] = -x * y2;    this->mat[5] = 1 - y * y2; this->mat[9] = -z * y2;      this->mat[13] = -w * y2;
        this->mat[2] = -x * z2;    this->mat[6] = -y * z2;    this->mat[10] = 1 - z * z2;  this->mat[14] = -w * z2;
        this->mat[3] = 0;          this->mat[7] = 0;          this->mat[11] = 0;           this->mat[15] = 1;
    }

    void lookAt( const vec3<T>& eye, const vec3<T>& target, const vec3<T>& up ) {
        vec3<T> zAxis( eye - target );
        zAxis.normalize();
        vec3<T> xAxis( cross( up, zAxis ) );
        xAxis.normalize();
        vec3<T> yAxis( cross( zAxis, xAxis ) );
        yAxis.normalize();

        this->m[0][0] = xAxis.x;
        this->m[1][0] = xAxis.y;
        this->m[2][0] = xAxis.z;
        this->m[3][0] = -xAxis.dot( eye );

        this->m[0][1] = yAxis.x;
        this->m[1][1] = yAxis.y;
        this->m[2][1] = yAxis.z;
        this->m[3][1] = -yAxis.dot( eye );

        this->m[0][2] = zAxis.x;
        this->m[1][2] = zAxis.y;
        this->m[2][2] = zAxis.z;
        this->m[3][2] = -zAxis.dot( eye );
    }

    void ortho(T left, T right, T bottom, T top, T zNear, T zFar ) {
        this->identity();

        this->m[0][0] = static_cast<T>( 2 ) / ( right - left );
        this->m[1][1] = static_cast<T>( 2 ) / ( top - bottom );
        this->m[2][2] = -T( 2 ) / ( zFar - zNear );
        this->m[3][0] = -( right + left ) / ( right - left );
        this->m[3][1] = -( top + bottom ) / ( top - bottom );
        this->m[3][2] = -( zFar + zNear ) / ( zFar - zNear );
    }

    void perspective(T fovyRad, T aspect, T zNear,T zFar) {
        assert(!IS_ZERO(aspect));
        assert(zFar > zNear);

        T tanHalfFovy = std::tan(fovyRad / static_cast<T>(2));

        this->zero();

        this->m[0][0] = static_cast<T>( 1 ) / ( aspect * tanHalfFovy );
        this->m[1][1] = static_cast<T>( 1 ) / ( tanHalfFovy );
        this->m[2][2] = -( zFar + zNear ) / ( zFar - zNear );
        this->m[2][3] = -static_cast<T>( 1 );
        this->m[3][2] = -( static_cast<T>( 2 ) * zFar * zNear ) / ( zFar - zNear );
    }

    void frustum( T left, T right, T bottom, T top, T nearVal, T farVal ) {
        this->zero();

        this->m[0][0] = ( static_cast<T>( 2 ) * nearVal ) / ( right - left );
        this->m[1][1] = ( static_cast<T>( 2 ) * nearVal ) / ( top - bottom );
        this->m[2][0] = ( right + left ) / ( right - left );
        this->m[2][1] = ( top + bottom ) / ( top - bottom );
        this->m[2][2] = -( farVal + nearVal ) / ( farVal - nearVal );
        this->m[2][3] = static_cast<T>( -1 );
        this->m[3][2] = -( static_cast<T>( 2 ) * farVal * nearVal ) / ( farVal - nearVal );
    }

    inline void reflect( T x, T y, T z, T w ) {
        this->reflect( Plane<T>( x, y, z, w ) );
    }

    inline void extractMat3( mat3<T>& matrix3 ) const {
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

    inline void swap( mat4& B ) {
        std::swap( this->m[0][0], B.m[0][0] );
        std::swap( this->m[0][1], B.m[0][1] );
        std::swap( this->m[0][2], B.m[0][2] );
        std::swap( this->m[0][3], B.m[0][3] );

        std::swap( this->m[1][0], B.m[1][0] );
        std::swap( this->m[1][1], B.m[1][1] );
        std::swap( this->m[1][2], B.m[1][2] );
        std::swap( this->m[1][3], B.m[1][3] );

        std::swap( this->m[2][0], B.m[2][0] );
        std::swap( this->m[2][1], B.m[2][1] );
        std::swap( this->m[2][2], B.m[2][2] );
        std::swap( this->m[2][3], B.m[2][3] );

        std::swap( this->m[3][0], B.m[3][0] );
        std::swap( this->m[3][1], B.m[3][1] );
        std::swap( this->m[3][2], B.m[3][2] );
        std::swap( this->m[3][3], B.m[3][3] );
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


struct Line {
    vec3<F32> _startPoint;
    vec3<F32> _endPoint;
    vec4<U8 > _color;

    Line()
    {
    }

    Line( const vec3<F32>& startPoint, 
          const vec3<F32>& endPoint, 
          const vec4<U8>& color ) : _startPoint( startPoint ),
                                    _endPoint( endPoint ),
                                    _color( color ) 
    {
    }
};

namespace Util {
    /// Converts a point from world coordinates to projection coordinates
    ///(from Y = depth, Z = up to Y = up, Z = depth)
    template<typename T>
    inline void projectPoint( const vec3<T>& position, vec3<T>& output ) {
        output.set( position.x, position.z, position.y );
    }
    inline F32 Lerp( const F32 v1, const F32 v2, const F32 t ) {
        return v1 + ( v2 - v1*t );
    }

    namespace Mat4 {
        // ----------------------------------------------------------------------------------------
        template<typename T>
        inline void decompose( const mat4<T>& matrix, 
                               vec3<T>& scale, 
                               Quaternion<T>& rotation, 
                               vec3<T>& position ) {

            // extract translation
            position.set( matrix.m[0][3], matrix.m[1][3], matrix.m[2][3] );

            // extract the rows of the matrix
            vec3<T> vRows[3] = {
                vec3<T>( matrix.m[0][0], matrix.m[1][0], matrix.m[2][0] ),
                vec3<T>( matrix.m[0][1], matrix.m[1][1], matrix.m[2][1] ),
                vec3<T>( matrix.m[0][2], matrix.m[1][2], matrix.m[2][2] )
            };

            // extract the scaling factors
            scale.set( vRows[0].length(), vRows[1].length(), vRows[2].length() );

            // and the sign of the scaling
            if ( matrix.det() < 0 ) {
                scale.set( -scale );
            }

            // and remove all scaling from the matrix
            if ( !IS_ZERO( scale.x ) ) {
                vRows[0] /= scale.x;
            }
            if ( !IS_ZERO( scale.y ) ) {
                vRows[1] /= scale.y;
            }
            if ( !IS_ZERO( scale.z ) ) {
                vRows[2] /= scale.z;
            }

            // build a 3x3 rotation matrix and generate the rotation quaternion from it
            rotation = Quaternion<T>( mat3<T>( vRows[0].x, vRows[1].x, vRows[2].x,
                                               vRows[0].y, vRows[1].y, vRows[2].y,
                                               vRows[0].z, vRows[1].z, vRows[2].z ));

        }

        // ----------------------------------------------------------------------------------------
        template<typename T>
        inline void decomposeNoScaling(const mat4<T>& matrix, 
                                       Quaternion<T>& rotation,
                                       vec3<T>& position) {
            // extract translation
            position.set( matrix.m[0][3], matrix.m[1][3], matrix.m[2][3] );
            // extract rotation
            rotation = Quaterion<T>( mat3<T>( matrix ) );
        }
    };//Mat4
};//Util

}; //namespace Divide

#endif

