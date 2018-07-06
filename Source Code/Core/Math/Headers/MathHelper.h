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

/*Code references:
    Matrix inverse: http://www.devmaster.net/forums/showthread.php?t=14569
    Matrix multiply:  http://fhtr.blogspot.com/2010/02/4x4-float-matrix-multiplication-using.html
    Square root: http://www.codeproject.com/KB/cpp/Sqrt_Prec_VS_Speed.aspx
*/

#ifndef _MATH_HELPER_H_
#define _MATH_HELPER_H_

#include "Hardware/Platform/Headers/PlatformDefines.h"
#include <string>
#include <sstream>
#include "Utility/Headers/Vector.h"

#define M_PIDIV2			1.570796326794896619231321691639f		//  PI / 2
#define M_2PI				6.283185307179586476925286766559f		//  2 * PI
#define M_PI2				9.869604401089358618834490999876f		//  PI ^ 2
#define M_PIDIV180			0.01745329251994329576923690768488f		//  PI / 180
#define M_180DIVPI			57.295779513082320876798154814105f		//  180 / PI
#define M_PIDIV360          0.00872664625997164788461845384244f     //  PI / 180 / 2 - PI / 360

template<typename T>
inline void DegToRad(T& a) { a*=M_PIDIV180; }
template<typename T>
inline void RadToDeg(T& a) { a*=M_180DIVPI; }
template<typename T>
inline F32 RADIANS(T a)	   { return a*M_PIDIV180;}
template<typename T>
inline F32 DEGREES(T a)	   { return a*M_180DIVPI; }
template<typename T>
inline F32 kilometre(T a)  { return a*1000.0f; }
template<typename T>
inline F32 metre(T a)	   { return a*1.0f; }
template<typename T>
inline F32 decimetre(T a)  { return a*0.1f; }
template<typename T>
inline F32 centimetre(T a) { return a*0.01f; }
template<typename T>
inline F32 millimeter(T a) { return a*0.001f; }
template<typename T>
inline D32 getUsToSec(T a) { return a * 0.000001; }
template<typename T>
inline D32 getUsToMs(T a)  { return getMsToSec(a); }
template<typename T>
inline D32 getMsToSec(T a) { return a*0.001; }
template<typename T>
inline D32 getSecToMs(T a) { return a*1000.0; }
template<typename T>
inline D32 getSecToUs(T a) { return a * 1000000.0;}
template<typename T>
inline D32 getMsToUs(T a)  { return getSecToMs(a);}
template<typename T>
inline void MsToSec(T& a)  { a*=0.001; }
template<typename T>
inline void SecToMs(T& a)  { a*=1000.0; }
template<typename T>
inline void UsToSec(T& a)  { a*=0.000001; }
template<typename T>
inline void SecToUs(T& a)  { a*=1000000.0; }
template<typename T>
inline void UsToMs(T& a)   { MsToSec(a); }
template<typename T>
inline void MsToUs(T& a)   { SecToMs(a); }

const  F32 INV_RAND_MAX = 1.0 / (RAND_MAX + 1);

inline F32 random(F32 max=1.0)      { 
    return max * rand() * INV_RAND_MAX; 
}

inline F32 random(F32 min, F32 max) { 
    return min + (max - min) * INV_RAND_MAX * rand(); 
}

inline I32 random(I32 max = RAND_MAX) { 
    return rand()%(max+1); 
}

inline bool bitCompare(U32 bitMask, U32 bit) {
    return ((bitMask & bit) == bit);
}

template <typename T>
inline T squared(T n){
    return n*n;
}

/// Clamps value n between min and max
template <typename T>
inline void CLAMP(T& n, T min, T max){
    n = std::min(std::max(n, min), max);
}

// bit manipulation
#define ToBit(posn)      (1 << posn)
#define BitSet(arg,posn) (arg |=  1 << posn)
#define BitClr(arg,posn) (arg &= ~(1 << (posn)))
#define BitTst(arg,posn) ((arg &   1 << (posn)) != 0)

#define BitDiff(arg1,arg2) (arg1 ^ arg2)
#define BitCmp(arg1,arg2,posn) ((arg1<<posn) == (arg2<<posn))

// bitmask manipulation
#define BitMaskSet(arg,mask) ((arg) |= (mask))
#define BitMaskClear(arg,mask) ((arg) &= (~(mask)))
#define BitMaskFlip(arg,mask) ((arg) ^= (mask))
#define BitMaskCheck(arg,mask) ((arg) & (mask))

template <typename T>
class mat4;
template <typename T>
class vec3;
template <typename T>
class Quaternion;

