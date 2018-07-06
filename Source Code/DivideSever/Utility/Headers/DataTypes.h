#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <string>
#include "Core/Math/Headers/MathVectors.h"

using namespace std;

enum GEOMETRY_TYPE
{
	MESH,
	VEGETATION,
	PRIMITIVE,
	PLACEHOLDER
};

class FileData
{
public:
	string ItemName;
	string ModelName;
	vec3<F32> scale;
	vec3<F32> position;
	vec3<F32> orientation;
	vec3<F32> color;
	GEOMETRY_TYPE type;
	F32 data; //general purpose
	string data2;
	F32 version;
};

namespace Util
{
    static std::stringstream _tempStream;
    template<class T>
    string toString(T data)
    {
        _tempStream.str(std::string());
        _tempStream << data;
        return _tempStream.str();
    }
}
#endif
