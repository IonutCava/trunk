#include "Headers/d3dTexture.h"
#include "Platform/Video/Direct3D/Headers/d3dEnumTable.h"

namespace Divide {

d3dTexture::d3dTexture(GFXDevice& context,
                       const stringImpl& name,
                       const stringImpl& resourceName,
                       const stringImpl& resourceLocation,
                       TextureType type,
                       bool asyncLoad)
    : Texture(context, name, resourceName, resourceLocation, type, asyncLoad)
{
    _type = d3dTextureTypeTable[to_uint(type)];
}

};