namespace Util {
    inline void replaceStringInPlace(std::string& subject, const std::string& search, const std::string& replace) {
        size_t pos = 0;
        while((pos = subject.find(search, pos)) != std::string::npos) {
             subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
    }

    inline void swap(char* first, char* second){
        char temp = *first;
        *first  = *second;
        *second = temp;
    }

    inline void permute(char* input, U32 startingIndex, U32 stringLength, vectorImpl<std::string>& container){
        if(startingIndex == stringLength -1){
            container.push_back(input);
        }else{
            for(U32 i = startingIndex; i < stringLength; i++){
                swap(&input[startingIndex], &input[i]);
                permute(input,startingIndex+1,stringLength,container);
                swap(&input[startingIndex], &input[i]);
            }
        }
    }

    inline vectorImpl<std::string> getPermutations(const std::string& inputString){
        vectorImpl<std::string> permutationContainer;
        permute((char*)inputString.c_str(), 0, (U32)inputString.length()-1, permutationContainer);
        return permutationContainer;
    }

    static std::stringstream _ssBuffer;

    inline bool isNumber(const std::string& s){
        _ssBuffer.str(std::string());
        _ssBuffer << s;
        F32 number;
        _ssBuffer >> number;
        if (_ssBuffer.good()) return false;
        else if(number == 0 && s[0] != 0) return false;
        return true;
    }

    template<typename T>
    inline std::string toString(const T& data){
        _ssBuffer.str(std::string());
        _ssBuffer << data;
        return _ssBuffer.str();
    }

    //U = to data type, T = from data type
    template<typename U, typename T>
    inline U convertData(const T& data){
        std::istringstream  iStream(data);
        U floatValue;
        iStream >> floatValue;
        return floatValue;
    }

    inline F32 xfov_to_yfov(F32 xfov, F32 aspect) {
        return DEGREES(2.0f * atan(tan(RADIANS(xfov) * 0.5f) / aspect));
    }

    inline F32 yfov_to_xfov(F32 yfov, F32 aspect) {
        return DEGREES(2.0f * atan(tan(RADIANS(yfov) * 0.5f) * aspect));
    }

    /** Ogre3D
    @brief Normalise the selected rotations to be within the +/-180 degree range.
    @details The normalise uses a wrap around, so for example a yaw of 360 degrees becomes 0 degrees, and -190 degrees becomes 170.
    @param normYaw If false, the yaw isn't normalized.
    @param normPitch If false, the pitch isn't normalized.
    @param normRoll If false, the roll isn't normalized.
    */
    void normalize(vec3<F32>& inputRotation, bool degrees = false, bool normYaw = true, bool normPitch = true, bool normRoll = true);

    namespace Mat4 {
        // ----------------------------------------------------------------------------------------
        template<typename T>
        void decompose (const mat4<T>& matrix, vec3<T>& scale, Quaternion<T>& rotation, vec3<T>& position);
        template<typename T>
        void decomposeNoScaling(const mat4<T>& matrix, Quaternion<T>& rotation,	vec3<T>& position);

        inline F32* Multiply(const F32 *a, const F32 *b, F32 *r = nullptr){
            static F32 temp[16];
            r = temp;
            U32 row, column, row_offset;
            for (row = 0, row_offset = row * 4; row < 4; ++row, row_offset = row * 4){
                for (column = 0; column < 4; ++column){
                    r[row_offset + column] =
                        (a[row_offset + 0] * b[column + 0]) +
                        (a[row_offset + 1] * b[column + 4]) +
                        (a[row_offset + 2] * b[column + 8]) +
                        (a[row_offset + 3] * b[column + 12]);
                }
            }
            return r;
        }
        __forceinline F32 det(const F32* mat) {
            return ((mat[0] * mat[5] * mat[10]) +
                    (mat[4] * mat[9] * mat[2]) +
                    (mat[8] * mat[1] * mat[6]) -
                    (mat[8] * mat[5] * mat[2]) -
                    (mat[4] * mat[1] * mat[10]) -
                    (mat[0] * mat[9] * mat[6]));
        }

        // Copyright 2011 The Closure Library Authors. All Rights Reserved.
        __forceinline void Inverse(const F32* in, F32* out){
            F32 m00 = in[ 0], m10 = in[ 1], m20 = in[ 2], m30 = in[ 3];
            F32 m01 = in[ 4], m11 = in[ 5], m21 = in[ 6], m31 = in[ 7];
            F32 m02 = in[ 8], m12 = in[ 9], m22 = in[10], m32 = in[11];
            F32 m03 = in[12], m13 = in[13], m23 = in[14], m33 = in[15];

            F32 a0 = m00 * m11 - m10 * m01;
            F32 a1 = m00 * m21 - m20 * m01;
            F32 a2 = m00 * m31 - m30 * m01;
            F32 a3 = m10 * m21 - m20 * m11;
            F32 a4 = m10 * m31 - m30 * m11;
            F32 a5 = m20 * m31 - m30 * m21;
            F32 b0 = m02 * m13 - m12 * m03;
            F32 b1 = m02 * m23 - m22 * m03;
            F32 b2 = m02 * m33 - m32 * m03;
            F32 b3 = m12 * m23 - m22 * m13;
            F32 b4 = m12 * m33 - m32 * m13;
            F32 b5 = m22 * m33 - m32 * m23;

            F32 det = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
            assert(!IS_ZERO(det));
                
            F32 idet = 1.0f / det;
            out[ 0] = ( m11 * b5 - m21 * b4 + m31 * b3) * idet;
            out[ 1] = (-m10 * b5 + m20 * b4 - m30 * b3) * idet;
            out[ 2] = ( m13 * a5 - m23 * a4 + m33 * a3) * idet;
            out[ 3] = (-m12 * a5 + m22 * a4 - m32 * a3) * idet;
            out[ 4] = (-m01 * b5 + m21 * b2 - m31 * b1) * idet;
            out[ 5] = ( m00 * b5 - m20 * b2 + m30 * b1) * idet;
            out[ 6] = (-m03 * a5 + m23 * a2 - m33 * a1) * idet;
            out[ 7] = ( m02 * a5 - m22 * a2 + m32 * a1) * idet;
            out[ 8] = ( m01 * b4 - m11 * b2 + m31 * b0) * idet;
            out[ 9] = (-m00 * b4 + m10 * b2 - m30 * b0) * idet;
            out[10] = ( m03 * a4 - m13 * a2 + m33 * a0) * idet;
            out[11] = (-m02 * a4 + m12 * a2 - m32 * a0) * idet;
            out[12] = (-m01 * b3 + m11 * b1 - m21 * b0) * idet;
            out[13] = ( m00 * b3 - m10 * b1 + m20 * b0) * idet;
            out[14] = (-m03 * a3 + m13 * a1 - m23 * a0) * idet;
            out[15] = ( m02 * a3 - m12 * a1 + m22 * a0) * idet;
        }
    };
};

#endif
