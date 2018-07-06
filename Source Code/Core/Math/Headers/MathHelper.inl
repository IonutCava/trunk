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

#ifndef _CORE_MATH_MATH_HELPER_INL_
#define _CORE_MATH_MATH_HELPER_INL_

namespace Divide {

template<typename T>
T random(T max) { 
    return max * rand() * static_cast<T>(INV_RAND_MAX);
}

template<>
inline I32 random(I32 max) { 
    return rand() % (max + 1); 
}

template<typename T>
inline T random(T min, T max) {
    return min + (max - min) * static_cast<T>(INV_RAND_MAX) * rand(); 
}

/// Clamps value n between min and max
template<typename T>
void CLAMP(T& n, T min, T max) {
    n = std::min(std::max(n, min), max);
}

template<typename T>
T CLAMPED(const T& n, T min, T max){
    return std::min(std::max(n, min), max);
}

inline bool bitCompare(U32 bitMask, U32 bit) {
    return ((bitMask & bit) == bit);
}

//Helper method to emulate GLSL
inline F32 fract(F32 floatValue) {  
    return static_cast<F32>(fmod(floatValue, 1.0f));
}

///Packs a floating point value into the [0...255] range (thx sqrt[-1] of opengl.org forums)
inline U8 PACK_FLOAT(F32 floatValue) {
    //Scale and bias
    floatValue = (floatValue + 1.0f) * 0.5f;
    return static_cast<U8>(floatValue * 255.0f);
}

//Pack 3 values into 1 float
inline F32 PACK_FLOAT(U8 x, U8 y, U8 z) {
    U32 packedColor = (x << 16) | (y << 8) | z;

    return static_cast<F32>( ((D32)packedColor) / ((D32) (1 << 24)) );
}

//UnPack 3 values from 1 float
inline void UNPACK_FLOAT(F32 src, F32& r, F32& g, F32& b) {
    r = fract(src);
    g = fract(src * 256.0f);
    b = fract(src * 65536.0f);

    //Unpack to the -1..1 range
    r = (r * 2.0f) - 1.0f;
    g = (g * 2.0f) - 1.0f;
    b = (b * 2.0f) - 1.0f;
}

namespace Angle {
    template<typename T>
    T DegreesToRadians(T angleDegrees) {
        return static_cast<T>(angleDegrees * M_PIDIV180);
    }

    template<typename T>
    T RadiansToDegrees(T angleRadians) {
        return static_cast<T>(angleRadians * M_180DIVPI);
    }
}; //namespace Angle

namespace Metric {
    template<typename T>
    T Tera(T a) { 
        return static_cast<T>(a * 1000000000000.0); 
    }

    template<typename T>
    T Giga(T a) { 
        return static_cast<T>(a * 1000000000.0); 
    }

    template<typename T>
    T Mega(T a) { 
        return static_cast<T>(a * 1000000.0); 
    }

    template<typename T>
    T Kilo(T a) { 
        return static_cast<T>(a * 1000.0); 
    }

    template<typename T>
    T Hecto(T a) { 
        return static_cast<T>(a * 100.0); 
    }

    template<typename T>
    T Deca(T a) { 
        return static_cast<T>(a * 10.0); 
    }

    template<typename T>
    T Base(T a) {
        return a;
    }

    template<typename T>
    T Deci(T a) { 
        return static_cast<T>(a * 0.1); 
    }

    template<typename T>
    T Centi(T a) { 
        return static_cast<T>(a * 0.01); 
    }

    template<typename T>
    T Milli(T a) { 
        return static_cast<T>(a * 0.001); 
    }

    template<typename T>
    T Micro(T a) { 
        return static_cast<T>(a * 0.000001); 
    }

    template<typename T>
    T Nano(T a) { 
        return static_cast<T>(a * 0.000000001); 
    }

    template<typename T>
    T Pico(T a) { 
        return static_cast<T>(a * 0.000000000001); 
    }
}; //namespace Metric

namespace Time {
    template<typename T>
    T MicrosecondsToSeconds(T a) { 
        return Metric::Pico(a); 
    }

    template<typename T>
    T MicrosecondsToMilliseconds(T a)  { 
        return MicrosecondsToSeconds(a); 
    }

    template<typename T>
    T SecondsToMilliseconds(T a) { 
        return Metric::Kilo(a); 
    }

    template<typename T>
    U64 SecondsToMicroseconds(T a) { 
        return Metric::Mega(static_cast<U64>(a)); 
    }

    template<typename T>
    U64 MillisecondsToMicroseconds(T a)  { 
        return SecondsToMicroseconds(a); 
    }

