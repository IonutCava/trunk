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

#ifndef _PLATFORM_DEFINES_H_
#define _PLATFORM_DEFINES_H_

#ifndef BOOST_EXCEPTION_DISABLE
#define BOOST_EXCEPTION_DISABLE
#endif

#include "Core/TemplateLibraries/Headers/Vector.h"
#include "Core/TemplateLibraries/Headers/String.h"
#include "Core/TemplateLibraries/Headers/HashMap.h"
#include "Core/Headers/Singleton.h"
#include "Core/Headers/NonCopyable.h"
#include "Core/Headers/GUIDWrapper.h"
#include "Platform/Threading/Headers/SharedMutex.h"

#include "ConditionalWait.h"

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
/// Data Types (double is 8 bytes with Microsoft's compiler)
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
typedef unsigned long long ULL;

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

}; //namespace Divide

#if defined(_WIN32)
#include "PlatformDefinesWindows.h"
#elif defined(__APPLE_CC__) 
#include "PlatformDefinesApple.h"
#else //defined(__linux) || defined (__unix)
#include "PlatformDefinesUnix.h"
#endif

#ifdef _DEBUG
#include <assert.h>

#define STUBBED(x)                                  \
do {                                                \
    static bool seen_this = false;                  \
    if (!seen_this) {                               \
        seen_this = true;                           \
        Console::errorfn("STUBBED: %s (%s : %d)\n", \
                         x, __FILE__, __LINE__);    \
    }                                               \
} while (0);

#else
#define STUBBED(x)
#endif

#ifndef TO_STRING
#define TO_STRING(X) TO_STRING_NAME(X)
#define TO_STRING_NAME(X) #X
#endif //TO_STRING

#define TYPEDEF_SMART_POINTERS_FOR_CLASS(T)      \
    typedef std::weak_ptr<T> T ## _wptr;         \
    typedef std::shared_ptr<T> T ## _ptr;        \
    typedef std::weak_ptr<const T> T ## _cwptr;  \
    typedef std::shared_ptr<const T> T ## _cptr; 

#define FWD_DECLARE_MANAGED_CLASS(T)      \
    class T;                              \
    TYPEDEF_SMART_POINTERS_FOR_CLASS(T);

