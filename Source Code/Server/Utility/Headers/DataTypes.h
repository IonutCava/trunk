#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include "Core/Math/Headers/MathVectors.h"

namespace Divide {

enum class GeometryType : U32 { 
    MESH,
    VEGETATION,
    PRIMITIVE,
    PLACEHOLDER
};

class FileData {
   public:
    stringImpl ItemName;
    stringImpl ModelName;
    vec3<F32> scale;
    vec3<F32> position;
    vec3<F32> orientation;
    vec3<F32> colour;
    GeometryType type;
    F32 data;  // general purpose
    stringImpl data2;
    F32 version;
};

};  // namespace Divide
#endif
