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

#ifndef _PLATFORM_DEFINES_H_
#define _PLATFORM_DEFINES_H_

#include "Utility/Headers/Vector.h"
#include "Utility/Headers/String.h"
#include "Utility/Headers/HashMap.h"
#include "Core/Headers/Singleton.h"
#include "Core/Headers/NonCopyable.h"
#include <limits.h>
#include <functional>
#include <atomic>

namespace Divide {

///Data Types
typedef uint8_t  U8;
typedef uint16_t U16; 
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t   I8;
typedef int16_t  I16;
typedef int32_t  I32;
typedef int64_t  I64;
typedef __int64 _I64;
typedef float    F32;
typedef double   D32;

/// Converts an arbitrary positive integer value to a bitwise value used for masks
#define toBit(X) (1 << (X))
/// a la Boost
template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
/* See
 
http://randomascii.wordpress.com/2012/01/11/tricks-with-the-floating-point-format/
 
for the potential portability problems with the union and bit-fields below.
*/
union Float_t {
    Float_t(F32 num = 0.0f) : f(num) {}
    // Portable extraction of components.
    bool Negative()    const { return (i >> 31) != 0; }
    I32  RawMantissa() const { return i & ((1 << 23) - 1); }
    I32  RawExponent() const { return (i >> 23) & 0xFF; }
 
    I32 i;
    F32 f;
};

union Double_t {
    Double_t(D32 num = 0.0) : d(num) {}
    // Portable extraction of components.
    bool  Negative()    const { return (i >> 63) != 0; }
    I64   RawMantissa() const { return i & ((1LL << 52) - 1); }
    I64   RawExponent() const { return (i >> 52) & 0x7FF; }

