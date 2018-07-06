#include "Headers/d3dTexture.h"
#include "Platform/Video/Direct3D/Headers/d3dEnumTable.h"

namespace Divide {

d3dTexture::d3dTexture(TextureType type, bool flipped)
    : Texture(type, flipped) {
    _type = d3dTextureTypeTable[enum_to_uint(type)];
}
};