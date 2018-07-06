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

/*Code references:
    Matrix inverse: http://www.devmaster.net/forums/showthread.php?t=14569
    Matrix multiply:
   http://fhtr.blogspot.com/2010/02/4x4-float-matrix-multiplication-using.html
    Square root: http://www.codeproject.com/KB/cpp/Sqrt_Prec_VS_Speed.aspx
*/

#ifndef _CORE_MATH_MATH_HELPER_H_
#define _CORE_MATH_MATH_HELPER_H_

#include "Platform/Headers/PlatformDefines.h"

#include <sstream>
#include <cctype>
#include <cstring>
#include <assert.h>
#include <random>

namespace Divide {

#if !defined(M_PI)
    constexpr D64 M_PI = 3.14159265358979323846;
#endif

    constexpr D64 M_2PI = 6.28318530717958647692;
    constexpr D64 M_PIDIV180 = 0.01745329251994329576;
    constexpr D64 M_180DIVPI = 57.29577951308232087679;
    constexpr D64 M_PIDIV360 = 0.00872664625997164788;
    constexpr F32 INV_RAND_MAX = 0.0000305185094f;
    constexpr D64 M_PI2 = M_2PI;

template<typename T>
using SignedIntegerBasedOnSize = typename std::conditional<sizeof(T) == 8, I64, I32>::type;
template<typename T>
using UnsignedIntegerBasedOnSize = typename std::conditional<sizeof(T) == 8, U64, U32>::type;
template<typename T>
using IntegerTypeBasedOnSign = typename std::conditional<std::is_unsigned<T>::value,
                                                         UnsignedIntegerBasedOnSize<T>,
                                                         SignedIntegerBasedOnSize<T>>::type;

template<typename T>
using DefaultDistribution = typename std::conditional<std::is_integral<T>::value, 
                                     std::uniform_int_distribution<IntegerTypeBasedOnSign<T>>,
                                     std::uniform_real_distribution<T>>::type;

template <typename T, 
          typename Engine = std::mt19937_64,
          typename Distribution = DefaultDistribution<T>>
T Random(T min, T max);

template <typename T,
          typename Engine = std::mt19937_64,
          typename Distribution = DefaultDistribution<T>>
T Random(T max);

template <typename T,
          typename Engine = std::mt19937_64,
          typename Distribution = DefaultDistribution<T>>
T Random();

template<typename Engine = std::mt19937_64>
void SeedRandom();

template<typename Engine = std::mt19937_64>
void SeedRandom(U32 seed);

template<typename T>
bool BitCompare(const T bitMask, const T bit);

/// Clamps value n between min and max
template <typename T>
void CLAMP(T& n, const T min, const T max);

template <typename T>
T CLAMPED(const T& n, const T min, const T max);

U32 nextPOW2(U32 n);

// Calculate the smalles NxN matrix that can hold the specified
// number of elements. Returns N
U32 minSquareMatrixSize(U32 elementCount);

template <typename T, typename U>
T Lerp(const T v1, const T v2, const U t);

///Helper methods to go from a float to packed char and back
U8 FLOAT_TO_CHAR(const F32 value);
U8 FLOAT_TO_CHAR_SNORM(const F32 value);
F32 CHAR_TO_FLOAT(const U8 value);
F32 CHAR_TO_FLOAT_SNORM(const U8 value);

/// Helper method to emulate GLSL
F32 FRACT(const F32 floatValue);

// Pack 3 values into 1 float
F32 PACK_FLOAT(const U8 x, const U8 y, const U8 z);

// UnPack 3 values from 1 float
void UNPACK_FLOAT(const F32 src, F32& r, F32& g, F32& b);

// bit manipulation
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
class mat2;
template <typename T>
class mat3;
template <typename T>
class mat4;
template <typename T>
class vec2;
template <typename T>
class vec3;
template <typename T>
class vec4;
template <typename T>
class Quaternion;

namespace Angle {
/// Return the radian equivalent of the given degree value
template <typename T>
constexpr T DegreesToRadians(const T angleDegrees);
/// Return the degree equivalent of the given radian value
template <typename T>
constexpr T RadiansToDegrees(const T angleRadians);
/// Returns the specified value. Used only for emphasis
template <typename T>
constexpr T Degrees(const T degrees);
/// Returns the specified value. Used only for emphasis
template <typename T>
constexpr T Radians(const T radians);
};

namespace Metric {
/// Base value * 1000000000000
template <typename T>
constexpr T Tera(const T a);
/// Base value * 1000000000
template <typename T>
constexpr T Giga(const T a);
/// Base value * 1000000
template <typename T>
constexpr T Mega(const T a);
/// Base value * 1000
template <typename T>
constexpr T Kilo(const T a);
/// Base value * 100
template <typename T>
constexpr T Hecto(const T a);
/// Base value * 10
template <typename T>
constexpr T Deca(const T a);
/// Base value
template <typename T>
constexpr T Base(const T a);
/// Base value * 0.1
template <typename T>
constexpr T Deci(const T a);
/// Base value * 0.01
template <typename T>
constexpr T Centi(const T a);
/// Base value * 0.001
template <typename T>
constexpr T Milli(const T a);
/// Base value * 0.000001
template <typename T>
constexpr T Micro(const T a);
/// Base value * 0.000000001
template <typename T>
constexpr T Nano(const T a);
/// Base value * 0.000000000001
template <typename T>
constexpr T Pico(const T a);
};

namespace Time {
/// Return the passed param without any modification
/// Used only for emphasis
template <typename T>
T Seconds(const T a);
template <typename T>
T Milliseconds(const T a);
template <typename T>
U64 Microseconds(const T a);

template <typename T>
T MicrosecondsToSeconds(const U64 a);
template <typename T>
T MicrosecondsToMilliseconds(const U64 a);
template <typename T>
T MillisecondsToSeconds(const T a);
template <typename T>
T SecondsToMilliseconds(const T a);
template <typename T>
U64 SecondsToMicroseconds(const T a);
template <typename T>
U64 MillisecondsToMicroseconds(const T a);
};  // namespace Time

namespace Util {

struct GraphPlot2D;
struct GraphPlot3D;
struct GlobalFloatEvent {
    explicit GlobalFloatEvent(const char* name,
                              const F32 eventValue,
                              const U64 timeStamp)
        : _eventName(name),
          _eventValue(eventValue),
          _timeStamp(timeStamp)
    {
    }

