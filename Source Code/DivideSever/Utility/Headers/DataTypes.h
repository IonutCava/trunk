#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include "Utility/Headers/String.h"
#include "Core/Math/Headers/MathVectors.h"

namespace Divide {

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
	stringImpl ItemName;
	stringImpl ModelName;
	vec3<F32> scale;
	vec3<F32> position;
	vec3<F32> orientation;
	vec3<F32> color;
	GEOMETRY_TYPE type;
	F32 data; //general purpose
	stringImpl data2;
	F32 version;
};

namespace Util
{
    static std::stringstream _tempStream;
    template<typename T>
    stringImpl toString(T data)
    {
        _tempStream.str(stringImpl());
        _tempStream << data;
        return _tempStream.str();
    }
}

}; //namespace Divide
#endif
