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

#ifndef _PLATFORM_DEFINES_H_
#define _PLATFORM_DEFINES_H_

#include "Core/TemplateLibraries/Headers/Vector.h"
#include "Core/TemplateLibraries/Headers/String.h"
#include "Core/TemplateLibraries/Headers/HashMap.h"
#include "Core/Headers/Singleton.h"
#include "Core/Headers/NonCopyable.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif //_USE_MATH_DEFINES
#include <cmath>
#include <functional>
#include <atomic>
#include <type_traits>
#include <array>
#include <memory>
#include <bitset>
#include <limits>
#include <stdarg.h>
#include <cmath>
#include <cstdio>

namespace Divide {
/// Data Types
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;
typedef float F32;
typedef double D32;
typedef void* bufferPtr;
}; //namespace Divide

#if defined(_WIN32)
#include "PlatformDefinesWindows.h"
#elif defined(__APPLE_CC__) 
#include "PlatformDefinesApple.h"
#else //defined(__linux) || defined (__unix)
#include "PlatformDefinesUnix.h"
#endif

#ifdef _DEBUG
#define STUBBED(x)                                                   \
    \
do {                                                                 \
        static bool seen_this = false;                               \
        if (!seen_this) {                                            \
            seen_this = true;                                        \
            Console::errorfn("STUBBED: %s (%s : %d)\n", x, __FILE__, \
                             __LINE__);                              \
        }                                                            \
    \
}                                                             \
    while (0)                                                        \
        ;

#else
#define STUBBED(x)
#endif

namespace std {
    template<typename T, size_t N>
    struct hash<array<T, N> >
    {
        typedef array<T, N> argument_type;
        typedef size_t result_type;

        result_type operator()(const argument_type& a) const
        {
            hash<T> hasher;
            result_type h = 0;
            for (result_type i = 0; i < N; ++i)
            {
                h = h * 31 + hasher(a[i]);
            }
            return h;
        }
    };
};

