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

#ifndef _CORE_MATH_MATH_HELPER_INL_
#define _CORE_MATH_MATH_HELPER_INL_

namespace Divide {

template <typename T>
T Random(T max) {
    return max * rand() * static_cast<T>(INV_RAND_MAX);
}

template <>
inline I32 Random(I32 max) {
    return rand() % (max + 1);
}

template <typename T>
inline T Random(T min, T max) {
    return min + (max - min) * 
           static_cast<T>(INV_RAND_MAX) * 
           static_cast<T>(rand());
}

/// Clamps value n between min and max
template <typename T>
inline void CLAMP(T& n, T min, T max) {
    n = std::min(std::max(n, min), max);
}

template <typename T>
inline T CLAMPED(const T& n, T min, T max) {
    return std::min(std::max(n, min), max);
}


template<typename T>
inline bool BitCompare(T bitMask, T bit) {
    return BitCompare(to_uint(bitMask), to_uint(bit));
}

template<>
inline bool BitCompare<U32>(U32 bitMask, U32 bit) {
    return ((bitMask & bit) == bit);
}

///(thx sqrt[-1] and canuckle of opengl.org forums)

// Helper method to emulate GLSL
inline F32 FRACT(F32 floatValue) {
    return to_float(fmod(floatValue, 1.0f));
}

//Helper method to go from a float to packed char
inline U8 FLOAT_TO_CHAR(F32 value) {
    value = (value + 1.0f) * 0.5f;
    return static_cast<U8>(value * 255.0f);
}

// Pack 3 values into 1 float
inline F32 PACK_FLOAT(U8 x, U8 y, U8 z) {
    static const D32 offset = to_double(1 << 24);

    U32 packedColor = (x << 16) | (y << 8) | z;
    return to_float(to_double(packedColor) / offset);
}

// UnPack 3 values from 1 float
inline void UNPACK_FLOAT(F32 src, F32& r, F32& g, F32& b) {
    r = FRACT(src);
    g = FRACT(src * 256.0f);
    b = FRACT(src * 65536.0f);

    // Unpack to the -1..1 range
    r = (r * 2.0f) - 1.0f;
    g = (g * 2.0f) - 1.0f;
    b = (b * 2.0f) - 1.0f;
}

namespace Angle {
template <typename T>
/*constexpr*/ T DegreesToRadians(T angleDegrees) {
    return static_cast<T>(angleDegrees * M_PIDIV180);
}

template <typename T>
/*constexpr*/ T RadiansToDegrees(T angleRadians) {
    return static_cast<T>(angleRadians * M_180DIVPI);
}

/// Returns the specified value. Used only for emphasis
template <typename T>
/*constexpr*/ T Degrees(T degrees) {
    return degrees;
}

/// Returns the specified value. Used only for emphasis
template <typename T>
/*constexpr*/ T Radians(T radians) {
    return radians;
}

};  // namespace Angle

namespace Metric {
template <typename T>
/*constexpr*/ T Tera(T a) {
    return static_cast<T>(a * 1000000000000.0);
}

template <typename T>
/*constexpr*/ T Giga(T a) {
    return static_cast<T>(a * 1000000000.0);
}

template <typename T>
/*constexpr*/ T Mega(T a) {
    return static_cast<T>(a * 1000000.0);
}

template <typename T>
/*constexpr*/ T Kilo(T a) {
    return static_cast<T>(a * 1000.0);
}

template <typename T>
/*constexpr*/ T Hecto(T a) {
    return static_cast<T>(a * 100.0);
}

template <typename T>
/*constexpr*/ T Deca(T a) {
    return static_cast<T>(a * 10.0);
}

template <typename T>
/*constexpr*/ T Base(T a) {
    return a;
}

template <typename T>
/*constexpr*/ T Deci(T a) {
    return static_cast<T>(a * 0.1);
}

template <typename T>
/*constexpr*/ T Centi(T a) {
    return static_cast<T>(a * 0.01);
}

template <typename T>
/*constexpr*/ T Milli(T a) {
    return static_cast<T>(a * 0.001);
}

template <typename T>
/*constexpr*/ T Micro(T a) {
    return static_cast<T>(a * 0.000001);
}

template <typename T>
/*constexpr*/ T Nano(T a) {
    return static_cast<T>(a * 0.000000001);
}

template <typename T>
/*constexpr*/ T Pico(T a) {
    return static_cast<T>(a * 0.000000000001);
}
};  // namespace Metric

namespace Time {
template <typename T>
T Seconds(T a) {
    return a;
}

template <typename T>
T Milliseconds(T a) {
    return a;
}

template <typename T>
U64 Microseconds(T a) {
    return static_cast<U64>(a);
}

template <typename T>
/*constexpr*/ T MicrosecondsToSeconds(U64 a) {
    return Metric::Micro(static_cast<T>(a));
}

template <typename T>
/*constexpr*/ T MicrosecondsToMilliseconds(U64 a) {
    return Metric::Milli(static_cast<T>(a));
}

template <typename T>
/*constexpr*/ T SecondsToMilliseconds(T a) {
    return Metric::Kilo(a);
}

template <typename T>
/*constexpr*/ U64 SecondsToMicroseconds(T a) {
    return static_cast<U64>(Metric::Mega(a));
}

template <typename T>
/*constexpr*/ U64 MillisecondsToMicroseconds(T a) {
    return static_cast<U64>(Metric::Kilo(a));
}

template <typename T>
/*constexpr*/ T MillisecondsToSeconds(T a) {
    return Metric::Milli(a);
}
};  // namespace Time

namespace Util {
/// a la Boost
template <typename T>
void Hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline void ReplaceStringInPlace(stringImpl& subject, const stringImpl& search,
                                 const stringImpl& replace) {
    stringAlg::stringSize pos = 0;
    while ((pos = subject.find(search, pos)) != stringImpl::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

// U = to data type, T = from data type
template <typename U, typename T>
U ConvertData(const T& data) {
    U targetValue;
    std::istringstream iss(data);
    iss >> targetValue;
    DIVIDE_ASSERT(!iss.fail(), "Util::convertData error : invalid conversion!");
    return targetValue;
}

/// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
inline stringImpl& Ltrim(stringImpl& s) {
    s.erase(std::begin(s),
            std::find_if(std::begin(s), std::end(s),
                         std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

inline stringImpl& Rtrim(stringImpl& s) {
    s.erase(
        std::find_if(std::rbegin(s), std::rend(s),
                     std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
        std::end(s));
    return s;
}

inline stringImpl& Trim(stringImpl& s) {
    return Ltrim(Rtrim(s));
}

namespace Mat4 {

template <typename T>
__forceinline T Det(const T* mat) {
    return ((mat[0] * mat[5] * mat[10]) + (mat[4] * mat[9] * mat[2]) +
            (mat[8] * mat[1] * mat[6]) - (mat[8] * mat[5] * mat[2]) -
            (mat[4] * mat[1] * mat[10]) - (mat[0] * mat[9] * mat[6]));
}

// Copyright 2011 The Closure Library Authors. All Rights Reserved.
template <typename T>
__forceinline void Inverse(const T* in, T* out) {
    T m00 = in[0], m10 = in[1], m20 = in[2], m30 = in[3];
    T m01 = in[4], m11 = in[5], m21 = in[6], m31 = in[7];
    T m02 = in[8], m12 = in[9], m22 = in[10], m32 = in[11];
    T m03 = in[12], m13 = in[13], m23 = in[14], m33 = in[15];

    T a0 = m00 * m11 - m10 * m01;
    T a1 = m00 * m21 - m20 * m01;
    T a2 = m00 * m31 - m30 * m01;
    T a3 = m10 * m21 - m20 * m11;
    T a4 = m10 * m31 - m30 * m11;
    T a5 = m20 * m31 - m30 * m21;
    T b0 = m02 * m13 - m12 * m03;
    T b1 = m02 * m23 - m22 * m03;
    T b2 = m02 * m33 - m32 * m03;
    T b3 = m12 * m23 - m22 * m13;
    T b4 = m12 * m33 - m32 * m13;
    T b5 = m22 * m33 - m32 * m23;

    T idet = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
    assert(!IS_ZERO(idet));

    idet = 1 / idet;

    out[0] = (m11 * b5 - m21 * b4 + m31 * b3) * idet;
    out[1] = (-m10 * b5 + m20 * b4 - m30 * b3) * idet;
    out[2] = (m13 * a5 - m23 * a4 + m33 * a3) * idet;
    out[3] = (-m12 * a5 + m22 * a4 - m32 * a3) * idet;
    out[4] = (-m01 * b5 + m21 * b2 - m31 * b1) * idet;
    out[5] = (m00 * b5 - m20 * b2 + m30 * b1) * idet;
    out[6] = (-m03 * a5 + m23 * a2 - m33 * a1) * idet;
    out[7] = (m02 * a5 - m22 * a2 + m32 * a1) * idet;
    out[8] = (m01 * b4 - m11 * b2 + m31 * b0) * idet;
    out[9] = (-m00 * b4 + m10 * b2 - m30 * b0) * idet;
    out[10] = (m03 * a4 - m13 * a2 + m33 * a0) * idet;
    out[11] = (-m02 * a4 + m12 * a2 - m32 * a0) * idet;
    out[12] = (-m01 * b3 + m11 * b1 - m21 * b0) * idet;
    out[13] = (m00 * b3 - m10 * b1 + m20 * b0) * idet;
    out[14] = (-m03 * a3 + m13 * a1 - m23 * a0) * idet;
    out[15] = (m02 * a3 - m12 * a1 + m22 * a0) * idet;
}

template <typename T>
__forceinline void Add(const T a[16], const T b[16], T r[16]) {
    T rTemp[] = {
        a[0]  + b[0],  a[1]  + b[1],  a[2]  + b[2],  a[3]  + b[3],
        a[4]  + b[4],  a[5]  + b[5],  a[6]  + b[6],  a[7]  + b[7],
        a[8]  + b[8],  a[9]  + b[9],  a[10] + b[10], a[11] + b[11],
        a[12] + b[12], a[13] + b[13], a[14] + b[14], a[15] + b[15]};

    memcpy(r, rTemp, 16 * sizeof(T));
}

template <typename T>
__forceinline void Substract(const T a[16], const T b[16], T r[16]) {
    T rTemp[] = {
        a[0]  - b[0],  a[1]  - b[1],  a[2]  - b[2],  a[3]  - b[3],
        a[4]  - b[4],  a[5]  - b[5],  a[6]  - b[6],  a[7]  - b[7],
        a[8]  - b[8],  a[9]  - b[9],  a[10] - b[10], a[11] - b[11],
        a[12] - b[12], a[13] - b[13], a[14] - b[14], a[15] - b[15]};

    memcpy(r, rTemp, 16 * sizeof(T));
}

template <typename T>
__forceinline void Multiply(const T a[16], const T b[16], T r[16]) {
    T rTemp[] = 
        {(a[0]  * b[0]) + (a[1]  * b[4]) + (a[2]  * b[8] ) + (a[3]  * b[12]),
         (a[0]  * b[1]) + (a[1]  * b[5]) + (a[2]  * b[9] ) + (a[3]  * b[13]),
         (a[0]  * b[2]) + (a[1]  * b[6]) + (a[2]  * b[10]) + (a[3]  * b[14]),
         (a[0]  * b[3]) + (a[1]  * b[7]) + (a[2]  * b[11]) + (a[3]  * b[15]),
         (a[4]  * b[0]) + (a[5]  * b[4]) + (a[6]  * b[8] ) + (a[7]  * b[12]),
         (a[4]  * b[1]) + (a[5]  * b[5]) + (a[6]  * b[9] ) + (a[7]  * b[13]),
         (a[4]  * b[2]) + (a[5]  * b[6]) + (a[6]  * b[10]) + (a[7]  * b[14]),
         (a[4]  * b[3]) + (a[5]  * b[7]) + (a[6]  * b[11]) + (a[7]  * b[15]),
         (a[8]  * b[0]) + (a[9]  * b[4]) + (a[10] * b[8] ) + (a[11] * b[12]),
         (a[8]  * b[1]) + (a[9]  * b[5]) + (a[10] * b[9] ) + (a[11] * b[13]),
         (a[8]  * b[2]) + (a[9]  * b[6]) + (a[10] * b[10]) + (a[11] * b[14]),
         (a[8]  * b[3]) + (a[9]  * b[7]) + (a[10] * b[11]) + (a[11] * b[15]),
         (a[12] * b[0]) + (a[13] * b[4]) + (a[14] * b[8] ) + (a[15] * b[12]),
         (a[12] * b[1]) + (a[13] * b[5]) + (a[14] * b[9] ) + (a[15] * b[13]),
         (a[12] * b[2]) + (a[13] * b[6]) + (a[14] * b[10]) + (a[15] * b[14]),
         (a[12] * b[3]) + (a[13] * b[7]) + (a[14] * b[11]) + (a[15] * b[15])};

    memcpy(r, rTemp, 16 * sizeof(T));
}

template <typename T>
__forceinline void MultiplyScalar(const T a[16], T b, T r[16]){
    T rTemp[] = { (a[0]  * b), (a[1]  * b), (a[2]  * b), (a[3]  * b),
                  (a[4]  * b), (a[5]  * b), (a[6]  * b), (a[7]  * b),
                  (a[8]  * b), (a[9]  * b), (a[10] * b), (a[11] * b),
                  (a[12] * b), (a[13] * b), (a[14] * b), (a[15] * b) };

   memcpy(r, rTemp, 16 * sizeof(T));
}

};  // namespace Mat4
};  // namespace Util
};  // namespace Divide

#endif  //_CORE_MATH_MATH_HELPER_INL_