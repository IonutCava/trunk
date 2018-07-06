#define U8  unsigned char
#define U16 unsigned short
#define U32 unsigned int
#define U64 unsigned long long

#define I8  signed char
#define I16 signed short
#define I32 signed int
#define I64 signed long long

#define F32 float
#define D32 double
#define UBYTE unsigned char

#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <string>
#include "MathClasses.h"

using namespace std;

enum GEOMETRY_TYPE
{
	MESH,
	VEGETATION,
	PLACEHOLDER
};

class FileData
{
public:
	string ModelName;
	vec3 scale;
	vec3 position;
	vec3 orientation;
	GEOMETRY_TYPE type;
	F32 version;
};

namespace Util
{
    static std::stringstream _tempStream;
	template<class T>
	string toString(T data)
	{
        _tempStream.clear();
		s << data;
		return s.str();
	}
}
#endif
