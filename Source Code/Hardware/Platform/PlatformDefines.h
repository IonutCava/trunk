#ifndef _PLATFORM_DEFINES_H_
#define _PLATFORM_DEFINES_H_

///Data Types
#define U8  unsigned char
#define U16 unsigned short
#define U32 unsigned int
#define U64 unsigned long long

#define I8  signed char
#define I16 signed short
#define I32 signed int
#define I64 signed long long
#define _I64 __int64

#define F32 float
#define D32 double

typedef struct packed_int {
		 U8 b0;
		 U8 b1;
		 U8 b2;
		 U8 b3;
} packed_int;

typedef union {
	U32 i;
	packed_int b;
} P32;

#endif