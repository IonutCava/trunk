#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <string>
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
	std::string ItemName;
	std::string ModelName;
	vec3<F32> scale;
	vec3<F32> position;
	vec3<F32> orientation;
	vec3<F32> color;
	GEOMETRY_TYPE type;
	F32 data; //general purpose
	std::string data2;
	F32 version;
};

namespace Util
{
    static std::stringstream _tempStream;
    template<typename T>
    std::string toString(T data)
    {
        _tempStream.str(std::string());
        _tempStream << data;
        return _tempStream.str();
    }
}

}; //namespace Divide
#endif
