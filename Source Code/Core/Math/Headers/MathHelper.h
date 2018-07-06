/*
   Copyright (c) 2015 DIVIDE-Studio
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

/*Code references:
    Matrix inverse: http://www.devmaster.net/forums/showthread.php?t=14569
    Matrix multiply:
   http://fhtr.blogspot.com/2010/02/4x4-float-matrix-multiplication-using.html
    Square root: http://www.codeproject.com/KB/cpp/Sqrt_Prec_VS_Speed.aspx
*/

#ifndef _CORE_MATH_MATH_HELPER_H_
#define _CORE_MATH_MATH_HELPER_H_

#include "Platform/DataTypes/Headers/PlatformDefines.h"
#include <sstream>
#include <cctype>
#include <algorithm>

namespace Divide {

#undef M_PI
#define M_PI 3.141592653589793238462643383279f          //  PI
#define M_PIDIV2 1.570796326794896619231321691639f      //  PI / 2
#define M_2PI 6.283185307179586476925286766559f         //  2 * PI
#define M_PI2 9.869604401089358618834490999876f         //  PI ^ 2
#define M_PIDIV180 0.01745329251994329576923690768488f  //  PI / 180
#define M_180DIVPI 57.295779513082320876798154814105f   //  180 / PI
#define M_PIDIV360 \
    0.00872664625997164788461845384244f  //  PI / 180 / 2 - PI / 360

const F32 INV_RAND_MAX = 1.0f / (RAND_MAX + 1);

template <typename T>
T random(T max = RAND_MAX);

template <>
I32 random(I32 max);

template <typename T>
T random(T min, T max);

bool bitCompare(U32 bitMask, U32 bit);

/// Clamps value n between min and max
template <typename T>
void CLAMP(T& n, T min, T max);

template <typename T>
T CLAMPED(const T& n, T min, T max);

// Helper method to emulate GLSL
F32 fract(F32 floatValue);

/// Packs a floating point value into the [0...255] range (thx sqrt[-1] of
/// opengl.org forums)
U8 PACK_FLOAT(F32 floatValue);

// Pack 3 values into 1 float
F32 PACK_FLOAT(U8 x, U8 y, U8 z);

// UnPack 3 values from 1 float
void UNPACK_FLOAT(F32 src, F32& r, F32& g, F32& b);

// bit manipulation
#define ToBit(posn) (1 << posn)
#define BitSet(arg, posn) (arg |= 1 << posn)
#define BitClr(arg, posn) (arg &= ~(1 << (posn)))
#define BitTst(arg, posn) ((arg & 1 << (posn)) != 0)

#define BitDiff(arg1, arg2) (arg1 ^ arg2)
#define BitCmp(arg1, arg2, posn) ((arg1 << posn) == (arg2 << posn))

// bitmask manipulation
#define BitMaskSet(arg, mask) ((arg) |= (mask))
#define BitMaskClear(arg, mask) ((arg) &= (~(mask)))
#define BitMaskFlip(arg, mask) ((arg) ^= (mask))
#define BitMaskCheck(arg, mask) ((arg) & (mask))

template <typename T>
class mat4;
template <typename T>
class vec3;
template <typename T>
class vec4;
template <typename T>
class Quaternion;

namespace Angle {
/// Return the radian equivalent of the given degree value
template <typename T>
T DegreesToRadians(T angleDegrees);
/// Return the degree equivalent of the given radian value
template <typename T>
T RadiansToDegrees(T angleRadians);
};

namespace Metric {
/// Base value * 1000000000000
template <typename T>
T Tera(T a);
/// Base value * 1000000000
template <typename T>
T Giga(T a);
/// Base value * 1000000
template <typename T>
T Mega(T a);
/// Base value * 1000
template <typename T>
T Kilo(T a);
/// Base value * 100
template <typename T>
T Hecto(T a);
/// Base value * 10
template <typename T>
T Deca(T a);
/// Base value
template <typename T>
T Base(T a);
/// Base value * 0.1
template <typename T>
T Deci(T a);
/// Base value * 0.01
template <typename T>
T Centi(T a);
/// Base value * 0.001
template <typename T>
T Milli(T a);
/// Base value * 0.000001
template <typename T>
T Micro(T a);
/// Base value * 0.000000001
template <typename T>
T Nano(T a);
/// Base value * 0.000000000001
template <typename T>
T Pico(T a);
};

namespace Time {
template <typename T>
T MicrosecondsToSeconds(T a);
template <typename T>
T MicrosecondsToMilliseconds(T a);
template <typename T>
T MillisecondsToSeconds(T a);
template <typename T>
T SecondsToMilliseconds(T a);
template <typename T>
U64 SecondsToMicroseconds(T a);
template <typename T>
U64 MillisecondsToMicroseconds(T a);
};  // namespace Time

namespace Util {

/// a la Boost
template <typename T>
void hash_combine(std::size_t& seed, const T& v);

void replaceStringInPlace(stringImpl& subject, const stringImpl& search,
                          const stringImpl& replace);

void getPermutations(const stringImpl& inputString,
                     vectorImpl<stringImpl>& permutationContainer);

bool isNumber(const stringImpl& s);

template <typename T>
stringImpl toString(T data);

template <>
stringImpl toString(U8 data);

template <>
stringImpl toString(U16 data);

template <>
stringImpl toString(I8 data);

template <>
stringImpl toString(I16 data);

// U = to data type, T = from data type
template <typename U, typename T>
U convertData(const T& data);

/// http://stackoverflow.com/questions/236129/split-a-string-in-c
vectorImpl<stringImpl> split(const stringImpl& input, char delimiter);
vectorImpl<stringImpl>& split(const stringImpl& input, char delimiter,
                              vectorImpl<stringImpl>& elems);
/// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
stringImpl& ltrim(stringImpl& s);

stringImpl& rtrim(stringImpl& s);

stringImpl& trim(stringImpl& s);

/** Ogre3D
@brief Normalise the selected rotations to be within the +/-180 degree range.
@details The normalise uses a wrap around,
@details so for example a yaw of 360 degrees becomes 0 degrees, and -190 degrees
becomes 170.
@param normYaw If false, the yaw isn't normalized.
@param normPitch If false, the pitch isn't normalized.
@param normRoll If false, the roll isn't normalized.
*/
void normalize(vec3<F32>& inputRotation, bool degrees = false,
               bool normYaw = true, bool normPitch = true,
               bool normRoll = true);

vec4<U8> toByteColor(const vec4<F32>& floatColor);
vec3<U8> toByteColor(const vec3<F32>& floatColor);
vec4<F32> toFloatColor(const vec4<U8>& byteColor);
vec3<F32> toFloatColor(const vec3<U8>& byteColor);

namespace Mat4 {
template <typename T>
T* multiply(const T* a, const T* b, T* r = nullptr);

template <typename T>
__forceinline T det(const T* mat);

// Copyright 2011 The Closure Library Authors. All Rights Reserved.
template <typename T>
__forceinline void inverse(const T* in, T* out);
};  // namespace Mat4
};  // namespace Util
};  // namespace Divide

#endif  //_CORE_MATH_MATH_HELPER_H_

#include "MathHelper.inl"
