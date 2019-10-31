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

#pragma once
#ifndef _PLATFORM_DATA_TYPES_H_
#define _PLATFORM_DATA_TYPES_H_

namespace Divide {

// "Exact" number of bits
typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t   I8;
typedef int16_t  I16;
typedef int32_t  I32;
typedef int64_t  I64;

// "At least" number of bits
typedef uint_least8_t  U8x;
typedef uint_least16_t U16x;
typedef uint_least32_t U32x;
typedef uint_least64_t U64x;
typedef int_least8_t   I8x;
typedef int_least16_t  I16x;
typedef int_least32_t  I32x;
typedef int_least64_t  I64x;

//double is 8 bytes with Microsoft's compiler)
typedef float F32;
typedef double D64;
typedef long double D128;

typedef void* bufferPtr;

typedef int8_t Byte;

typedef union {
    U32 i;
    U8  b[4];
} P32;

typedef union {
    U64 i;
    U8  b[8];
} P64;

inline bool operator==(const P32& lhs, const P32& rhs) {
    return lhs.i == rhs.i;
}
inline bool operator!=(const P32& lhs, const P32& rhs) {
    return lhs.i != rhs.i;
}
inline bool operator==(const P64& lhs, const P64& rhs) {
    return lhs.i == rhs.i;
}
inline bool operator!=(const P64& lhs, const P64& rhs) {
    return lhs.i != rhs.i;
}

//Ref: https://stackoverflow.com/questions/7416699/how-to-define-24bit-data-type-in-c
const I32 INT24_MAX = 8388607;

#pragma pack(push, 1)
class I24
{
protected:
    U8 value[3];

public:
    I24() noexcept
        : I24(0)
    {
    }

    I24(I32 val) noexcept 
    {
        *this = val;
    }

    I24(const I24& val) noexcept
    {
        *this = val;
    }

    I24(I24&& other) noexcept
        : value{ std::move(other.value[0]), std::move(other.value[1]), std::move(other.value[2]) }
    {
    }

    FORCE_INLINE I24& operator= (I24&& other) noexcept {
        value[0] = std::move(other.value[0]);
        value[1] = std::move(other.value[1]);
        value[2] = std::move(other.value[2]);
        return *this;
    }

    FORCE_INLINE I24& operator= (const I24& input) {
        std::memcpy(value, input.value, sizeof(U8) * 3);
        return *this;
    }

    FORCE_INLINE I24& operator= (const I32 input) noexcept {
        value[0] = ((U8*)&input)[0];
        value[1] = ((U8*)&input)[1];
        value[2] = ((U8*)&input)[2];

        return *this;
    }

    FORCE_INLINE operator I32() const noexcept {
        /* Sign extend negative quantities */
        if (value[2] & 0x80) {
            return (0xff << 24) | (value[2] << 16) | (value[1] << 8) | value[0];
        }
         
        return (value[2] << 16) | (value[1] << 8) | value[0];
    }

    FORCE_INLINE I24 operator+   (I32 val)        const noexcept { return I24(static_cast<I32>(*this) + static_cast<I32>(val)); }
    FORCE_INLINE I24 operator-   (I32 val)        const noexcept { return I24(static_cast<I32>(*this) - static_cast<I32>(val)); }
    FORCE_INLINE I24 operator+   (const I24& val) const noexcept { return I24(static_cast<I32>(*this) + static_cast<I32>(val)); }
    FORCE_INLINE I24 operator-   (const I24& val) const noexcept { return I24(static_cast<I32>(*this) - static_cast<I32>(val)); }
    FORCE_INLINE I24 operator*   (const I24& val) const noexcept { return I24(static_cast<I32>(*this) * static_cast<I32>(val)); }
    FORCE_INLINE I24 operator/   (const I24& val) const noexcept { return I24(static_cast<I32>(*this) / static_cast<I32>(val)); }
    FORCE_INLINE I24& operator+= (const I24& val)       noexcept { *this = *this + val; return *this; }
    FORCE_INLINE I24& operator-= (const I24& val)       noexcept { *this = *this - val; return *this; }
    FORCE_INLINE I24& operator*= (const I24& val)       noexcept { *this = *this * val; return *this; }
    FORCE_INLINE I24& operator/= (const I24& val)       noexcept { *this = *this / val; return *this; }
    FORCE_INLINE I24 operator>>  (const I32 val) const  noexcept { return I24(static_cast<I32>(*this) >> val); }
    FORCE_INLINE I24 operator<<  (const I32 val) const  noexcept { return I24(static_cast<I32>(*this) << val); }