   I64 i;
   D32 d;
};

inline bool AlmostEqualUlpsAndAbs(F32 A, F32 B, F32 maxDiff, I32 maxUlpsDiff)
{
    // Check if the numbers are really close -- needed
    // when comparing numbers near zero.
    F32 absDiff = std::fabs(A - B);
    if (absDiff <= maxDiff)
        return true;
 
    Float_t uA(A);
    Float_t uB(B);
 
    // Different signs means they do not match.
    if (uA.Negative() != uB.Negative())
        return false;
 
    // Find the difference in ULPs.
    return (std::abs(uA.i - uB.i) <= maxUlpsDiff);
}

inline bool AlmostEqualUlpsAndAbs(D32 A, D32 B, D32 maxDiff, I32 maxUlpsDiff)
{
    // Check if the numbers are really close -- needed
    // when comparing numbers near zero.
    D32 absDiff = std::fabs(A - B);
    if (absDiff <= maxDiff)
        return true;
 
    Double_t uA(A);
    Double_t uB(B);
 
    // Different signs means they do not match.
    if (uA.Negative() != uB.Negative())
        return false;
 
    // Find the difference in ULPs.
    return (std::abs(uA.i - uB.i) <= maxUlpsDiff);
}

inline bool AlmostEqualRelativeAndAbs(F32 A, F32 B, F32 maxDiff, F32 maxRelDiff)
{
    // Check if the numbers are really close -- needed
    // when comparing numbers near zero.
    F32 diff = std::fabs(A - B);
    if (diff <= maxDiff)
        return true;
 
    A = std::fabs(A);
    B = std::fabs(B);
    F32 largest = (B > A) ? B : A;
 
    return (diff <= largest * maxRelDiff);
}

inline bool AlmostEqualRelativeAndAbs(D32 A, D32 B, D32 maxDiff, D32 maxRelDiff)
{
    // Check if the numbers are really close -- needed
    // when comparing numbers near zero.
    D32 diff = std::fabs(A - B);
    if (diff <= maxDiff)
        return true;
 
    A = std::fabs(A);
    B = std::fabs(B);
    D32 largest = (B > A) ? B : A;
 
    return (diff <= largest * maxRelDiff);
}

static const F32 EPSILON_F32 = std::numeric_limits<F32>::epsilon();
static const D32 EPSILON_D32 = std::numeric_limits<D32>::epsilon();

template<typename T>
inline bool IS_ZERO( T X ) { return X == 0; }
template<>
inline bool IS_ZERO(F32 X) { return  (std::fabs(X) < EPSILON_F32); }
template<>
inline bool IS_ZERO(D32 X) { return  (std::fabs(X) < EPSILON_D32); }

template<typename T>
inline bool IS_TOLERANCE(T X, T TOLERANCE )      { return ( std::abs( X ) < TOLERANCE ); }
template<>
inline bool IS_TOLERANCE( F32 X, F32 TOLERANCE ) { return ( std::fabs( X ) < TOLERANCE ); }
template<>
inline bool IS_TOLERANCE( D32 X, D32 TOLERANCE ) { return ( std::fabs( X ) < TOLERANCE ); }

inline bool FLOAT_COMPARE_TOLERANCE(F32 X, F32 Y, F32 TOLERANCE)  {  return AlmostEqualUlpsAndAbs(X, Y, TOLERANCE, 4); }
inline bool DOUBLE_COMPARE_TOLERANCE(D32 X, D32 Y, D32 TOLERANCE) {  return AlmostEqualUlpsAndAbs(X, Y, TOLERANCE, 4); }

inline bool FLOAT_COMPARE(F32 X, F32 Y)  { return FLOAT_COMPARE_TOLERANCE(X, Y, EPSILON_F32); }
inline bool DOUBLE_COMPARE(D32 X, D32 Y) { return DOUBLE_COMPARE_TOLERANCE(X, Y, EPSILON_D32); }

/// Performes extra asserts steps (logging, message boxes, etc). Returns true if the assert should be processed.
bool preAssert( const bool expression, const char* failMessage );
/// It is safe to call evaluate expressions and call functions inside the assert check as it will compile for every build type
inline bool DIVIDE_ASSERT(const bool expression, const char* failMessage) {
	if ( preAssert( expression, failMessage ) ) {
		assert( expression && failMessage );
	}
	return expression;
}

typedef struct packed_int {
    U8 b0; U8 b1; U8 b2; U8 b3;
} packed_int;

typedef union {
    U32 i;
    packed_int b;
} P32;

template <typename... Args>
auto DELEGATE_BIND(Args&&... args) -> decltype(std::bind(std::forward<Args>(args)...)) {
	return std::bind(std::forward<Args>(args)...);
}

template <typename... Args>
auto DELEGATE_REF(Args&&... args) -> decltype(std::bind(std::forward<Args>(args)...)) {
	return std::bind(std::forward<Args>(args)...);
}

template <typename... Args>
auto DELEGATE_CREF(Args&&... args) -> decltype(std::cref(std::forward<Args>(args)...)) {
	return std::cref(std::forward<Args>(args)...);
}

template <typename T = void>
using DELEGATE_CBK = std::function<T()>;

}; //namespace Divide

void* operator new[]( size_t size, const char* pName, Divide::I32 flags, Divide::U32 debugFlags, const char* file, Divide::I32 line );
void  operator delete[]( void* ptr, const char* pName, Divide::I32 flags, Divide::U32 debugFlags, const char* file, Divide::I32 line );
void* operator new[]( size_t size, size_t alignment, size_t alignmentOffset, const char* pName, Divide::I32 flags, Divide::U32 debugFlags, const char* file, Divide::I32 line );
void  operator delete[]( void* ptr, size_t alignment, size_t alignmentOffset, const char* pName, Divide::I32 flags, Divide::U32 debugFlags, const char* file, Divide::I32 line );
// EASTL also wants us to define this (see string.h line 197)
Divide::I32 Vsnprintf8( char* pDestination, size_t n, const char* pFormat, va_list arguments );

