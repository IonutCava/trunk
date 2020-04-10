#include "stdafx.h"

#include "Headers/Impostor.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

ImpostorSphere::ImpostorSphere(GFXDevice& context,
                               ResourceCache* parentCache,
                               size_t descriptorHash,
                               const Str128& name,
                               F32 radius)
    : Sphere3D(context, parentCache, descriptorHash, name, radius, 8)
{
    renderState().drawState(false);
    renderState().addToDrawExclusionMask(RenderStage::SHADOW, RenderPassType::COUNT, -1);
}

ImpostorSphere::~ImpostorSphere()
{
}

ImpostorBox::ImpostorBox(GFXDevice& context,
                         ResourceCache* parentCache,
                         size_t descriptorHash,
                         const Str128& name,
                         const vec3<F32>& size)
    : Box3D(context, parentCache, descriptorHash, name, size)
{
    renderState().drawState(false);
    renderState().addToDrawExclusionMask(RenderStage::SHADOW, RenderPassType::COUNT, -1);
}

ImpostorBox::~ImpostorBox()
{
}

};