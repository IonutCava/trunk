#ifndef _PLATFORM_DEFINES_H_
#define _PLATFORM_DEFINES_H_

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

#define TEST_EPSILON std::numeric_limits<float>::epsilon()
#define IS_ZERO(X)  (fabs(X) < TEST_EPSILON)
#define FLOAT_COMPARE(X,Y) (fabs(X - Y) < TEST_EPSILON)
#define FLOAT_COMPARE_TOLERANCE(X,Y,TOLERANCE) (fabs(X - Y) < TOLERANCE)

#define SAFE_DELETE(R)	           if(R){ delete R; R=NULL; }
#define SAFE_DELETE_ARRAY(R)	   if(R){ delete [] R; R=NULL; }
#define SAFE_DELETE_CHECK(R)       if(R){ delete R; R=NULL; return true;}else{return false;}
#define SAFE_DELETE_ARRAY_CHECK(R) if(R){ delete [] R; R=NULL; return true;}else{return false;}
#define SAFE_DELETE_vector(R)      for(size_t r_iter(0); r_iter< R.size(); r_iter++){ delete R[r_iter]; }
#define SAFE_UPDATE(OLD,NEW)       if(OLD || NEW){ delete OLD; OLD=NEW;} ///OLD or NEW check is kinda' useless, but it's there for consistency

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

#pragma warning(disable:4103) ///<Boost alignment shouts
#pragma warning(disable:4244)
#pragma warning(disable:4996) ///< strcpy

#endif