/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PLATFORM_DEFINES_H_
#define _PLATFORM_DEFINES_H_

#include <string>
///Data Types
#define U8   unsigned char
#define U16  unsigned short
#define U32  unsigned int
#define UL32 unsigned long
#define U64  unsigned long long

#define I8  signed char
#define I16 signed short
#define I32 signed int
#define L32 signed long
#define I64 signed long long
#define _I64 __int64

#define F32 float
#define D32 double

#ifndef _P_D_TYPES_ONLY_

#define TEST_EPSILON std::numeric_limits<F32>::epsilon()
#define IS_ZERO(X)  (fabs((F32)X) < TEST_EPSILON)
#define IS_TOLERANCE(X,TOLERANCE) (fabs(X) < TOLERANCE)
#define FLOAT_COMPARE(X,Y) (fabs(X - Y) < TEST_EPSILON)
#define FLOAT_COMPARE_TOLERANCE(X,Y,TOLERANCE) (fabs(X - Y) < TOLERANCE)

#define SAFE_DELETE(R)	           if(R){ delete R; R=NULL; }
#define SAFE_DELETE_ARRAY(R)	   if(R){ delete [] R; R=NULL; }
#define SAFE_DELETE_CHECK(R)       if(R){ delete R; R=NULL; return true;}else{return false;}
#define SAFE_DELETE_ARRAY_CHECK(R) if(R){ delete [] R; R=NULL; return true;}else{return false;}
#define SAFE_DELETE_vector(R)      for(size_t r_iter(0); r_iter< R.size(); r_iter++){ delete R[r_iter]; }
#define SAFE_UPDATE(OLD,NEW)       if(OLD || NEW){ delete OLD; OLD=NEW;} ///OLD or NEW check is kinda' useless, but it's there for consistency

#else
#undef _P_D_TYPES_ONLY_
#endif

namespace DivideNetworking{
template<class T>
inline std::string toString(const T& data){
	std::stringstream s;
	s << data;
	return s.str();
}
};

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