#if defined(NDEBUG)
#   define New new
#else
	void* operator new( size_t size );
	void  operator delete( void *p );
	void* operator new[]( size_t size );
	void  operator delete[]( void *p );

	void* operator new( size_t size, char* zFile, Divide::I32 nLine );
	void  operator delete( void *ptr, char* zFile, Divide::I32 nLine );
	void* operator new[]( size_t size, char* zFile, Divide::I32 nLine );
	void  operator delete[]( void *ptr, char* zFile, Divide::I32 nLine );

#   define New new(__FILE__, __LINE__)
#endif

namespace Divide {
    namespace MemoryManager {

    template<class T>
    /// Deletes and nullifies the specified pointer only if it's not null
    inline void SAFE_DELETE( T*& ptr ) {
        if ( ptr ) {
            delete ptr;
            ptr = nullptr;
        }
    }
#   define SET_SAFE_DELETE_FRIEND template<class T> friend void MemoryManager::SAFE_DELETE( T*& ptr );
    template<class T>
    /// Deletes and nullifies the specified array pointer only if it's not null
    inline void SAFE_DELETE_ARRAY( T*& ptr ) {
        if ( ptr ) {
            delete[] ptr;
            ptr = nullptr;
        }
    }
#   define SET_SAFE_DELETE_ARRAY_FRIEND template<class T> friend void MemoryManager::SAFE_DELETE_ARRAY( T*& ptr );
    template<class T>
    /// Deletes and nullifies the specified pointer only if it's not null. Returns "true" if deletion was successful
    inline bool SAFE_DELETE_CHECK( T*& ptr ) {
        if ( ptr ) {
            delete ptr;
            ptr = nullptr;
            return true;
        }
        return false;
    }
#   define SET_SAFE_DELETE_CHECK_FRIEND template<class T> friend void MemoryManager::SAFE_DELETE_CHECK( T*& ptr );
    template<class T>
    /// Deletes and nullifies the specified array pointer only if it's not null. Returns "true" if deletion was successful
    inline bool SAFE_DELETE_ARRAY_CHECK( T*& ptr ) {
        if ( ptr ) {
            delete[] ptr;
            ptr = nullptr;
            return true;
        }
        return false;
    }
#   define SET_SAFE_DELETE_ARRAY_CHECK_FRIEND template<class T> friend void MemoryManager::SAFE_DELETE_ARRAY_CHECK( T*& ptr );
    template<class T>
    /// Deletes every element from the vector and clears it at the end
    inline void SAFE_DELETE_VECTOR( vectorImpl<T*>& vec ) {
        if ( !vec.empty() ) {
            for ( T* iter : vec ) {
                delete iter;
            }
            vec.clear();
        }
    }
#   define SET_SAFE_DELETE_VECTOR_FRIEND template<class T> friend void MemoryManager::SAFE_DELETE_VECTOR( vectorImpl<T*>& vec );
    template<class T, class U>
    /// Deletes the object pointed to by "OLD" and redirects that pointer to the object pointed by "NEW"
    /// "NEW" must be a derived (or same) class of OLD
    inline void SAFE_UPDATE( T*& OLD, U* const NEW ) {
        static_assert( std::is_base_of<T, U>::value, "SAFE_UPDATE error: T must be a descendant of U" );
        if ( OLD ) {
            delete OLD;
        }
        OLD = NEW;
    }
#   define SET_SAFE_UPDATE_FRIEND template<class T, class U> friend void MemoryManager::SAFE_UPDATE( T*& OLD, U* const NEW );

    }; //namespace MemoryManager
}; //namespace Divide
#if defined(_MSC_VER)

#	pragma warning(disable:4103) ///<Boost alignment shouts
#	pragma warning(disable:4244)
#	pragma warning(disable:4996) ///< strcpy
#	pragma warning(disable:4201) ///<nameless struct
#	pragma warning(disable:4100) ///<unreferenced formal param
#	pragma warning(disable:4505) ///<unreferenced local function removal
#	pragma warning(disable:4127) ///<Constant conditional expressions
#elif defined(__GNUC__)
//#	pragma GCC diagnostic ignored "-Wall"
#endif

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#endif