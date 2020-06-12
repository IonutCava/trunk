/*
   Copyright (c) 2018 DIVIDE-Studio
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

    inline U32 float2packed11(U32 const & f) noexcept
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

    inline U32 packed11ToFloat(U32 const & p) noexcept
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

    inline U32 float2packed10(U32 const & f) noexcept
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

    inline U32 packed10ToFloat(U32 const & p) noexcept {
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

    inline U32 floatTo11bit(F32 x) noexcept {
             if (x == 0.0f)     return 0u;
        else if (std::isnan(x)) return ~0u;
        else if (std::isinf(x)) return 0x1f << 6;

        return float2packed11(reinterpret_cast<U32&>(x));
    }

    inline F32 packed11bitToFloat(U32 x) noexcept {
             if (x == 0u)              return 0.0f;
        else if (x == ((1 << 11) - 1)) return ~0;//NaN
        else if (x == (0x1f << 6))     return ~0;//Inf

        U32 result = packed11ToFloat(x);
        return reinterpret_cast<F32&>(result);
    }

    inline U32 floatTo10bit(F32 x) noexcept {
             if (x == 0.0f)     return 0u;
        else if (std::isnan(x)) return ~0u;
        else if (std::isinf(x)) return 0x1f << 5;

        return float2packed10(reinterpret_cast<U32&>(x));
    }

    inline F32 packed10bitToFloat(U32 x) noexcept {
             if (x == 0)               return 0.0f;
        else if (x == ((1 << 10) - 1)) return ~0;//NaN
        else if (x == (0x1f << 5))     return ~0;//Inf

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
            static thread_local Engine rndeng = Engine(rnddev());
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
T Random(T min, T max) noexcept {
    static_assert(std::is_arithmetic<T>::value, "Only arithmetic values can be randomized!");

    if (min > max) {
        std::swap(min, max);
    }

    return customRNG::rand<T, Engine, Distribution>(min, max);
}

template <typename T,
          typename Engine,
          typename Distribution>
T Random(T max) noexcept {
    static_assert(std::is_arithmetic<T>::value, "Only arithmetic values can be randomized!");

    return Random<T, Engine, Distribution>(max < 0 ? std::numeric_limits<T>::lowest() : 0, max);
}

template <typename T,
          typename Engine,
          typename Distribution>
T Random() noexcept {
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
template <typename T, typename U>
void CLAMP(T& n, const U min, const U max) noexcept {
    static_assert(std::is_arithmetic<T>::value &&
                  std::is_arithmetic<U>::value,
                  "Only arithmetic values can be clamped!");
    n = std::min(std::max(n, static_cast<T>(min)), static_cast<T>(max));
}

template <typename T>
void CLAMP_01(T& n) noexcept {
    return CLAMP(n, 0, 1);
}

template <typename T, typename U>
T CLAMPED(const T& n, const U min, const U max) noexcept {
    static_assert(std::is_arithmetic<T>::value &&
                  std::is_arithmetic<U>::value,
                  "Only arithmetic values can be clamped!");

    T ret = n;
    CLAMP(ret, min, max);
    return ret;
}

template <typename T>
T CLAMPED_01(const T& n) noexcept {
    return CLAMPED(n, T(0), T(1));
}

template <typename T>
T MAP(T input, const T in_min, const T in_max, const T out_min, const T out_max, D64& slopeOut) noexcept {
    static_assert(std::is_arithmetic<T>::value, "Only arithmetic values can be mapped!");
    const D64 diff = in_max > in_min ? to_D64(in_max - in_min) : std::numeric_limits<D64>::epsilon();
    slopeOut = 1.0 * (out_max - out_min) / diff;
    return static_cast<T>(out_min + (slopeOut * (input - in_min)));
}

template <typename T>
T SQUARED(T input) noexcept {
    static_assert(std::is_arithmetic<T>::value, "Only arithmetic values can be squared!");
    return input * input;
}

template<typename T>
T SIGNED_SQUARED(T input) noexcept {
    static_assert(std::is_arithmetic<T>::value, "Only arithmetic values can be squared!");
    return std::copysign(SQUARED(input), input);
}

template<typename T>
void CLAMP_IN_RECT(T& inout_x, T& inout_y, T rect_x, T rect_y, T rect_z, T rect_w) noexcept {
    CLAMP(inout_x, rect_x, rect_z + rect_x);
    CLAMP(inout_y, rect_y, rect_w + rect_y);
}

template<typename T>
void CLAMP_IN_RECT(T& inout_x, T& inout_y, const Rect<T>& rect) noexcept {
    CLAMP_IN_RECT(inout_x, inout_y, rect.x, rect.y, rect.z, rect.w);
}

template<typename T>
void CLAMP_IN_RECT(T& inout_x, T& inout_y, const vec4<T>& rect) noexcept {
    CLAMP_IN_RECT(inout_x, inout_y, rect.x, rect.y, rect.z, rect.w);
}

template <typename T>
bool COORDS_IN_RECT(T input_x, T input_y, T rect_x, T rect_y, T rect_z, T rect_w) noexcept {
    return IS_IN_RANGE_INCLUSIVE(input_x, rect_x, rect_z + rect_x) &&
           IS_IN_RANGE_INCLUSIVE(input_y, rect_y, rect_w + rect_y);
}

template<typename T>
bool COORDS_IN_RECT(T input_x, T input_y, const Rect<T>& rect) noexcept {
    return COORDS_IN_RECT(input_x, input_y, rect.x, rect.y, rect.z, rect.w);
}

template <typename T>
bool COORDS_IN_RECT(T input_x, T input_y, const vec4<T>& rect) noexcept {
    return COORDS_IN_RECT(input_x, input_y, rect.x, rect.y, rect.z, rect.w);
}

template<typename Mask, typename Type>
constexpr typename std::enable_if<std::is_enum<Type>::value, bool>::type
BitCompare(const Mask bitMask, const Type bit) noexcept {
    return BitCompare(bitMask, static_cast<Mask>(bit));
}

template<typename Mask, typename Type>
constexpr typename std::enable_if<std::is_enum<Type>::value, void>::type
SetBit(Mask& bitMask, const Type bit) noexcept {
    SetBit(bitMask, static_cast<Mask>(bit));
}

template<typename Mask, typename Type>
constexpr typename std::enable_if<std::is_enum<Type>::value, void>::type
ClearBit(Mask& bitMask, const Type bit) noexcept {
    ClearBit(bitMask, static_cast<Mask>(bit));
}

template<typename Mask, typename Type>
constexpr typename std::enable_if<std::is_enum<Type>::value, void>::type
ToggleBit(Mask& bitMask, const Type bit) noexcept {
    ToggleBit(bitMask, static_cast<Mask>(bit));
}

template<typename Mask, typename Type>
constexpr typename std::enable_if<std::is_enum<Type>::value, void>::type
ToggleBit(Mask& bitMask, const Type bit, bool state) noexcept {
    ToggleBit(bitMask, static_cast<Mask>(bit), state);
}

template<typename Mask>
constexpr bool AnyCompare(const Mask bitMask, const Mask checkMask) noexcept {
    static_assert(std::is_integral<Mask>::value, "Invalid bit mask type!");
    return ((bitMask & checkMask) != 0);
}

template<typename Mask>
constexpr bool AllCompare(const Mask bitMask, const Mask checkMask) noexcept {
    static_assert(std::is_integral<Mask>::value, "Invalid bit mask type!");
    return ((bitMask & checkMask) == checkMask);
}

template<typename Mask>
constexpr bool BitCompare(const Mask bitMask, const Mask bit) noexcept {
    static_assert(std::is_integral<Mask>::value, "Invalid bit mask type!");
    return ((bitMask & bit) == bit);
}

template<typename Mask>
constexpr void SetBit(Mask& bitMask, const Mask bit) noexcept {
    static_assert(std::is_integral<Mask>::value, "Invalid bit mask type!");
    bitMask |= bit;
}

template<typename Mask>
constexpr void ClearBit(Mask& bitMask, const Mask bit) noexcept {
    static_assert(std::is_integral<Mask>::value, "Invalid bit mask type!");
    bitMask &= ~(bit);
}

template<typename Mask>
constexpr void ToggleBit(Mask& bitMask, const Mask bit) noexcept {
    static_assert(std::is_integral<Mask>::value, "Invalid bit mask type!");
    bitMask ^= 1 << bit;
}

template<typename Mask>
constexpr void ToggleBit(Mask& bitMask, const Mask bit, bool state) noexcept {
    static_assert(std::is_integral<Mask>::value, "Invalid bit mask type!");
    if (state) {
        SetBit(bitMask, bit);
    } else {
        ClearBit(bitMask, bit);
    }
}

template<typename Mask, typename Type>
constexpr typename std::enable_if<std::is_enum<Type>::value, bool>::type
BitCompare(const std::atomic<Mask> bitMask, const Type bit) noexcept {
    return BitCompare(bitMask, static_cast<Mask>(bit));
}

template<typename Mask, typename Type>
constexpr typename std::enable_if<std::is_enum<Type>::value, void>::type
SetBit(std::atomic<Mask>& bitMask, const Type bit) noexcept {
    SetBit(bitMask, static_cast<Mask>(bit));
}

template<typename Mask, typename Type>
constexpr typename std::enable_if<std::is_enum<Type>::value, void>::type
ClearBit(std::atomic<Mask>& bitMask, const Type bit) noexcept {
    ClearBit(bitMask, static_cast<Mask>(bit));
}

template<typename Mask>
constexpr bool AnyCompare(const std::atomic<Mask> bitMask, const Mask checkMask) noexcept {
    static_assert(std::is_integral<Mask>::value, "Invalid bit mask type!");
    return ((bitMask & checkMask) != 0);
}

template<typename Mask>
constexpr bool BitCompare(const std::atomic<Mask>& bitMask, const Mask bit) noexcept {
    static_assert(std::is_integral<Mask>::value, "Invalid bit mask type!");
    return ((bitMask & bit) == bit);
}

template<typename Mask>
constexpr void SetBit(std::atomic<Mask>& bitMask, const Mask bit) noexcept {
    static_assert(std::is_integral<Mask>::value, "Invalid bit mask type!");
    bitMask |= bit;
}

template<typename Mask>
constexpr void ClearBit(std::atomic<Mask>& bitMask, const Mask bit) noexcept {
    static_assert(std::is_integral<Mask>::value, "Invalid bit mask type!");
    bitMask &= ~(bit);
}

template<typename Mask>
constexpr void ToggleBit(std::atomic<Mask>& bitMask, const Mask bit) noexcept {
    static_assert(std::is_integral<Mask>::value, "Invalid bit mask type!");
    bitMask ^= 1 << bit;
}

template<typename T,
         typename = typename std::enable_if<std::is_integral<T>::value>::type,
         typename = typename std::enable_if<std::is_unsigned<T>::value>::type>
constexpr T roundup(T value,
                    unsigned maxb = sizeof(T)*CHAR_BIT,
                    unsigned curb = 1)
{
    return maxb <= curb
                ? value
                : roundup(((value - 1) | ((value - 1) >> curb)) + 1, maxb, curb << 1);
}

constexpr U32 nextPOW2(U32 n) noexcept {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    return ++n;
}

constexpr U32 prevPOW2(U32 n) noexcept {
    n = n | (n >> 1);
    n = n | (n >> 2);
    n = n | (n >> 4);
    n = n | (n >> 8);
    n = n | (n >> 16);
    return n - (n >> 1);
}

constexpr U32 minSquareMatrixSize(U32 elementCount) noexcept {
    U32 result = 1;
    while (result * result < elementCount) {
        ++result;
    }
    return result;
}

template <typename T, typename U>
T Lerp(const T v1, const T v2, const U t) {
#if defined(FAST_LERP)
    return v1 + t * (v2 - v1);
#else
    return (v1 * (static_cast<U>(1) - t)) + (v2 * t);
#endif
}

template <typename T>
T Sqrt(T input) noexcept {
    return static_cast<T>(std::sqrt(input));
}

template <typename T, typename U>
T Sqrt(U input) noexcept {
    return static_cast<T>(std::sqrt(input));
}

template <>
inline F32 Sqrt(const __m128 input) noexcept {
    return _mm_cvtss_f32(_mm_sqrt_ss(input));
}
///(thx sqrt[-1] and canuckle of opengl.org forums)

// Helper method to emulate GLSL
inline F32 FRACT(const F32 floatValue) noexcept {
    return to_F32(fmod(floatValue, 1.0f));
}

///Helper methods to go from a float [0 ... 1] to packed char and back
inline I8 FLOAT_TO_SCHAR_SNORM(const F32 value) noexcept {
    assert(value > 0.0f);
    return to_I8(std::min(255, to_I32(value * 256.0f)));
}

inline I8 FLOAT_TO_SCHAR(const F32 value) noexcept {
    assert(value > 0.0f);
    return to_I8(((value + 1.0f) * 0.5f) * 255.0f);
}

constexpr U8 FLOAT_TO_CHAR_SNORM(const F32 value) noexcept {
    return to_U8(std::min(255, to_I32((value * 256.0f))));
}

constexpr U8 FLOAT_TO_CHAR(const F32 value) noexcept {
    return to_U8(((value + 1.0f) * 0.5f) * 255.0f);
}

constexpr F32 CHAR_TO_FLOAT(const U8 value) noexcept {
    return ((value / 255.0f) * 2.0f) - 1.0f;
}

constexpr F32 CHAR_TO_FLOAT_SNORM(const U8 value)noexcept {
    return value / 256.0f;
}

// Pack 3 values into 1 float
inline F32 PACK_FLOAT(const U8 x, const U8 y, const U8 z) noexcept {
    constexpr D64 offset = to_D64(1 << 24);

    const U32 packed = (x << 16) | (y << 8) | z;
    return to_F32(to_D64(packed) / offset);
}

// UnPack 3 values from 1 float
inline void UNPACK_FLOAT(const F32 src, F32& r, F32& g, F32& b) noexcept {
    r = FRACT(src);
    g = FRACT(src * 256.0f);
    b = FRACT(src * 65536.0f);

    // Unpack to the -1..1 range
    r = (r * 2.0f) - 1.0f;
    g = (g * 2.0f) - 1.0f;
    b = (b * 2.0f) - 1.0f;
}

inline U32 PACK_11_11_10(const F32 x, const F32 y, const F32 z) noexcept {
    return
        ((detail::floatTo11bit(x) & ((1 << 11) - 1)) << 0) |
        ((detail::floatTo11bit(y) & ((1 << 11) - 1)) << 11) |
        ((detail::floatTo10bit(z) & ((1 << 10) - 1)) << 22);
}

inline void UNPACK_11_11_10(const U32 src, F32& x, F32& y, F32& z) noexcept {
    x = detail::packed11bitToFloat(src >> 0);
    y = detail::packed11bitToFloat(src >> 11);
    z = detail::packed10bitToFloat(src >> 22);
}

namespace Angle {

template <typename T>
constexpr DEGREES<T> to_VerticalFoV(const DEGREES<T> horizontalFoV, D64 aspectRatio) noexcept {
    return static_cast<DEGREES<T>>(
        to_DEGREES(2 * std::atan(std::tan(Angle::to_RADIANS(horizontalFoV) * 0.5f) / aspectRatio))
    );
}

template <typename T>
constexpr DEGREES<T> to_HorizontalFoV(const DEGREES<T> verticalFoV, D64 aspectRatio) noexcept {
    return static_cast<DEGREES<T>>(
        to_DEGREES(2 * std::atan(std::tan(Angle::to_RADIANS(verticalFoV) * 0.5f)) * aspectRatio)
    );
}

template <typename T>
constexpr RADIANS<T> to_RADIANS(const DEGREES<T> angle) noexcept {
    return static_cast<RADIANS<T>>(angle * M_PIDIV180);
}

template <typename T>
constexpr DEGREES<T> to_DEGREES(const RADIANS<T> angle) noexcept {
    return static_cast<DEGREES<T>>(angle * M_180DIVPI);
}

template <typename T>
constexpr vec2<RADIANS<T>> to_RADIANS(const vec2<DEGREES<T>>& angle) noexcept {
    return vec2<RADIANS<T>>(angle * M_PIDIV180);
}

template <typename T>
constexpr vec2<DEGREES<T>> to_DEGREES(const vec2<RADIANS<T>>& angle) noexcept {
    return vec2<RADIANS<T>>(angle * M_180DIVPI);
}

template <typename T>
constexpr vec3<RADIANS<T>> to_RADIANS(const vec3<DEGREES<T>>& angle) noexcept {
    return vec3<RADIANS<T>>(angle * M_PIDIV180);
}

template <typename T>
constexpr vec3<DEGREES<T>> to_DEGREES(const vec3<RADIANS<T>>& angle) noexcept {
    return vec3<DEGREES<T>>(angle * M_180DIVPI);
}

template <typename T>
constexpr vec4<RADIANS<T>> to_RADIANS(const vec4<DEGREES<T>>& angle) noexcept {
    return vec4<RADIANS<T>>(angle * M_PIDIV180);
}

template <typename T>
constexpr vec4<DEGREES<T>> to_DEGREES(const vec4<RADIANS<T>>& angle) noexcept {
    return vec4<DEGREES<T>>(angle * M_180DIVPI);
}

/// Return the radian equivalent of the given degree value
template <typename T>
constexpr T DegreesToRadians(const T angleDegrees) noexcept {
    return to_RADIANS(angleDegrees);
}

/// Return the degree equivalent of the given radian value
template <typename T>
constexpr T RadiansToDegrees(const T angleRadians) noexcept {
    return to_DEGREES(angleRadians);
}
/// Returns the specified value. Used only for emphasis
template <typename T>
constexpr T Degrees(const T degrees) noexcept {
    return degrees;
}

/// Returns the specified value. Used only for emphasis
template <typename T>
constexpr T Radians(const T radians) noexcept {
    return radians;
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
constexpr T Seconds(const T a) {
    return a;
}

template <typename T>
constexpr T Milliseconds(const T a) {
    return a;
}

template <typename T>
constexpr T Microseconds(const T a) {
    return a;
}

template <typename T>
constexpr T Nanoseconds(const T a) {
    return a;
}

template <typename T, typename U>
constexpr T Seconds(const U a) {
    return static_cast<T>(a);
}

template <typename T, typename U>
constexpr T Milliseconds(const U a) {
    return static_cast<T>(a);
}

template <typename T, typename U>
constexpr T Microseconds(const U a) {
    return static_cast<T>(a);
}

template <typename T, typename U>
constexpr T Nanoseconds(const U a) {
    return static_cast<T>(a);
}

template <typename T, typename U>
constexpr T NanosecondsToSeconds(const U a) noexcept {
    return Metric::Nano<T, U>(a);
}

template <typename T, typename U>
constexpr T NanosecondsToMilliseconds(const U a) noexcept {
    return Metric::Micro<T, U>(a);
}

template <typename T, typename U>
constexpr T NanosecondsToMicroseconds(const U a) noexcept {
    return Metric::Milli<T, U>(a);
}

template <typename T, typename U>
constexpr T MicrosecondsToSeconds(const U a) noexcept {
    return Metric::Micro<T, U>(a);
}

template <typename T, typename U>
constexpr T MicrosecondsToMilliseconds(const U a) noexcept {
    return Metric::Milli<T, U>(a);
}

template <typename T, typename U>
constexpr T MicrosecondsToNanoseconds(const U a) noexcept {
    return Metric::Kilo<T, U>(a);
}

template <typename T, typename U>
constexpr T MillisecondsToSeconds(const U a) noexcept {
    return Metric::Milli<T, U>(a);
}

template <typename T, typename U>
constexpr T MillisecondsToMicroseconds(const U a) noexcept {
    return Metric::Kilo<T, U>(a);
}

template <typename T, typename U>
constexpr T MillisecondsToNanoseconds(const U a) noexcept {
    return Metric::Mega<T, U>(a);
}

template <typename T, typename U>
constexpr T SecondsToMilliseconds(const U a) noexcept {
    return Metric::Kilo<T, U>(a);
}

template <typename T, typename U>
constexpr T SecondsToMicroseconds(const U a) noexcept {
    return Metric::Mega<T, U>(a);
}

template <typename T, typename U>
constexpr T SecondsToNanoseconds(const U a) noexcept {
    return Metric::Giga<T, U>(a);
}

};  // namespace Time

namespace Util {

/// a la Boost
template <typename T>
void Hash_combine(size_t& seed, const T& v) noexcept {
    eastl::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<>
FORCE_INLINE void Hash_combine(size_t& seed, const size_t& hash) noexcept {
    seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
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
        auto const insertion = eastl::upper_bound(first, it, *it, cmp);
        eastl::rotate(insertion, it, eastl::next(it));
    }
}

};  // namespace Util
};  // namespace Divide

#endif  //_CORE_MATH_MATH_HELPER_INL_