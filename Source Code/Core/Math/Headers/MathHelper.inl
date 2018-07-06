/*
   Copyright (c) 2017 DIVIDE-Studio
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

namespace detail {
    ///////////////////////////////////////////////////////////////////////////////////
    /// OpenGL Mathematics (glm.g-truc.net)
    ///
    /// Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
    /// Permission is hereby granted, free of charge, to any person obtaining a copy
    /// of this software and associated documentation files (the "Software"), to deal
    /// in the Software without restriction, including without limitation the rights
    /// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    /// copies of the Software, and to permit persons to whom the Software is
    /// furnished to do so, subject to the following conditions:
    /// 
    /// The above copyright notice and this permission notice shall be included in
    /// all copies or substantial portions of the Software.
    /// 
    /// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    /// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    /// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    /// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    /// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    /// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    /// THE SOFTWARE.
    ///
    /// @ref gtc_packing
    /// @file glm/gtc/packing.inl
    /// @date 2013-08-08 / 2013-08-08
    /// @author Christophe Riccio
    ///////////////////////////////////////////////////////////////////////////////////

    inline U32 float2packed11(U32 const & f)
    {
        // 10 bits    =>                         EE EEEFFFFF
        // 11 bits    =>                        EEE EEFFFFFF
        // Half bits  =>                   SEEEEEFF FFFFFFFF
        // Float bits => SEEEEEEE EFFFFFFF FFFFFFFF FFFFFFFF

        // 0x000007c0 => 00000000 00000000 00000111 11000000
        // 0x00007c00 => 00000000 00000000 01111100 00000000
        // 0x000003ff => 00000000 00000000 00000011 11111111
        // 0x38000000 => 00111000 00000000 00000000 00000000
        // 0x7f800000 => 01111111 10000000 00000000 00000000
        // 0x00008000 => 00000000 00000000 10000000 00000000
        return
            ((((f & 0x7f800000) - 0x38000000) >> 17) & 0x07c0) | // exponential
            ((f >> 17) & 0x003f); // Mantissa
    }

    inline U32 packed11ToFloat(U32 const & p)
    {
        // 10 bits    =>                         EE EEEFFFFF
        // 11 bits    =>                        EEE EEFFFFFF
        // Half bits  =>                   SEEEEEFF FFFFFFFF
        // Float bits => SEEEEEEE EFFFFFFF FFFFFFFF FFFFFFFF

        // 0x000007c0 => 00000000 00000000 00000111 11000000
        // 0x00007c00 => 00000000 00000000 01111100 00000000
        // 0x000003ff => 00000000 00000000 00000011 11111111
        // 0x38000000 => 00111000 00000000 00000000 00000000
        // 0x7f800000 => 01111111 10000000 00000000 00000000
        // 0x00008000 => 00000000 00000000 10000000 00000000
        return
            ((((p & 0x07c0) << 17) + 0x38000000) & 0x7f800000) | // exponential
            ((p & 0x003f) << 17); // Mantissa
    }

    inline U32 float2packed10(U32 const & f)
    {
        // 10 bits    =>                         EE EEEFFFFF
        // 11 bits    =>                        EEE EEFFFFFF
        // Half bits  =>                   SEEEEEFF FFFFFFFF
        // Float bits => SEEEEEEE EFFFFFFF FFFFFFFF FFFFFFFF

        // 0x0000001F => 00000000 00000000 00000000 00011111
        // 0x0000003F => 00000000 00000000 00000000 00111111
        // 0x000003E0 => 00000000 00000000 00000011 11100000
        // 0x000007C0 => 00000000 00000000 00000111 11000000
        // 0x00007C00 => 00000000 00000000 01111100 00000000
        // 0x000003FF => 00000000 00000000 00000011 11111111
        // 0x38000000 => 00111000 00000000 00000000 00000000
        // 0x7f800000 => 01111111 10000000 00000000 00000000
        // 0x00008000 => 00000000 00000000 10000000 00000000
        return
            ((((f & 0x7f800000) - 0x38000000) >> 18) & 0x03E0) | // exponential
            ((f >> 18) & 0x001f); // Mantissa
    }

    inline U32 packed10ToFloat(U32 const & p)
    {
        // 10 bits    =>                         EE EEEFFFFF
        // 11 bits    =>                        EEE EEFFFFFF
        // Half bits  =>                   SEEEEEFF FFFFFFFF
        // Float bits => SEEEEEEE EFFFFFFF FFFFFFFF FFFFFFFF

        // 0x0000001F => 00000000 00000000 00000000 00011111
        // 0x0000003F => 00000000 00000000 00000000 00111111
        // 0x000003E0 => 00000000 00000000 00000011 11100000
        // 0x000007C0 => 00000000 00000000 00000111 11000000
        // 0x00007C00 => 00000000 00000000 01111100 00000000
        // 0x000003FF => 00000000 00000000 00000011 11111111
        // 0x38000000 => 00111000 00000000 00000000 00000000
        // 0x7f800000 => 01111111 10000000 00000000 00000000
        // 0x00008000 => 00000000 00000000 10000000 00000000
        return
            ((((p & 0x03E0) << 18) + 0x38000000) & 0x7f800000) | // exponential
            ((p & 0x001f) << 18); // Mantissa
    }

    inline U32 floatTo11bit(F32 x)
    {
        if (x == 0.0f)
            return 0u;
        else if (std::isnan(x))
            return ~0u;
        else if (std::isinf(x))
            return 0x1f << 6;

        return float2packed11(reinterpret_cast<U32&>(x));
    }

    inline F32 packed11bitToFloat(U32 x)
    {
        if (x == 0u)
            return 0.0f;
        else if (x == ((1 << 11) - 1))
            return ~0;//NaN
        else if (x == (0x1f << 6))
            return ~0;//Inf

        U32 result = packed11ToFloat(x);
        return reinterpret_cast<F32&>(result);
    }

    inline U32 floatTo10bit(F32 x)
    {
        if (x == 0.0f)
            return 0u;
        else if (std::isnan(x))
            return ~0u;
        else if (std::isinf(x))
            return 0x1f << 5;

        return float2packed10(reinterpret_cast<U32&>(x));
    }

    inline F32 packed10bitToFloat(U32 x)
    {
        if (x == 0)
            return 0.0f;
        else if (x == ((1 << 10) - 1))
            return ~0;//NaN
        else if (x == (0x1f << 5))
            return ~0;//Inf

        U32 result = packed10ToFloat(x);
        return reinterpret_cast<F32&>(result);
    }
}

namespace customRNG
{
    namespace detail
    {
        template<typename Engine>
        Engine& getEngine() noexcept {
            static thread_local std::random_device rnddev{};
            static thread_local auto rndeng = Engine(rnddev());
            return rndeng;
        }
    }  // namespace detail

    template<typename Engine>
    void srand(const U32 seed) noexcept {
        detail::getEngine<Engine>().seed(seed);
    }

    template<typename Engine>
    void srand() {
        std::random_device rnddev{};
        srand<Engine>(rnddev());
    }

    template<typename T,
             typename Engine,
             typename Distribution>
    typename std::enable_if<std::is_fundamental<T>::value, T>::type
    rand(T min, T max) noexcept {
        return static_cast<T>(Distribution{ min, max }(detail::getEngine<Engine>()));
    }

    template<typename T,
             typename Engine,
             typename Distribution>
    T rand() noexcept {
        return rand<T, Engine, Distribution>(T(0), std::numeric_limits<T>::max());
    }
}  // namespace customRNG

template <typename T,
          typename Engine,
          typename Distribution>
T Random(T min, T max) {
    static_assert(std::is_arithmetic<T>::value, "Only arithmetic values can be randomized!");

    if (min > max) {
        std::swap(min, max);
    }

    return customRNG::rand<T, Engine, Distribution>(min, max);
}

template <typename T,
          typename Engine,
          typename Distribution>
T Random(T max) {
    static_assert(std::is_arithmetic<T>::value, "Only arithmetic values can be randomized!");

    return Random<T, Engine, Distribution>(max < 0 ? std::numeric_limits<T>::min() : 0, max);
}

template <typename T,
          typename Engine,
          typename Distribution>
T Random() {
    static_assert(std::is_arithmetic<T>::value, "Only arithmetic values can be randomized!");

    return Random<T, Engine, Distribution>(std::numeric_limits<T>::max());
}

template<typename Engine>
void SeedRandom() {
    customRNG::srand<Engine>();
}

template<typename Engine>
void SeedRandom(U32 seed) {
    customRNG::srand<Engine>(seed);
}

/// Clamps value n between min and max
template <typename T>
void CLAMP(T& n, const T min, const T max) {
    static_assert(std::is_arithmetic<T>::value, "Only arithmetic values can be clamped!");
    n = std::min(std::max(n, min), max);
}

template <typename T>
T CLAMPED(const T& n, const T min, const T max) {
    static_assert(std::is_arithmetic<T>::value, "Only arithmetic values can be clamped!");

    return std::min(std::max(n, min), max);
}

template <typename T>
T MAP(T input, const T in_min, const T in_max, const T out_min, const T out_max) {
    static_assert(std::is_arithmetic<T>::value, "Only arithmetic values can be mapped!");

    D64 slope = 1.0 * (output_end - output_start) / (input_end - input_start);
    return static_cast<T>(output_start + std::round(slope * (input - input_start)));
}

template<typename T>
bool BitCompare(const T bitMask, const T bit) {
    return BitCompare(to_U32(bitMask), to_U32(bit));
}

template<>
inline bool BitCompare<U32>(const U32 bitMask, const U32 bit) {
    return ((bitMask & bit) == bit);
}

inline void SetBit(U32& bitMask, const U32 bit) {
    bitMask |= bit;
}

inline void ClearBit(U32& bitMask, const U32 bit) {
    bitMask &= ~(bit);
}

template<typename T,
         typename = typename enable_if<is_integral<T>::value>::type,
         typename = typename enable_if<is_unsigned<T>::value>::type>
constexpr T roundup(T value,
                    unsigned maxb = sizeof(T)*CHAR_BIT,
                    unsigned curb = 1)
{
    return maxb <= curb
                ? value
                : roundup(((value - 1) | ((value - 1) >> curb)) + 1, maxb, curb << 1);
}

inline U32 nextPOW2(U32 n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    return ++n;
}

inline U32 minSquareMatrixSize(U32 elementCount) {
    U32 result = 1;
    while (result * result < elementCount) {
        ++result;
    }
    return result;
}

template <typename T, typename U>
inline T Lerp(const T v1, const T v2, const U t) {
    return v1 + (v2 - v1 * t);
}

template <typename T>
inline T Sqrt(T input) {
    return (T)std::sqrt(input);
}

template <typename T, typename U>
inline T Sqrt(U input) {
    return (T)std::sqrt(input);
}

template <>
inline F32 Sqrt(__m128 input) {
    return _mm_cvtss_f32(_mm_sqrt_ss(input));
}
///(thx sqrt[-1] and canuckle of opengl.org forums)

// Helper method to emulate GLSL
inline F32 FRACT(const F32 floatValue) {
    return to_F32(fmod(floatValue, 1.0f));
}

///Helper methods to go from a float [0 ... 1] to packed char and back
inline I8 FLOAT_TO_SCHAR_SNORM(const F32 value) {
    assert(value > 0.0f);
    return to_I8(std::min(255, (I32)(value * 256.0f)));
}

inline I8 FLOAT_TO_SCHAR(const F32 value) {
    assert(value > 0.0f);
    return to_I8(((value + 1.0f) * 0.5f) * 255.0f);
}

inline U8 FLOAT_TO_CHAR_SNORM(const F32 value) {
    return to_U8(std::min(255, (I32)(value * 256.0f)));
}

inline U8 FLOAT_TO_CHAR(const F32 value) {
    return to_U8(((value + 1.0f) * 0.5f) * 255.0f);
}

inline F32 CHAR_TO_FLOAT(const U8 value) {
    return ((value / 255.0f) * 2.0f) - 1.0f;
}

inline F32 CHAR_TO_FLOAT_SNORM(const U8 value) {
    return value / 256.0f;
}

// Pack 3 values into 1 float
inline F32 PACK_FLOAT(const U8 x, const U8 y, const U8 z) {
    static const D64 offset = to_D64(1 << 24);

    U32 packed = (x << 16) | (y << 8) | z;
    return to_F32(to_D64(packed) / offset);
}

// UnPack 3 values from 1 float
inline void UNPACK_FLOAT(const F32 src, F32& r, F32& g, F32& b) {
    r = FRACT(src);
    g = FRACT(src * 256.0f);
    b = FRACT(src * 65536.0f);

    // Unpack to the -1..1 range
    r = (r * 2.0f) - 1.0f;
    g = (g * 2.0f) - 1.0f;
    b = (b * 2.0f) - 1.0f;
}

inline U32 PACK_11_11_10(const F32 x, const F32 y, const F32 z) {
    return
        ((detail::floatTo11bit(x) & ((1 << 11) - 1)) << 0) |
        ((detail::floatTo11bit(y) & ((1 << 11) - 1)) << 11) |
        ((detail::floatTo10bit(z) & ((1 << 10) - 1)) << 22);
}

inline void UNPACK_11_11_10(const U32 src, F32& x, F32& y, F32& z) {
    x = detail::packed11bitToFloat(src >> 0);
    y = detail::packed11bitToFloat(src >> 11);
    z = detail::packed10bitToFloat(src >> 22);
}

namespace Angle {

template<typename T>
using RADIANS = T;

template<typename T>
using DEGREES = T;

template <typename T>
constexpr RADIANS<T> to_RADIANS(const DEGREES<T> angle) {
    return static_cast<RADIANS<T>>(angle * M_PIDIV180);
}

template <typename T>
constexpr DEGREES<T> to_DEGREES(const RADIANS<T> angle) {
    return static_cast<DEGREES<T>>(angle * M_180DIVPI);
}

template <typename T>
constexpr vec2<RADIANS<T>> to_RADIANS(const vec2<DEGREES<T>>& angle) {
    return vec2<RADIANS<T>>(angle * M_PIDIV180);
}

template <typename T>
constexpr vec2<DEGREES<T>> to_DEGREES(const vec2<RADIANS<T>>& angle) {
    return vec2<RADIANS<T>>(angle * M_180DIVPI);
}

template <typename T>
constexpr vec3<RADIANS<T>> to_RADIANS(const vec3<DEGREES<T>>& angle) {
    return vec3<RADIANS<T>>(angle * M_PIDIV180);
}

template <typename T>
constexpr vec3<DEGREES<T>> to_DEGREES(const vec3<RADIANS<T>>& angle) {
    return vec3<DEGREES<T>>(angle * M_180DIVPI);
}

template <typename T>
constexpr vec4<RADIANS<T>> to_RADIANS(const vec4<DEGREES<T>>& angle) {
    return vec4<RADIANS<T>>(angle * M_PIDIV180);
}

template <typename T>
constexpr vec4<DEGREES<T>> to_DEGREES(const vec4<RADIANS<T>>& angle) {
    return vec4<DEGREES<T>>(angle * M_180DIVPI);
}

};  // namespace Angle

namespace Metric {

/// Base value * 1000000000000
template <typename T>
constexpr T Tera(const T a) {
    return Tera<T, T>(a);
}

/// Base value * 1000000000
template <typename T>
constexpr T Giga(const T a) {
    return Giga<T, T>(a);
}

/// Base value * 1000000
template <typename T>
constexpr T Mega(const T a) {
    return Mega<T, T>(a);
}

/// Base value * 1000
template <typename T>
constexpr T Kilo(const T a) {
    return Kilo<T, T>(a);
}

/// Base value * 100
template <typename T>
constexpr T Hecto(const T a) {
    return Hecto<T, T>(a);
}

/// Base value * 10
template <typename T>
constexpr T Deca(const T a) {
    return Deca<T, T>(a);
}

/// Base value
template <typename T>
constexpr T Base(const T a) {
    return Base<T, T>(a);
}

/// Base value * 0.1
template <typename T>
constexpr T Deci(const T a) {
    return Deci<T, T>(a);
}

/// Base value * 0.01
template <typename T>
constexpr T Centi(const T a) {
    return Centi<T, T>(a);
}

/// Base value * 0.001
template <typename T>
constexpr T Milli(const T a) {
    return Milli<T, T>(a);
}

/// Base value * 0.000001
template <typename T>
constexpr T Micro(const T a) {
    return Micro<T, T>(a);
}

/// Base value * 0.000000001
template <typename T>
constexpr T Nano(const T a) {
    return Nano<T, T>(a);
}

/// Base value * 0.000000000001
template <typename T>
constexpr T Pico(const T a) {
    return Pico<T, T>(a);
}


template <typename T, typename U>
constexpr T Tera(const U a) {
    return static_cast<T>(multiply(a, 1'000'000'000'000));
}

template <typename T, typename U>
constexpr T Giga(const U a) {
    return static_cast<T>(multiply(a, 1'000'000'000));
}

template <typename T, typename U>
constexpr T Mega(const U a) {
    return static_cast<T>(multiply(a, 1'000'000));
}

template <typename T, typename U>
constexpr T Kilo(const U a) {
    return static_cast<T>(multiply(a, 1'000));
}

template <typename T, typename U>
constexpr T Hecto(const U a) {
    return static_cast<T>(multiply(a, 100));
}

template <typename T, typename U>
constexpr T Deca(const U a) {
    return static_cast<T>(multiply(a, 10));
}

template <typename T, typename U>
constexpr T Base(const U a) {
    return static_cast<T>(a);
}

template <typename T, typename U>
constexpr T Deci(const U a) {
    return static_cast<T>(divide(a, 10.0));
}

template <typename T, typename U>
constexpr T Centi(const U a) {
    return static_cast<T>(divide(a, 100.0));
}

template <typename T, typename U>
constexpr T Milli(const U a) {
    return static_cast<T>(divide(a, 1000.0));
}

template <typename T, typename U>
constexpr T Micro(const U a) {
    return static_cast<T>(divide(a, 1e6));
}

template <typename T, typename U>
constexpr T Nano(const U a) {
    return static_cast<T>(divide(a, 1e9));
}

template <typename T, typename U>
constexpr T Pico(const U a) {
    return static_cast<T>(divide(a,  1e12));
}

};  // namespace Metric

namespace Time {

template <typename T>
T Seconds(const T a) {
    return a;
}

template <typename T>
T Milliseconds(const T a) {
    return a;
}

template <typename T>
T Microseconds(const T a) {
    return a;
}

template <typename T>
T Nanoseconds(const T a) {
    return a;
}

template <typename T, typename U>
T Seconds(const U a) {
    return static_cast<T>(a);
}

template <typename T, typename U>
T Milliseconds(const U a) {
    return static_cast<T>(a);
}

template <typename T, typename U>
T Microseconds(const U a) {
    return static_cast<T>(a);
}

template <typename T, typename U>
T Nanoseconds(const U a) {
    return static_cast<T>(a);
}

template <typename T, typename U>
T NanosecondsToSeconds(const U a) {
    return Metric::Nano<T, U>(a);
}

template <typename T, typename U>
T NanosecondsToMilliseconds(const U a) {
    return Metric::Micro<T, U>(a);
}

template <typename T, typename U>
T NanosecondsToMicroseconds(const U a) {
    return Metric::Milli<T, U>(a);
}

template <typename T, typename U>
T MicrosecondsToSeconds(const U a) {
    return Metric::Micro<T, U>(a);
}

template <typename T, typename U>
T MicrosecondsToMilliseconds(const U a) {
    return Metric::Milli<T, U>(a);
}

template <typename T, typename U>
T MicrosecondsToNanoseconds(const U a) {
    return Metric::Kilo<T, U>(a);
}

template <typename T, typename U>
T MillisecondsToSeconds(const U a) {
    return Metric::Milli<T, U>(a);
}

template <typename T, typename U>
T MillisecondsToMicroseconds(const U a) {
    return Metric::Kilo<T, U>(a);
}

template <typename T, typename U>
T MillisecondsToNanoseconds(const U a) {
    return Metric::Mega<T, U>(a);
}

template <typename T, typename U>
T SecondsToMilliseconds(const U a) {
    return Metric::Kilo<T, U>(a);
}

template <typename T, typename U>
T SecondsToMicroseconds(const U a) {
    return Metric::Mega<T, U>(a);
}

template <typename T, typename U>
T SecondsToNanoseconds(const U a) {
    return Metric::Giga<T, U>(a);
}

};  // namespace Time

namespace Util {
/// a la Boost
template <typename T>
void Hash_combine(size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline void ReplaceStringInPlace(stringImpl& subject, 
                                 const stringImpl& search,
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

template<class FwdIt, class Compare>
void InsertionSort(FwdIt first, FwdIt last, Compare cmp)
{
    for (auto it = first; it != last; ++it) {
        auto const insertion = std::upper_bound(first, it, *it, cmp);
        std::rotate(insertion, it, std::next(it));
    }
}

};  // namespace Util
};  // namespace Divide

#endif  //_CORE_MATH_MATH_HELPER_INL_