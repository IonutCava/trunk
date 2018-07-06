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

#ifndef _PLATFORM_DATA_TYPES_H_
#define _PLATFORM_DATA_TYPES_H_

#include <cstdint>
#include <type_traits>

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

typedef void* bufferPtr;

typedef union {
    U32 i;
    U8  b[4];
} P32;

typedef union {
    U64 i;
    U8  b[8];
} P64;

enum class CallbackParam : U32 {
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
    TYPE_CHAR,
    TYPE_BOOL,
    TYPE_VOID
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
constexpr auto to_base(const Type value) -> typename std::underlying_type<Type>::type {
    return static_cast<typename std::underlying_type<Type>::type>(value);
}

template <typename T>
U32 to_U32(const T value) {
    return static_cast<U32>(value);
}

template <typename T>
U16 to_U16(const T value) {
    return static_cast<U16>(value);
}

template <typename T>
U8 to_U8(const T value) {
    return static_cast<U8>(value);
}

template <typename T>
I32 to_I32(const T value) {
    return static_cast<I32>(value);
}

template <typename T>
I16 to_I16(const T value) {
    return static_cast<I16>(value);
}

template <typename T>
I8 to_I8(const T value) {
    return static_cast<I8>(value);
}
template <typename T>
F32 to_F32(const T value) {
    return static_cast<F32>(value);
}

template <typename T>
D64 to_D64(const T value) {
    return static_cast<D64>(value);
}

//ref: http://codereview.stackexchange.com/questions/51235/udp-network-server-client-for-gaming-using-boost-asio
class counter {
    size_t count;
public:
    counter &operator=(size_t val) { count = val; return *this; }
    counter(size_t count = 0) : count(count) {}
    operator size_t() { return count; }
    counter &operator++() { ++count; return *this; }
    counter operator++(int) { counter ret(count); ++count; return ret; }
    bool operator==(counter const &other) { return count == other.count; }
    bool operator!=(counter const &other) { return count != other.count; }
};

}; //namespace Divide;

#endif //_PLATFORM_DATA_TYPES_H_
