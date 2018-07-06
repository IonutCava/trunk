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

#ifndef _PLATFORM_DATA_TYPES_H_
#define _PLATFORM_DATA_TYPES_H_

#include <cstdint>
#include <type_traits>

namespace Divide {
// Data Types (double is 8 bytes with Microsoft's compiler)
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;
typedef float F32;
typedef double D64;
typedef void* bufferPtr;

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

/*
template<typename Enum>
constexpr U32 operator"" _u32 ( Enum value )
{
return static_cast<U32>(value);
}*/

template <typename Type, typename = void>
constexpr auto to_underlying_type(const Type value) -> Type {
    return value;
}

template <typename Type, typename std::enable_if<std::is_enum<Type>::value>::type>
constexpr auto to_underlying_type(const Type value) -> typename std::underlying_type<Type>::type {
    return static_cast<typename std::underlying_type<Type>::type>(value);
}

template<typename T>
constexpr U8 to_const_ubyte(const T value) {
    return static_cast<U8>(value);
}

template<typename T>
constexpr U16 to_const_ushort(const T value) {
    return static_cast<U16>(value);
}

template<typename T>
constexpr U32 to_const_uint(const T value) {
    return static_cast<U32>(value);
}

template<typename T>
constexpr I8 to_const_byte(const T value) {
    return static_cast<I8>(value);
}

template<typename T>
constexpr I16 to_const_short(const T value) {
    return static_cast<I16>(value);
}
template<typename T>
constexpr I32 to_const_int(const T value) {
    return static_cast<I32>(value);
}

template<typename T>
constexpr F32 to_const_float(const T value) {
    return static_cast<F32>(value);
}

template<typename T>
constexpr D64 to_const_double(const T value) {
    return static_cast<D64>(value);
}

template <typename T>
U32 to_uint(const T value) {
    return static_cast<U32>(to_underlying_type(value));
}

template <typename T>
U16 to_ushort(const T value) {
    return static_cast<U16>(to_underlying_type(value));
}

template <typename T>
U8 to_ubyte(const T value) {
    return static_cast<U8>(to_underlying_type(value));
}

template <typename T>
I32 to_int(const T value) {
    return static_cast<I32>(to_underlying_type(value));
}

template <typename T>
I16 to_short(const T value) {
    return static_cast<I16>(to_underlying_type(value));
}

template <typename T>
I8 to_byte(const T value) {
    return static_cast<I8>(to_underlying_type(value));
}
template <typename T>
F32 to_float(const T value) {
    return static_cast<F32>(to_underlying_type(value));
}

template <typename T>
D64 to_double(const T value) {
    return static_cast<D64>(to_underlying_type(value));
}

}; //namespace Divide;

#endif //_PLATFORM_DATA_TYPES_H_