    template<typename T>
    T MillisecondsToSeconds(T a) {
        return Metric::Milli(a);
    }
}; //namespace Time

namespace Util {
    /// a la Boost
    template<typename T>
    void hash_combine(std::size_t& seed, const T& v) {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    inline void swap( char* first, char* second ) {
        char temp = *first;
        *first = *second;
        *second = temp;
    }

    
    inline void replaceStringInPlace(stringImpl& subject, 
                                     const stringImpl& search, 
                                     const stringImpl& replace ) {
        stringAlg::stringSize pos = 0;
        while ( ( pos = subject.find( search, pos ) ) != stringImpl::npos ) {
            subject.replace( pos, search.length(), replace );
            pos += replace.length();
        }
    }

    //U = to data type, T = from data type
    template<typename U, typename T>
    U convertData(const T& data){
        std::istringstream iStream(data);
        U targetValue;
        iStream >> targetValue;
        return targetValue;
    }

    template<typename T>
    stringImpl toString(T data) {
        return stringAlg::toString(data);
    }

    template<> 
    inline stringImpl toString(U8 data) {
        return stringAlg::toString(static_cast<U32>(data)); 
    }

    template<> 
    inline stringImpl toString(U16 data) { 
        return stringAlg::toString(static_cast<U32>(data)); 
    }

    template<> 
    inline stringImpl toString(I8  data) { 
        return stringAlg::toString(static_cast<I32>(data)); 
    }

    template<> 
    inline stringImpl toString(I16 data) { 
        return stringAlg::toString(static_cast<I32>(data)); 
    }

    /// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
    inline stringImpl& ltrim( stringImpl& s ) {
        s.erase( s.begin(), std::find_if(s.begin(), s.end(), 
                                         std::not1( std::ptr_fun<int, int>( std::isspace ) ) ) );
        return s;
    }

    inline stringImpl& rtrim(stringImpl& s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), 
                std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
    }

    inline stringImpl& trim(stringImpl& s) {
        return ltrim(rtrim(s));
    }

    inline F32 xfov_to_yfov(F32 xfov, F32 aspect) {
        return Angle::RadiansToDegrees(2.0f * std::atan(tan(Angle::DegreesToRadians(xfov) * 0.5f) / aspect));
    }

    inline F32 yfov_to_xfov(F32 yfov, F32 aspect) {
        return Angle::RadiansToDegrees(2.0f * std::atan(tan(Angle::DegreesToRadians(yfov) * 0.5f) * aspect));
    }

    namespace Mat4 {

        template<typename T>
        __forceinline T det( const T* mat ) {
            return ( ( mat[0] * mat[5] * mat[10] ) +
                     ( mat[4] * mat[9] * mat[2] ) +
                     ( mat[8] * mat[1] * mat[6] ) -
                     ( mat[8] * mat[5] * mat[2] ) -
                     ( mat[4] * mat[1] * mat[10] ) -
                     ( mat[0] * mat[9] * mat[6] ) );
        }

        // Copyright 2011 The Closure Library Authors. All Rights Reserved.
        template<typename T>
        __forceinline void Inverse(const T* in, T* out){
            T m00 = in[ 0], m10 = in[ 1], m20 = in[ 2], m30 = in[ 3];
            T m01 = in[ 4], m11 = in[ 5], m21 = in[ 6], m31 = in[ 7];
            T m02 = in[ 8], m12 = in[ 9], m22 = in[10], m32 = in[11];
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

        template<typename T>
        T* Multiply(const T *a, const T *b, T *r) {
            static T temp[16];
            r = temp;
            U32 row, row_offset;
            for ( row = 0, row_offset = row * 4; row < 4; ++row, row_offset = row * 4 ) {
                r[row_offset + 0] = ( a[row_offset + 0] * b[0 + 0]  ) +
                                    ( a[row_offset + 1] * b[0 + 4]  ) +
                                    ( a[row_offset + 2] * b[0 + 8]  ) +
                                    ( a[row_offset + 3] * b[0 + 12] );

                r[row_offset + 1] = ( a[row_offset + 0] * b[1 + 0]  ) +
                                    ( a[row_offset + 1] * b[1 + 4]  ) +
                                    ( a[row_offset + 2] * b[1 + 8]  ) +
                                    ( a[row_offset + 3] * b[1 + 12] );

                r[row_offset + 2] = ( a[row_offset + 0] * b[2 + 0]  ) +
                                    ( a[row_offset + 1] * b[2 + 4]  ) +
                                    ( a[row_offset + 2] * b[2 + 8]  ) +
                                    ( a[row_offset + 3] * b[2 + 12] );

                r[row_offset + 3] = ( a[row_offset + 0] * b[3 + 0]  ) +
                                    ( a[row_offset + 1] * b[3 + 4]  ) +
                                    ( a[row_offset + 2] * b[3 + 8]  ) +
                                    ( a[row_offset + 3] * b[3 + 12] );
            }
            return r;
        }
    };
}; //namespace Util

}; //namespace Divide

#endif //_CORE_MATH_MATH_HELPER_INL_