#include "Headers/Impostor.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

Impostor::Impostor(const stringImpl& name, F32 radius) : Sphere3D(radius, 8)
{
    renderState().setDrawState(false);
    
    RenderStateBlockDescriptor dummyDesc(GFX_DEVICE.getStateBlockDescriptor(getMaterialTpl()->getRenderStateBlock(FINAL_STAGE)));
    dummyDesc.setFillMode(FILL_MODE_WIREFRAME);
    getMaterialTpl()->setRenderStateBlock(dummyDesc, FINAL_STAGE);
}

Impostor::~Impostor()
{
}

};