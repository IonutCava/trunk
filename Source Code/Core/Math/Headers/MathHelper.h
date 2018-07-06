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

template<class T>
inline void DegToRad(T& a)	{ a*=M_PIDIV180; }
template<class T>
inline void RadToDeg(T& a)	{ a*=M_180DIVPI; }
template<class T>
inline F32 RADIANS(T a)	    { return a*M_PIDIV180;}
template<class T>
inline F32 DEGREES(T a)	    { return a*M_180DIVPI; }
template<class T>
inline F32 kilometre(T a)   { return a*1000.0f; }
template<class T>
inline F32 metre(T a)		{ return a*1.0f; }
template<class T>
inline F32 decimetre(T a)   { return a*0.1f; }
template<class T>
inline F32 centimetre(T a)  { return a*0.01f; }
template<class T>
inline F32 millimeter(T a)  { return a*0.001f; }

template<class T>
inline D32 getUsToSec(T a) {return a * 0.000001; }

template<class T>
inline D32 getUsToMs(T a)  {return getMsToSec(a); }

template<class T>
inline D32 getMsToSec(T a) { return a*0.001; }

template<class T>
inline D32 getSecToMs(T a) { return a*1000.0; }

template<class T>
inline D32 getSecToUs(T a) { return a * 1000000.0;}

template<class T>
inline D32 getMsToUs(T a)  { return getSecToMs(a);}

template<class T>
inline void MsToSec(T& a)  { a*=0.001; }

template<class T>
inline void SecToMs(T& a)  { a*=1000.0; }

template<class T>
inline void UsToSec(T& a)  { a*=0.000001; }

template<class T>
inline void SecToUs(T& a)  { a*=1000000.0; }

template<class T>
inline void UsToMs(T& a)   { MsToSec(a); }

template<class T>
inline void MsToUs(T& a)   { SecToMs(a); }


const  F32 INV_RAND_MAX = 1.0 / (RAND_MAX + 1);

inline F32 random(F32 max=1.0)      { return max * rand() * INV_RAND_MAX; }
inline F32 random(F32 min, F32 max) { return min + (max - min) * INV_RAND_MAX * rand(); }
inline I32 random(I32 max=RAND_MAX) { return rand()%(max+1); }

inline bool bitCompare(U32 bitMask, U32 bit) {return ((bitMask & bit) == bit);}

template <class T>
inline T squared(T n){
    return n*n;
}
/// Clamps value n between min and max
template <class T>
inline void CLAMP(T& n, T min, T max){
    n = ((n)<(min))?(min):(((n)>(max))?(max):(n));
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

inline D32 square_root(D32 n){
    return sqrt(n);
}

template<class T>
inline T square_root_tpl(T n){
    return (T)square_root((D32)n);
}

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

    template<class T>
    inline std::string toString(const T& data){
        _ssBuffer.str(std::string());
        _ssBuffer << data;
        return _ssBuffer.str();
    }

    //U = to data type, T = from data type
    template<class U, class T>
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
        template<class T>
        void decompose (const mat4<T>& matrix, vec3<T>& scale, Quaternion<T>& rotation, vec3<T>& position);
        template<class T>
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

        __forceinline void Inverse(const F32* in, F32* out){
            F32 idet = 1.0f / det(in);
            out[0] = (in[5] * in[10] - in[9] * in[6]) * idet;
            out[1] = -(in[1] * in[10] - in[9] * in[2]) * idet;
            out[2] = (in[1] * in[6] - in[5] * in[2]) * idet;
            out[3] = 0.0f;
            out[4] = -(in[4] * in[10] - in[8] * in[6]) * idet;
            out[5] = (in[0] * in[10] - in[8] * in[2]) * idet;
            out[6] = -(in[0] * in[6] - in[4] * in[2]) * idet;
            out[7] = 0.0f;
            out[8] = (in[4] * in[9] - in[8] * in[5]) * idet;
            out[9] = -(in[0] * in[9] - in[8] * in[1]) * idet;
            out[10] = (in[0] * in[5] - in[4] * in[1]) * idet;
            out[11] = 0.0f;
            out[12] = -(in[12] * (out)[0] + in[13] * (out)[4] + in[14] * (out)[8]);
            out[13] = -(in[12] * (out)[1] + in[13] * (out)[5] + in[14] * (out)[9]);
            out[14] = -(in[12] * (out)[2] + in[13] * (out)[6] + in[14] * (out)[10]);
            out[15] = 1.0f;
        }
    };
};

#endif