namespace Divide {

/// Data Types
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;
typedef float F32;
typedef double D32;
typedef void* bufferPtr;

/*
template<typename Enum>
constexpr U32 operator"" _u32 ( Enum value )
{
    return static_cast<U32>(value);
}*/

template <typename Type, typename = void>
/*constexpr*/ auto to_underlying_type(const Type value) -> Type {
    return value;
}

template <typename Type, typename std::enable_if<std::is_enum<Type>::value>::type>
/*constexpr*/ auto to_underlying_type(const Type value) ->
    typename std::underlying_type<Type>::type {
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
constexpr D32 to_const_double(const T value) {
    return static_cast<D32>(value);
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
D32 to_double(const T value) {
    return static_cast<D32>(to_underlying_type(value));
}

struct SysInfo;

extern void getWindowHandle(void* window, SysInfo& info);

bool CheckMemory(const U32 physicalRAMNeeded, SysInfo& info);
/// Converts an arbitrary positive integer value to a bitwise value used for masks
template<typename T>
constexpr T toBit(const T X) {
    //static_assert(X > 0, "toBit(0) is currently disabled!");
    return 1 << X;
}

inline U32 powerOfTwo(U32 X) {
    U32 r = 0;
    while (X >>= 1) {
        r++;
    }
    return r;
}

template<typename T>
T to_bitwise(T X) {
    DIVIDE_ASSERT(X > 0, "to_bitwise(0) is currently disabled!");
    return 1 << X;
}

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<typename T >
std::unique_ptr<T> copy_unique(const std::unique_ptr<T>& source)
{
    return source ? make_unique<T>(*source) : nullptr;
}


/* See

http://randomascii.wordpress.com/2012/01/11/tricks-with-the-floating-point-format/

for the potential portability problems with the union and bit-fields below.
*/
union Float_t {
    Float_t(F32 num = 0.0f) : f(num) {}
    // Portable extraction of components.
    bool Negative() const { return (i >> 31) != 0; }
    I32 RawMantissa() const { return i & ((1 << 23) - 1); }
    I32 RawExponent() const { return (i >> 23) & 0xFF; }

    I32 i;
    F32 f;
};

union Double_t {
    Double_t(D32 num = 0.0) : d(num) {}
    // Portable extraction of components.
    bool Negative() const { return (i >> 63) != 0; }
    I64 RawMantissa() const { return i & ((1LL << 52) - 1); }
    I64 RawExponent() const { return (i >> 52) & 0x7FF; }

    I64 i;
    D32 d;
};

inline bool AlmostEqualUlpsAndAbs(F32 A, F32 B, F32 maxDiff, I32 maxUlpsDiff) {
    // Check if the numbers are really close -- needed
    // when comparing numbers near zero.
    F32 absDiff = std::fabs(A - B);
    if (absDiff <= maxDiff) {
        return true;
    }

    Float_t uA(A);
    Float_t uB(B);

    // Different signs means they do not match.
    if (uA.Negative() != uB.Negative()) return false;

    // Find the difference in ULPs.
    return (std::abs(uA.i - uB.i) <= maxUlpsDiff);
}

inline bool AlmostEqualUlpsAndAbs(D32 A, D32 B, D32 maxDiff, I32 maxUlpsDiff) {
    // Check if the numbers are really close -- needed
    // when comparing numbers near zero.
    D32 absDiff = std::fabs(A - B);
    if (absDiff <= maxDiff) return true;

    Double_t uA(A);
    Double_t uB(B);

    // Different signs means they do not match.
    if (uA.Negative() != uB.Negative()) return false;

    // Find the difference in ULPs.
    return (std::abs(uA.i - uB.i) <= maxUlpsDiff);
}

inline bool AlmostEqualRelativeAndAbs(F32 A, F32 B, F32 maxDiff,
                                      F32 maxRelDiff) {
    // Check if the numbers are really close -- needed
    // when comparing numbers near zero.
    F32 diff = std::fabs(A - B);
    if (diff <= maxDiff) return true;

    A = std::fabs(A);
    B = std::fabs(B);
    F32 largest = (B > A) ? B : A;

    return (diff <= largest * maxRelDiff);
}

inline bool AlmostEqualRelativeAndAbs(D32 A, D32 B, D32 maxDiff,
                                      D32 maxRelDiff) {
    // Check if the numbers are really close -- needed
    // when comparing numbers near zero.
    D32 diff = std::fabs(A - B);
    if (diff <= maxDiff) return true;

    A = std::fabs(A);
    B = std::fabs(B);
    D32 largest = (B > A) ? B : A;

    return (diff <= largest * maxRelDiff);
}

#define ACKNOWLEDGE_UNUSED(p) ((void)p)

#define WAIT_FOR_CONDITION(p) {while (!(p)) {}}

static const F32 EPSILON_F32 = std::numeric_limits<F32>::epsilon();
static const D32 EPSILON_D32 = std::numeric_limits<D32>::epsilon();

template <typename T>
inline bool IS_VALID_CONTAINER_RANGE(T elementCount, T min, T max) {
    return min >= 0 && max < elementCount;
}
template <typename T>
inline bool IS_IN_RANGE_INCLUSIVE(T x, T min, T max) {
    return x >= min && x <= max;
}
template <typename T>
inline bool IS_IN_RANGE_EXCLUSIVE(T x, T min, T max) {
    return x > min && x < max;
}
template <typename T>
inline bool IS_ZERO(T X) {
    return X == 0;
}
template <>
inline bool IS_ZERO(F32 X) {
    return (std::fabs(X) < EPSILON_F32);
}
template <>
inline bool IS_ZERO(D32 X) {
    return (std::fabs(X) < EPSILON_D32);
}

template <typename T>
inline bool IS_TOLERANCE(T X, T TOLERANCE) {
    return (std::abs(X) <= TOLERANCE);
}
template <>
inline bool IS_TOLERANCE(F32 X, F32 TOLERANCE) {
    return (std::fabs(X) <= TOLERANCE);
}
template <>
inline bool IS_TOLERANCE(D32 X, D32 TOLERANCE) {
    return (std::fabs(X) <= TOLERANCE);
}

template<typename T>
inline bool COMPARE_TOLERANCE(T X, T Y, T TOLERANCE) {
    return std::fabs(X - Y) <= TOLERANCE;
}

template<>
inline bool COMPARE_TOLERANCE(F32 X, F32 Y, F32 TOLERANCE) {
    return AlmostEqualUlpsAndAbs(X, Y, TOLERANCE, 4);
}

template<>
inline bool COMPARE_TOLERANCE(D32 X, D32 Y, D32 TOLERANCE) {
    return AlmostEqualUlpsAndAbs(X, Y, TOLERANCE, 4);
}

template<typename T>
inline bool COMPARE(T X, T Y) {
    return X == Y;
}

template<>
inline bool COMPARE(F32 X, F32 Y) {
    return COMPARE_TOLERANCE(X, Y, EPSILON_F32);
}

template<>
inline bool COMPARE(D32 X, D32 Y) {
    return COMPARE_TOLERANCE(X, Y, EPSILON_D32);
}

/// Performes extra asserts steps (logging, message boxes, etc). 
/// Returns true if the assert should be processed.
bool preAssert(const bool expression, const char* failMessage);
/// It is safe to call evaluate expressions and call functions inside the assert
/// check as it will compile for every build type
inline bool DIVIDE_ASSERT(const bool expression, const char* failMessage) {
#if defined(_DEBUG) || defined(_PROFILE)
    if (preAssert(expression, failMessage)) {
#   if defined(_DEBUG)
        assert(expression && failMessage);
#   endif
    }
#else
    ACKNOWLEDGE_UNUSED(failMessage);
#endif
    return expression;
}

typedef union {
    U32 i;
    U8  b[4];
} P32;

template <typename... Args>
auto DELEGATE_BIND(Args&&... args)
    -> decltype(std::bind(std::forward<Args>(args)...)) {
    return std::bind(std::forward<Args>(args)...);
}

template <typename... Args>
auto DELEGATE_REF(Args&&... args)
    -> decltype(std::bind(std::forward<Args>(args)...)) {
    return std::bind(std::forward<Args>(args)...);
}

template <typename... Args>
auto DELEGATE_CREF(Args&&... args)
    -> decltype(std::cref(std::forward<Args>(args)...)) {
    return std::cref(std::forward<Args>(args)...);
}

template <typename T = void>
using DELEGATE_CBK = std::function<T()>;

template <typename P, typename T = void>
using DELEGATE_CBK_PARAM = std::function < T(P param) > ;

template <typename P1, typename P2, typename T = void>
using DELEGATE_CBK_PARAM_2 = std::function < T(P1 param1, P2 param2) >;

U32 HARDWARE_THREAD_COUNT();

};  // namespace Divide

void* malloc_aligned(const size_t size, size_t alignment);
void  malloc_free(void*& ptr);

void* operator new[](size_t size, const char* pName, Divide::I32 flags,
                     Divide::U32 debugFlags, const char* file,
                     Divide::I32 line);

void operator delete[](void* ptr, const char* pName, Divide::I32 flags,
                       Divide::U32 debugFlags, const char* file,
                       Divide::I32 line);

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                     const char* pName, Divide::I32 flags,
                     Divide::U32 debugFlags, const char* file,
                     Divide::I32 line);

void operator delete[](void* ptr, size_t alignment, size_t alignmentOffset,
                       const char* pName, Divide::I32 flags,
                       Divide::U32 debugFlags, const char* file,
                       Divide::I32 line);

// EASTL also wants us to define this (see string.h line 197)
Divide::I32 Vsnprintf8(char* pDestination, size_t n, const char* pFormat,
                       va_list arguments);

#if !defined(_DEBUG)
#define MemoryManager_NEW new
#else
void* operator new(size_t size);
void operator delete(void* p) noexcept;
void* operator new[](size_t size);
void operator delete[](void* p) noexcept;

void* operator new(size_t size, const char* zFile, Divide::I32 nLine);
void operator delete(void* ptr, const char* zFile, Divide::I32 nLine);
void* operator new[](size_t size, const char* zFile, Divide::I32 nLine);
void operator delete[](void* ptr, const char* zFile, Divide::I32 nLine);

#define MemoryManager_NEW new (__FILE__, __LINE__)
#endif

namespace Divide {
namespace MemoryManager {

template <typename T>
inline void SAFE_FREE(T*& ptr) {
    if (ptr) {
        free(ptr);
        ptr = nullptr;
    }
}

/// Deletes and nullifies the specified pointer
template <typename T>
inline void DELETE(T*& ptr) {
    delete ptr;
    ptr = nullptr;
}

#define SET_SHARED_PTR_DELETE_FRIEND(X) \
    friend void DeletelFunc(X *);
    

#define SET_UNIQUE_PTR_DELETE_FRIEND(X) \
    friend std::unique_ptr<X>::deleter_type;

#define SET_DELETE_FRIEND \
    template <typename T> \
    friend void MemoryManager::DELETE(T*& ptr);

/// Deletes and nullifies the specified pointer
template <typename T>
inline void SAFE_DELETE(T*& ptr) {
    DIVIDE_ASSERT(ptr != nullptr, "SAFE_DELETE: null pointer received");

    delete ptr;
    ptr = nullptr;
}
#define SET_SAFE_DELETE_FRIEND \
    template <typename T>      \
    friend void MemoryManager::DELETE(T*& ptr);

/// Deletes and nullifies the specified array pointer
template <typename T>
inline void DELETE_ARRAY(T*& ptr) {
    delete[] ptr;
    ptr = nullptr;
}
#define SET_DELETE_ARRAY_FRIEND \
    template <typename T>       \
    friend void MemoryManager::DELETE_ARRAY(T*& ptr);

/// Deletes and nullifies the specified array pointer
template <typename T>
inline void SAFE_DELETE_ARRAY(T*& ptr) {
    DIVIDE_ASSERT(ptr != nullptr, "SAFE_DELETE_ARRAY: null pointer received");

    delete[] ptr;
    ptr = nullptr;
}
#define SET_SAFE_DELETE_ARRAY_FRIEND \
    template <typename T>            \
    friend void MemoryManager::DELETE_ARRAY(T*& ptr);

/// Deletes and nullifies the specified pointer. Returns "false" if the pointer
/// was already null
template <typename T>
inline bool DELETE_CHECK(T*& ptr) {
    if (ptr == nullptr) {
        return false;
    }

    delete ptr;
    ptr = nullptr;

    return true;
}
#define SET_DELETE_CHECK_FRIEND \
    template <typename T>       \
    friend void MemoryManager::DELETE_CHECK(T*& ptr);

/// Deletes and nullifies the specified array pointer. Returns "false" if the
/// pointer was already null
template <typename T>
inline bool DELETE_ARRAY_CHECK(T*& ptr) {
    if (ptr == nullptr) {
        return false;
    }

    delete[] ptr;
    ptr = nullptr;

    return true;
}

#define SET_DELETE_ARRAY_CHECK_FRIEND \
    template <typename T>             \
    friend void MemoryManager::DELETE_ARRAY_CHECK(T*& ptr);
/// Deletes every element from the vector and clears it at the end
template <typename T>
inline void DELETE_VECTOR(vectorImpl<T*>& vec) {
    if (!vec.empty()) {
        for (T* iter : vec) {
            delete iter;
        }
        vec.clear();
    }
}
#define SET_DELETE_VECTOR_FRIEND \
    template <typename T>        \
    friend void MemoryManager::DELETE_VECTOR(vectorImpl<T*>& vec);

/// Deletes every element from the map and clears it at the end
template <typename K, typename V, typename HashFun = hashAlg::hash<K> >
inline void DELETE_HASHMAP(hashMapImpl<K, V, HashFun>& map) {
    if (!map.empty()) {
        for (typename hashMapImpl<K, V, HashFun>::value_type iter : map) {
            delete iter.second;
        }
        map.clear();
    }
}
#define SET_DELETE_HASHMAP_FRIEND                                           \
    template <typename K, typename V, typename HashFun = hashAlg::hash<K> > \
    friend void MemoryManager::DELETE_HASHMAP(                              \
        hashMapImpl<K, V, HashFun>& map);

/// Deletes the object pointed to by "OLD" and redirects that pointer to the
/// object pointed by "NEW"
/// "NEW" must be a derived (or same) class of OLD
template <typename Base, typename Derived>
inline void SAFE_UPDATE(Base*& OLD, Derived* const NEW) {
    static_assert(std::is_base_of<Base, Derived>::value,
                  "SAFE_UPDATE error: New must be a descendant of Old");

    delete OLD;
    OLD = NEW;
}
#define SET_SAFE_UPDATE_FRIEND                 \
    template <typename Base, typename Derived> \
    friend void MemoryManager::SAFE_UPDATE(Base*& OLD, Derived* const NEW);

};  // namespace MemoryManager
};  // namespace Divide

#endif

