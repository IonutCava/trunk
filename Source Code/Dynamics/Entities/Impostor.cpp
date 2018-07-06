#include "Headers/Impostor.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

ImpostorSphere::ImpostorSphere(const stringImpl& name, F32 radius) : Sphere3D(radius, 8) {
    renderState().setDrawState(false);

    RenderStateBlock dummyDesc(GFX_DEVICE.getRenderStateBlock(
        getMaterialTpl()->getRenderStateBlock(RenderStage::DISPLAY)));
    dummyDesc.setFillMode(FillMode::WIREFRAME);
    getMaterialTpl()->setRenderStateBlock(dummyDesc.getHash(), RenderStage::DISPLAY);
}

ImpostorSphere::~ImpostorSphere()
{
}

ImpostorBox::ImpostorBox(const stringImpl& name, const vec3<F32>& size) : Box3D(size) {
    renderState().setDrawState(false);

    RenderStateBlock dummyDesc(GFX_DEVICE.getRenderStateBlock(
        getMaterialTpl()->getRenderStateBlock(RenderStage::DISPLAY)));
    dummyDesc.setFillMode(FillMode::WIREFRAME);
    getMaterialTpl()->setRenderStateBlock(dummyDesc.getHash(), RenderStage::DISPLAY);
}

ImpostorBox::~ImpostorBox()
{
}

};