    stringImpl _eventName;
    F32 _eventValue;
    U64 _timeStamp;
};

void FlushFloatEvents();

void RecordFloatEvent(const char* eventName, F32 eventValue, U64 timestamp);

const vectorImpl<GlobalFloatEvent>& GetFloatEvents();

void PlotFloatEvents(const stringImpl& eventName,
                     vectorImpl<GlobalFloatEvent> eventsCopy,
                     GraphPlot2D& targetGraph);

/// a la Boost
template <typename T>
void Hash_combine(size_t& seed, const T& v);

void ReplaceStringInPlace(stringImpl& subject, const stringImpl& search,
                          const stringImpl& replace);

void GetPermutations(const stringImpl& inputString,
                     vectorImpl<stringImpl>& permutationContainer);

bool IsNumber(const stringImpl& s);

stringImpl GetTrailingCharacters(const stringImpl& input, size_t count);

template<class FwdIt, class Compare = std::less<typename std::iterator_traits<FwdIt>::value_type>>
void insertion_sort(FwdIt first, FwdIt last, Compare cmp = Compare());

void ReadTextFile(const stringImpl& filePath, stringImpl& contentOut);
stringImpl ReadTextFile(const stringImpl& filePath);
void WriteTextFile(const stringImpl& filePath, const stringImpl& content);

bool CompareIgnoreCase(const stringImpl& a, const stringImpl&b);

/// will add '.' automatically at the start of 'extension'
bool HasExtension(const stringImpl& filePath, const stringImpl& extension);

// U = to data type, T = from data type
template <typename U, typename T>
U ConvertData(const T& data);

/// http://stackoverflow.com/questions/236129/split-a-string-in-c
vectorImpl<stringImpl> Split(const stringImpl& input, char delimiter);
vectorImpl<stringImpl>& Split(const stringImpl& input, char delimiter,
                              vectorImpl<stringImpl>& elems);
/// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
stringImpl& Ltrim(stringImpl& s);
stringImpl Ltrim(const stringImpl& s);

stringImpl& Rtrim(stringImpl& s);
stringImpl Rtrim(const stringImpl& s);

stringImpl& Trim(stringImpl& s);
stringImpl  Trim(const stringImpl& s);
//format is passed by value to conform with the requirements of va_start.
//ref: http://codereview.stackexchange.com/questions/115760/use-va-list-to-format-a-string
stringImpl StringFormat(const char *const format, ...);

std::pair<stringImpl/*fileName*/, stringImpl/*filePath*/> SplitPathToNameAndLocation(const stringImpl& input);

void CStringRemoveChar(char* str, char charToRemove);
bool FileExists(const char* filePath);
/** Ogre3D
@brief Normalise the selected rotations to be within the +/-180 degree range.
@details The normalise uses a wrap around,
@details so for example a yaw of 360 degrees becomes 0 degrees, and -190 degrees
becomes 170.
@param normYaw If false, the yaw isn't normalized.
@param normPitch If false, the pitch isn't normalized.
@param normRoll If false, the roll isn't normalized.
*/
void Normalize(vec3<F32>& inputRotation, bool degrees = false,
               bool normYaw = true, bool normPitch = true,
               bool normRoll = true);

vec4<U8>  ToByteColour(const vec4<F32>& floatColour);
vec3<U8>  ToByteColour(const vec3<F32>& floatColour);
vec4<I32> ToIntColour(const vec4<F32>& floatColour);
vec3<I32> ToIntColour(const vec3<F32>& floatColour);
vec4<U32> ToUIntColour(const vec4<F32>& floatColour);
vec3<U32> ToUIntColour(const vec3<F32>& floatColour);
vec4<F32> ToFloatColour(const vec4<U8>& byteColour);
vec3<F32> ToFloatColour(const vec3<U8>& byteColour);
vec4<F32> ToFloatColour(const vec4<U32>& uintColour);
vec3<F32> ToFloatColour(const vec3<U32>& uintColour);

void ToByteColour(const vec4<F32>& floatColour, vec4<U8>& colourOut);
void ToByteColour(const vec3<F32>& floatColour, vec3<U8>& colourOut);
void ToIntColour(const vec4<F32>& floatColour, vec4<I32>& colourOut);
void ToIntColour(const vec3<F32>& floatColour, vec3<I32>& colourOut);
void ToUIntColour(const vec4<F32>& floatColour, vec4<U32>& colourOut);
void ToUIntColour(const vec3<F32>& floatColour, vec3<U32>& colourOut);
void ToFloatColour(const vec4<U8>& byteColour, vec4<F32>& colourOut);
void ToFloatColour(const vec3<U8>& byteColour, vec3<F32>& colourOut);
void ToFloatColour(const vec4<U32>& uintColour, vec4<F32>& colourOut);
void ToFloatColour(const vec3<U32>& uintColour, vec3<F32>& colourOut);


inline F32 PACK_VEC3_SNORM(const F32 x, const F32 y, const F32 z) {
    return PACK_FLOAT(FLOAT_TO_CHAR_SNORM(x),
                      FLOAT_TO_CHAR_SNORM(y),
                      FLOAT_TO_CHAR_SNORM(z));
}

inline F32 PACK_VEC3(const F32 x, const F32 y, const F32 z) {
    return PACK_FLOAT(FLOAT_TO_CHAR(x),
                      FLOAT_TO_CHAR(y),
                      FLOAT_TO_CHAR(z));
}

F32 PACK_VEC3(const vec3<F32>& value);

inline void UNPACK_VEC3(const F32 src, F32& x, F32& y, F32& z) {
    UNPACK_FLOAT(src, x, y, z);
}
};  // namespace Util
};  // namespace Divide

namespace std {
    template<typename T, size_t N>
    struct hash<array<T, N> >
    {
        typedef array<T, N> argument_type;
        typedef size_t result_type;

        result_type operator()(const argument_type& a) const
        {
            result_type h = 0;
            for (const T& elem : a)
            {
                Divide::Util::Hash_combine(h, elem);
            }
            return h;
        }
    };
};

#endif  //_CORE_MATH_MATH_HELPER_H_

#include "MathHelper.inl"