    FORCE_INLINE operator bool()   const noexcept { return static_cast<I32>(*this) != 0; }
    FORCE_INLINE bool operator! () const noexcept { return !(static_cast<I32>(*this)); }
    FORCE_INLINE I24  operator- ()       noexcept { return I24(-static_cast<I32>(*this)); }
    FORCE_INLINE I24& operator++ ()      noexcept { *this = *this + 1; return *this; }
    FORCE_INLINE I24& operator-- ()      noexcept { *this = *this - 1; return *this; }

    FORCE_INLINE I24  operator++ (I32)                  noexcept { I24 ret = *this; ++(*this); return ret; }
    FORCE_INLINE I24  operator-- (I32)                  noexcept { I24 ret = *this; --(*this); return ret; }
    FORCE_INLINE bool operator== (const I24& val) const noexcept { return static_cast<I32>(*this) == static_cast<I32>(val); }
    FORCE_INLINE bool operator!= (const I24& val) const noexcept { return static_cast<I32>(*this) != static_cast<I32>(val); }
    FORCE_INLINE bool operator>= (const I24& val) const noexcept { return static_cast<I32>(*this) >= static_cast<I32>(val); }
    FORCE_INLINE bool operator<= (const I24& val) const noexcept { return static_cast<I32>(*this) <= static_cast<I32>(val); }
    FORCE_INLINE bool operator>  (const I24& val) const noexcept { return static_cast<I32>(*this) >  static_cast<I32>(val); }
    FORCE_INLINE bool operator<  (const I24& val) const noexcept { return static_cast<I32>(*this) <  static_cast<I32>(val); }
    FORCE_INLINE bool operator>  (const size_t& val) const noexcept {
        const I32 lhs = static_cast<I32>(*this);
        return lhs >=0 && static_cast<size_t>(lhs) > val;
    }
    FORCE_INLINE bool operator<  (const size_t& val) const noexcept { 
        const I32 lhs = static_cast<I32>(*this);
        return lhs < 0 || static_cast<size_t>(lhs) < val;
    }
    FORCE_INLINE bool operator>  (const U32& val) const noexcept {
        const I32 lhs = static_cast<I32>(*this);
        return lhs >= 0 && static_cast<U32>(lhs) > val; 
    }
    FORCE_INLINE bool operator<  (const U32& val) const noexcept {
        const I32 lhs = static_cast<I32>(*this);
        return lhs < 0 || static_cast<U32>(lhs) < val;
    }
};
#pragma pack(pop)

enum class CallbackParam : U8 {
    TYPE_SMALL_INTEGER = 0,
    TYPE_MEDIUM_INTEGER,
    TYPE_INTEGER,
    TYPE_LARGE_INTEGER,
    TYPE_SMALL_UNSIGNED_INTEGER,
    TYPE_MEDIUM_UNSIGNED_INTEGER,
    TYPE_UNSIGNED_INTEGER,
    TYPE_LARGE_UNSIGNED_INTEGER,
    TYPE_STRING,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_LONG_DOUBLE,
    TYPE_CHAR,
    TYPE_BOOL,
    TYPE_VOID,
    COUNT
};

template <typename From, typename To>
struct static_caster
{
    To operator()(From p) { return static_cast<To>(p); }
};

/*
template<typename Enum>
constexpr U32 operator"" _u32 ( Enum value )
{
return static_cast<U32>(value);
}*/

template <typename Type, typename = typename std::enable_if<std::is_integral<Type>::value>::type>
constexpr auto to_base(const Type value) -> Type {
    return value;
}

template <typename Type, typename = typename std::enable_if<std::is_enum<Type>::value>::type>
constexpr auto to_base(const Type value) -> std::underlying_type_t<Type> {
    return static_cast<std::underlying_type_t<Type>>(value);
}

template <typename T>
constexpr size_t to_size(const T value) {
    return static_cast<size_t>(value);
}

template <typename T>
constexpr U64 to_U64(const T value) {
    return static_cast<U64>(value);
}

template <typename T>
constexpr U32 to_U32(const T value) {
    return static_cast<U32>(value);
}

template <typename T>
constexpr U16 to_U16(const T value) {
    return static_cast<U16>(value);
}

template <typename T>
constexpr U8 to_U8(const T value) {
    return static_cast<U8>(value);
}

template <typename T>
constexpr I64 to_I64(const T value) {
    return static_cast<I64>(value);
}

template <typename T>
constexpr I32 to_I32(const T value) {
    return static_cast<I32>(value);
}

template <typename T>
constexpr I16 to_I16(const T value) {
    return static_cast<I16>(value);
}

template <typename T>
constexpr I8 to_I8(const T value) {
    return static_cast<I8>(value);
}
template <typename T>
constexpr F32 to_F32(const T value) {
    return static_cast<F32>(value);
}

template <typename T>
constexpr D64 to_D64(const T value) {
    return static_cast<D64>(value);
}

template<typename T>
constexpr D128 to_D128(const T value) {
    return static_cast<D128>(value);
}

template<typename T>
constexpr char to_byte(const T value) {
    return static_cast<char>(value);
}

//ref: http://codereview.stackexchange.com/questions/51235/udp-network-server-client-for-gaming-using-boost-asio
class counter {
    size_t count;
public:
    counter &operator=(size_t val) { count = val; return *this; }
    counter(size_t count = 0) noexcept : count(count) {}
    operator size_t() { return count; }
    counter &operator++() { ++count; return *this; }
    counter operator++(int) { counter ret(count); ++count; return ret; }
    bool operator==(counter const &other) { return count == other.count; }
    bool operator!=(counter const &other) { return count != other.count; }
};


// Type promotion
// ref: https://janmr.com/blog/2010/08/cpp-templates-usual-arithmetic-conversions/

template <typename T>
struct promote {
    typedef T type;
};

template <>
struct promote<signed short>
{
    typedef I32 type;
};

template <>
struct promote<bool>
{
    typedef I32 type;
};

template <bool C, typename T, typename F>
struct choose_type
{
    typedef F type;
};

template <typename T, typename F>
struct choose_type<true, T, F>
{
    typedef T type;
};

template <>
struct promote<unsigned short>
{
    typedef choose_type<sizeof(short) < sizeof(I32), I32, U32>::type type;
};

template <>
struct promote<signed char>
{
    typedef choose_type<sizeof(char) <= sizeof(I32), I32, U32>::type type;
};

template <>
struct promote<unsigned char>
{
    typedef choose_type<sizeof(char) < sizeof(I32), I32, U32>::type type;
};

template <>
struct promote<char>
    : public promote<choose_type<std::numeric_limits<char>::is_signed,
    signed char, unsigned char>::type>
{
};

template <>
struct promote<wchar_t>
{
    typedef choose_type<
        std::numeric_limits<wchar_t>::is_signed,
        choose_type<sizeof(wchar_t) <= sizeof(I32), I32, long>::type,
        choose_type<sizeof(wchar_t) <= sizeof(I32), U32, unsigned long>::type
    >::type type;
};

template <typename T> struct type_rank;
template <> struct type_rank<I32> { static const I32 rank = 1; };
template <> struct type_rank<U32> { static const I32 rank = 2; };
template <> struct type_rank<long> { static const I32 rank = 3; };
template <> struct type_rank<unsigned long> { static const I32 rank = 4; };
template <> struct type_rank<I64> { static const I32 rank = 5; };
template <> struct type_rank<U64> { static const I32 rank = 6; };
template <> struct type_rank<F32> { static const I32 rank = 7; };
template <> struct type_rank<D64> { static const I32 rank = 8; };
template <> struct type_rank<D128> { static const int rank = 9; };

template <typename A, typename B>
struct resolve_uac2
{
    typedef typename choose_type<
        type_rank<A>::rank >= type_rank<B>::rank, A, B
    >::type return_type;
};

template <>
struct resolve_uac2<long, U32>
{
    typedef choose_type<sizeof(long) == sizeof(U32),
        unsigned long, long>::type return_type;
};

template <>
struct resolve_uac2<U32, long> : public resolve_uac2<long, U32>
{
};

template <typename A, typename B>
struct resolve_uac : public resolve_uac2<typename promote<A>::type,
    typename promote<B>::type>
{
};

template <typename A, typename B>
constexpr typename resolve_uac<A, B>::return_type add(const A& a, const B& b) noexcept
{
    return a + b;
}


template <typename A, typename B>
constexpr typename resolve_uac<A, B>::return_type substract(const A& a, const B& b) noexcept
{
    return a - b;
}


template <typename A, typename B>
constexpr typename resolve_uac<A, B>::return_type divide(const A& a, const B& b) noexcept
{
    return a / b;
}


template <typename A, typename B>
constexpr typename resolve_uac<A, B>::return_type multiply(const A& a, const B& b) noexcept
{
    return a * b;
}

template <typename ToCheck, std::size_t ExpectedSize, std::size_t RealSize = sizeof(ToCheck)>
constexpr void check_size() {
    static_assert(ExpectedSize == RealSize, "Wrong data size!");
}

}; //namespace Divide;

#endif //_PLATFORM_DATA_TYPES_H_
