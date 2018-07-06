#include "Headers/d3dTexture.h"
#include "Hardware/Video/Direct3D/Headers/d3dEnumTable.h"

d3dTexture::d3dTexture(TextureType type, bool flipped) : Texture(type, flipped)
{
    _type = d3dTextureTypeTable[type];
}