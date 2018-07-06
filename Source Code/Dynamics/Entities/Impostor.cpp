#include "Headers/Impostor.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

ImpostorSphere::ImpostorSphere(const stringImpl& name, F32 radius) : Sphere3D(name, radius, 8) {
    renderState().setDrawState(false);
    renderState().addToDrawExclusionMask(RenderStage::SHADOW);
}

ImpostorSphere::~ImpostorSphere()
{
}

ImpostorBox::ImpostorBox(const stringImpl& name, const vec3<F32>& size) : Box3D(name, size) {
    renderState().setDrawState(false);
    renderState().addToDrawExclusionMask(RenderStage::SHADOW);
}

ImpostorBox::~ImpostorBox()
{
}

};