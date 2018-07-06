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

#include <limits>

///Data Types
#ifndef U8
#define U8   unsigned char
#endif
#ifndef U16
#define U16  unsigned short
#endif
#ifndef U32
#define U32  unsigned int
#endif
#ifndef UL32
#define UL32 unsigned long
#endif
#ifndef U64
#define U64  unsigned long long
#endif

#ifndef I8
#define I8  signed char
#endif
#ifndef I16
#define I16 signed short
#endif
#ifndef I32
#define I32 signed int
#endif
#ifndef L32
#define L32 signed long
#endif
#ifndef I64
#define I64 signed long long
#endif
#ifndef _I64
#define _I64 __int64
#endif
#ifndef F32
#define F32 float
#endif
#ifndef D32
#define D32 double
#endif
#ifndef _P_D_TYPES_ONLY_

/// Converts an arbitrary positive integer value to a bitwise value used for masks
#define toBit(X) (1 << (X))

static const F32 TEST_EPSILON     = std::numeric_limits<F32>::epsilon();
static const D32 TEST_EPSILON_D32 = std::numeric_limits<D32>::epsilon();

inline bool IS_ZERO(F32 X) { return  (fabs(X) < TEST_EPSILON); }
inline bool IS_ZERO(D32 X) { return  (fabs(X) < TEST_EPSILON_D32); }

inline bool IS_TOLERANCE(F32 X, F32 TOLERANCE) { return (fabs(X) < TOLERANCE); }
inline bool IS_TOLERANCE(D32 X, D32 TOLERANCE) { return (fabs(X) < TOLERANCE); }

inline bool FLOAT_COMPARE(F32 X, F32 Y)  { return (fabs(X/Y - 1) < TEST_EPSILON); }
inline bool DOUBLE_COMPARE(D32 X, D32 Y) { return (fabs(X/Y - 1) < TEST_EPSILON_D32); }

inline bool FLOAT_COMPARE_TOLERANCE(F32 X, F32 Y, F32 TOLERANCE)  { return (fabs(X/Y - 1) < TOLERANCE); }
inline bool DOUBLE_COMPARE_TOLERANCE(D32 X, D32 Y, D32 TOLERANCE) { return (fabs(X/Y - 1) < TOLERANCE); }

#define SAFE_DELETE(R)	           if(R){ delete R; R=NULL; }
#define SAFE_DELETE_ARRAY(R)	   if(R){ delete [] R; R=NULL; }
#define SAFE_DELETE_CHECK(R)       if(R){ delete R; R=NULL; return true;}else{return false;}
#define SAFE_DELETE_ARRAY_CHECK(R) if(R){ delete [] R; R=NULL; return true;}else{return false;}
#define SAFE_DELETE_vector(R)      for(size_t r_iter(0); r_iter< R.size(); r_iter++){ delete R[r_iter]; }
#define SAFE_UPDATE(OLD,NEW)       if(OLD || NEW){ delete OLD; OLD=NEW;} ///OLD or NEW check is kinda' useless, but it's there for consistency

#define DELEGATE_BIND boost::bind
#define DELEGATE_REF  boost::ref
typedef struct packed_int {
	U8 b0; U8 b1; U8 b2; U8 b3;
} packed_int;

typedef union {
	U32 i;
	packed_int b;
} P32;
#else
#undef _P_D_TYPES_ONLY_
#endif
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

#endif