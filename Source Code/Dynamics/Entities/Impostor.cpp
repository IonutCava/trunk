#include "Headers/Impostor.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

Impostor::Impostor(const stringImpl& name, F32 radius) : Sphere3D(radius, 8) {
    renderState().setDrawState(false);

    RenderStateBlockDescriptor dummyDesc(GFX_DEVICE.getStateBlockDescriptor(
        getMaterialTpl()->getRenderStateBlock(RenderStage::DISPLAY_STAGE)));
    dummyDesc.setFillMode(FillMode::FILL_MODE_WIREFRAME);
    getMaterialTpl()->setRenderStateBlock(dummyDesc, RenderStage::DISPLAY_STAGE);
}

Impostor::~Impostor() {}
};