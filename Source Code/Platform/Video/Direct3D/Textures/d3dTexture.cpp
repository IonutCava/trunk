#include "stdafx.h"

#include "Headers/d3dTexture.h"
#include "Platform/Video/Direct3D/Headers/d3dEnumTable.h"

namespace Divide {

d3dTexture::d3dTexture(GFXDevice& context,
                       size_t descriptorHash,
                       const stringImpl& name,
                       const stringImpl& resourceName,
                       const stringImpl& resourceLocation,
                       bool isFlipped,
                       bool asyncLoad,
                       const TextureDescriptor& texDescriptor)
    : Texture(context, descriptorHash, name, resourceName, resourceLocation, isFlipped, asyncLoad, texDescriptor)
{
    _type = d3dTextureTypeTable[to_U32(texDescriptor.type())];
}

d3dTexture::~d3dTexture()
{
}

};