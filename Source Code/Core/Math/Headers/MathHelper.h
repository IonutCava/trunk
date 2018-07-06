/*
   Copyright (c) 2013 DIVIDE-Studio
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

#define DegToRad(a)	(a)*=M_PIDIV180
#define RadToDeg(a)	(a)*=M_180DIVPI
#define RADIANS(a)	((a)*M_PIDIV180)
#define DEGREES(a)	((a)*M_180DIVPI)

#define kilometre(a)    a*1000
#define metre(a)		a*1
#define decimetre(a)    a*0.1f
#define centimetre(a)   a*0.01f
#define millimeter(a)   a*0.001f

#define getMsToSec(a) a*0.001f
#define getSecToMs(a) a*1000.0f

#define MsToSec(a)   (a)*=0.001f
#define SecToMs(a)   (a)*=1000.0f

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
        permute((char*)inputString.c_str(), 0, inputString.length()-1, permutationContainer);
        return permutationContainer;
    }

    inline bool isNumber(const std::string& s){
        std::stringstream ss;
        ss << s;
        F32 number;
        ss >> number;
        if(ss.good()) return false;
        else if(number == 0 && s[0] != 0) return false;
        return true;
    }

    template<class T>
    inline std::string toString(const T& data){
        std::stringstream s;
        s << data;
        return s.str();
    }

    //U = to data type, T = from data type
    template<class U, class T>
    inline U convertData(const T& data){
        std::istringstream  iStream(data);
        U floatValue;
        iStream >> floatValue;
        return floatValue;
    }

    inline F32 max(const F32& a, const F32& b){
        return (a<b) ? b : a;
    }

    inline F32 min(const F32& a, const F32& b){
        return (a<b) ? a : b;
    }

    inline F32 xfov_to_yfov(F32 xfov, F32 aspect) {
        return DEGREES(2.0f * atan(tan(RADIANS(xfov) * 0.5f) / aspect));
    }

    inline F32 yfov_to_xfov(F32 yfov, F32 aspect) {
        return DEGREES(2.0f * atan(tan(RADIANS(yfov) * 0.5f) * aspect));
    }
    namespace Mat4{
        // ----------------------------------------------------------------------------------------
        template<class Type>
        void decompose (const mat4<Type>& matrix, vec3<Type>& scale, Quaternion<Type>& rotation, vec3<Type>& position);
        template<class Type>
        void decomposeNoScaling(const mat4<Type>& matrix, Quaternion<Type>& rotation,	vec3<Type>& position);
    }
}

#endif