namespace Divide {

static constexpr ULL basis = 14695981039346656037ULL;
static constexpr ULL prime = 1099511628211ULL;

constexpr ULL hash_one(char c, const char* remain, ULL value)
{
    return c == 0 ? value : hash_one(remain[0], remain + 1, (value ^ c) * prime);
}

constexpr ULL _ID(const char* str)
{
    return hash_one(str[0], str + 1, basis);
}

FORCE_INLINE ULL _ID_RT(const char* str)
{
    ULL hash = basis;
    while (*str != 0) {
        hash ^= str[0];
        hash *= prime;
        ++str;
    }
    return hash;
}

FORCE_INLINE ULL _ID_RT(const stringImpl& str) {
    return _ID_RT(str.c_str());
}

FORCE_INLINE bufferPtr bufferOffset(size_t offset) {
    return ((char *)NULL + (offset));
}

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

struct SysInfo;

extern void getWindowHandle(void* window, SysInfo& info);

extern void setThreadName(std::thread* thread, const char* threadName);
extern void setThreadName(const char* threadName);

bool PlatformInitStatic();
bool PlatformInit();
bool PlatformClose();
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

inline bool isPowerOfTwo(U32 x) {
    return !(x == 0) && !(x & (x - 1));
}

static inline size_t realign_offset(size_t offset, size_t align) {
    return (offset + align - 1) & ~(align - 1);
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

//ref: http://stackoverflow.com/questions/14226952/partitioning-batch-chunk-a-container-into-equal-sized-pieces-using-std-algorithm
template<typename Iterator>
void for_each_interval(Iterator from, Iterator to, std::ptrdiff_t partition_size,
                       std::function<void(Iterator, Iterator)> operation) 
{
    if (partition_size > 0) {
        Iterator partition_end = from;
        while (partition_end != to) 
        {
            while (partition_end != to &&
                   std::distance(from, partition_end) < partition_size) 
            {
                ++partition_end;
            }
            operation(from, partition_end);
            from = partition_end;
        }
    }
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
    Double_t(D64 num = 0.0) : d(num) {}
    // Portable extraction of components.
    bool Negative() const { return (i >> 63) != 0; }
    I64 RawMantissa() const { return i & ((1LL << 52) - 1); }
    I64 RawExponent() const { return (i >> 52) & 0x7FF; }

    I64 i;
    D64 d;
};

inline bool AlmostEqualUlpsAndAbs(F32 A, F32 B, F32 maxDiff, I32 maxUlpsDiff) {
    // Check if the numbers are really close -- needed when comparing numbers near zero.
    F32 absDiff = std::fabs(A - B);
    if (absDiff <= maxDiff) {
        return true;
    }

    Float_t uA(A);
    Float_t uB(B);

    // Different signs means they do not match.
    if (uA.Negative() != uB.Negative()) {
        return false;
    }

    // Find the difference in ULPs.
    return (std::abs(uA.i - uB.i) <= maxUlpsDiff);
}

inline bool AlmostEqualUlpsAndAbs(D64 A, D64 B, D64 maxDiff, I32 maxUlpsDiff) {
    // Check if the numbers are really close -- needed when comparing numbers near zero.
    D64 absDiff = std::fabs(A - B);
    if (absDiff <= maxDiff) {
        return true;
    }

    Double_t uA(A);
    Double_t uB(B);

    // Different signs means they do not match.
    if (uA.Negative() != uB.Negative()) {
        return false;
    }

    // Find the difference in ULPs.
    return (std::abs(uA.i - uB.i) <= maxUlpsDiff);
}

inline bool AlmostEqualRelativeAndAbs(F32 A, F32 B, F32 maxDiff, F32 maxRelDiff) {
    // Check if the numbers are really close -- needed when comparing numbers near zero.
    F32 diff = std::fabs(A - B);
    if (diff <= maxDiff) {
        return true;
    }

    A = std::fabs(A);
    B = std::fabs(B);
    F32 largest = (B > A) ? B : A;

    return (diff <= largest * maxRelDiff);
}

inline bool AlmostEqualRelativeAndAbs(D64 A, D64 B, D64 maxDiff, D64 maxRelDiff) {
    // Check if the numbers are really close -- needed when comparing numbers near zero.
    D64 diff = std::fabs(A - B);
    if (diff <= maxDiff) {
        return true;
    }

    A = std::fabs(A);
    B = std::fabs(B);
    D64 largest = (B > A) ? B : A;

    return (diff <= largest * maxRelDiff);
}

#define ACKNOWLEDGE_UNUSED(p) ((void)p)

#if defined(_MSC_VER)
#define _FUNCTION_NAME_AND_SIG_ __FUNCSIG__
#elif defined(__GNUC__)
#define _FUNCTION_NAME_AND_SIG_ __PRETTY_FUNCTION__
#else
#define _FUNCTION_NAME_AND_SIG_ __FUNCTION__
#endif

static const F32 EPSILON_F32 = std::numeric_limits<F32>::epsilon();
static const D64 EPSILON_D64 = std::numeric_limits<D64>::epsilon();

template <typename T>
inline bool IS_VALID_CONTAINER_RANGE(T elementCount, T min, T max) {
    return min >= 0 && max < elementCount;
}
template <typename T, typename U>
inline bool IS_IN_RANGE_INCLUSIVE(T x, U min, U max) {
    return x >= min && x <= max;
}
template <typename T, typename U>
inline bool IS_IN_RANGE_EXCLUSIVE(T x, U min, U max) {
    return x > min && x < max;
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
inline bool IS_ZERO(D64 X) {
    return (std::fabs(X) < EPSILON_D64);
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
inline bool IS_TOLERANCE(D64 X, D64 TOLERANCE) {
    return (std::fabs(X) <= TOLERANCE);
}

template<typename T, typename U>
inline bool COMPARE_TOLERANCE(T X, U Y, T TOLERANCE) {
    return COMPARE_TOLERANCE(X, static_cast<T>(Y), TOLERANCE);
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
inline bool COMPARE_TOLERANCE(D64 X, D64 Y, D64 TOLERANCE) {
    return AlmostEqualUlpsAndAbs(X, Y, TOLERANCE, 4);
}

template<typename T, typename U>
inline bool COMPARE(T X, U Y) {
    return COMPARE(X, static_cast<T>(Y));
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
inline bool COMPARE(D64 X, D64 Y) {
    return COMPARE_TOLERANCE(X, Y, EPSILON_D64);
}

template <typename T>
inline bool IS_GEQUAL(T X, T Y) {
    return X >= Y;
}

template <>
/// should be fast enough as the first condition is almost always true
inline bool IS_GEQUAL(F32 X, F32 Y) {
    return X > Y || COMPARE(X, Y);
}

template <>
inline bool IS_GEQUAL(D64 X, D64 Y) {
    return X > Y || COMPARE(X, Y);
}

template <typename T>
inline bool IS_LEQUAL(T X, T Y) {
    return X <= Y;
}

template <>
inline bool IS_LEQUAL(F32 X, F32 Y) {
    return X < Y || COMPARE(X, Y);
}

template <>
inline bool IS_LEQUAL(D64 X, D64 Y) {
    return X < Y || COMPARE(X, Y);
}

///ref: http://blog.molecular-matters.com/2011/08/12/a-safer-static_cast/#more-120
// base template
template <bool IsFromSigned, bool IsToSigned>
struct safe_static_cast_helper;

// template specialization for casting from an unsigned type into an unsigned type
template <>
struct safe_static_cast_helper<false, false>
{
    template <typename TO, typename FROM>
    static inline TO cast(FROM from)
    {
        assert(IS_IN_RANGE_INCLUSIVE(std::is_enum<FROM>::value 
                                         ? static_cast<U32>(to_underlying_type(from))
                                         : from,
                                     std::numeric_limits<TO>::min(),
                                     std::numeric_limits<TO>::max()) &&
            "Number to cast exceeds numeric limits.");

        return static_cast<TO>(from);
    }
};

// template specialization for casting from an unsigned type into a signed type
template <>
struct safe_static_cast_helper<false, true>
{
    template <typename TO, typename FROM>
    static inline TO cast(FROM from)
    {
        assert(IS_IN_RANGE_INCLUSIVE(from,
                                     std::numeric_limits<TO>::min(),
                                     std::numeric_limits<TO>::max()) &&
            "Number to cast exceeds numeric limits.");

        return static_cast<TO>(from);
    }
};

// template specialization for casting from a signed type into an unsigned type
template <>
struct safe_static_cast_helper<true, false>
{
    template <typename TO, typename FROM>
    static inline TO cast(FROM from)
    {
        // make sure the input is not negative
        assert(from >= 0 && "Number to cast exceeds numeric limits.");

        // assuring a positive input, we can safely cast it into its unsigned type and check the numeric limits
        typedef typename std::make_unsigned<FROM>::type UnsignedFrom;
        assert(IS_IN_RANGE_INCLUSIVE(static_cast<UnsignedFrom>(from),
                                     std::numeric_limits<TO>::min(),
                                     std::numeric_limits<TO>::max()) &&
            "Number to cast exceeds numeric limits.");
        return static_cast<TO>(from);
    }
};

// template specialization for casting from a signed type into a signed type
template <>
struct safe_static_cast_helper<true, true>
{
    template <typename TO, typename FROM>
    static inline TO cast(FROM from)
    {
        assert(IS_IN_RANGE_INCLUSIVE(std::is_enum<FROM>::value ? to_underlying_type(from) : from,
                                     std::numeric_limits<TO>::min(),
                                     std::numeric_limits<TO>::max()) &&
            "Number to cast exceeds numeric limits.");

        return static_cast<TO>(from);
    }
};

#if 0
template <typename TO, typename FROM>
inline TO safe_static_cast(FROM from)
{
/*#if defined(_DEBUG)
    // delegate the call to the proper helper class, depending on the signedness of both types
    return safe_static_cast_helper<std::numeric_limits<FROM>::is_signed,
                                   std::numeric_limits<TO>::is_signed>
           ::cast<TO>(from);
#else*/
    return static_cast<TO>(from);
//#endif
}

template <typename TO>
inline TO safe_static_cast(F32 from)
{
    return static_cast<TO>(from);
}

template <typename TO>
inline TO safe_static_cast(D64 from)
{
    return static_cast<TO>(from);
} 
#endif
/// Performes extra asserts steps (logging, message boxes, etc). 
/// Returns true if the assert should be processed.
bool preAssert(const bool expression, const char* failMessage);
/// It is safe to call evaluate expressions and call functions inside the assert
/// check as it will compile for every build type
FORCE_INLINE bool DIVIDE_ASSERT(const bool expression, const char* failMessage) {
    if (!Config::Build::IS_RELEASE_BUILD) {
        if (preAssert(expression, failMessage)) {
            if (Config::Build::IS_DEBUG_BUILD) {
                assert(expression && failMessage);
            }
        }
    } else {
        ACKNOWLEDGE_UNUSED(failMessage);
    }
    return expression;
}

FORCE_INLINE void DIVIDE_UNEXPECTED_CALL(const char* failMessage = "") {
    DIVIDE_ASSERT(false, failMessage);
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
    -> decltype(std::ref(std::forward<Args>(args)...)) {
    return std::ref(std::forward<Args>(args)...);
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

template <typename P1, typename P2, typename P3, typename T = void>
using DELEGATE_CBK_PARAM_3 = std::function < T(P1 param1, P2 param2, P3 param3) >;

template <typename P1, typename P2, typename P3, typename P4, typename T = void>
using DELEGATE_CBK_PARAM_4 = std::function < T(P1 param1, P2 param2, P3 param3, P4 param4) >;

class GUID_DELEGATE_CBK : public GUIDWrapper {
  public:
    GUID_DELEGATE_CBK(const DELEGATE_CBK<>& cbk)
        : GUIDWrapper(),
          _callback(cbk)
    {
    }

    DELEGATE_CBK<> _callback;
};

U32 HARDWARE_THREAD_COUNT();

template<typename T, typename U>
constexpr void assert_type(const U& value) {
    static_assert(std::is_same<U, T>::value, "value type not satisfied");
}

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
#if defined(DEBUG_EXTERNAL_ALLOCATIONS)
void* operator new(size_t size);
void operator delete(void* p) noexcept;
void* operator new[](size_t size);
void operator delete[](void* p) noexcept;
#endif

void* operator new(size_t size, const char* zFile, size_t nLine);
void operator delete(void* ptr, const char* zFile, size_t nLine);
void* operator new[](size_t size, const char* zFile, size_t nLine);
void operator delete[](void* ptr, const char* zFile, size_t nLine);

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
   
#define SET_DELETE_FRIEND \
    template <typename T> \
    friend void MemoryManager::DELETE(T*& ptr); \

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

/// Wrapper that allows usage of atomic variables in containers
/// Copy is not atomic! (e.g. push/pop from containers is not threadsafe!)
/// ref: http://stackoverflow.com/questions/13193484/how-to-declare-a-vector-of-atomic-in-c
template <typename T>
struct AtomicWrapper
{
    std::atomic<T> _a;

    AtomicWrapper() : _a()
    {
    }

    AtomicWrapper(const std::atomic<T> &a) :_a(a.load())
    {
    }

    AtomicWrapper(const AtomicWrapper &other) : _a(other._a.load())
    {
    }

    AtomicWrapper &operator=(const AtomicWrapper &other)
    {
        _a.store(other._a.load());
        return *this;
    }

    AtomicWrapper &operator=(const T &value)
    {
        _a.store(value);
        return *this;
    }

    bool operator==(const T &value) const
    {
        return _a == value;
    }
};
};  // namespace Divide

#endif

#if defined(USE_CUSTOM_MEMORY_ALLOCATORS)
#if !defined(AUTOMATIC_XALLOCATOR_INIT_DESTROY)
#define AUTOMATIC_XALLOCATOR_INIT_DESTROY
#endif
// Modify the allocator values to TIGHTLY fit memory requirments
// The application will assert if it requires more allocators than the specified nubmer
// xallocator.cpp contains the number of allocators available
#include <Allocator/Allocator.h>

#define USE_CUSTOM_ALLOCATOR DECLARE_ALLOCATOR
#define IMPLEMENT_CUSTOM_ALLOCATOR(class, objects, memory) IMPLEMENT_ALLOCATOR(class, objects, memory)
#else
#define USE_CUSTOM_ALLOCATOR
#define IMPLEMENT_CUSTOM_ALLOCATOR(class, objects, memory)
#endif
