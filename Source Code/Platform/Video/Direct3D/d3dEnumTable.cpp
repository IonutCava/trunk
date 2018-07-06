#include "Headers/d3dEnumTable.h"

namespace Divide {

U32 d3dTextureTypeTable[enum_to_uint_const(
    TextureType::TextureType_PLACEHOLDER)];

namespace D3D_ENUM_TABLE {
void fill() {
    d3dTextureTypeTable[enum_to_uint(TextureType::TEXTURE_1D)] = 0;
    d3dTextureTypeTable[enum_to_uint(TextureType::TEXTURE_2D)] = 1;
    d3dTextureTypeTable[enum_to_uint(TextureType::TEXTURE_3D)] = 2;
    d3dTextureTypeTable[enum_to_uint(TextureType::TEXTURE_CUBE_MAP)] = 3;
    d3dTextureTypeTable[enum_to_uint(TextureType::TEXTURE_2D_ARRAY)] = 4;
    d3dTextureTypeTable[enum_to_uint(TextureType::TEXTURE_CUBE_ARRAY)] = 5;
    d3dTextureTypeTable[enum_to_uint(TextureType::TEXTURE_2D_MS)] = 6;
    d3dTextureTypeTable[enum_to_uint(TextureType::TEXTURE_2D_ARRAY_MS)] = 7;
}
}
};