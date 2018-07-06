#include "Headers/Impostor.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"

Impostor::Impostor(const std::string& name, F32 radius) : Sphere3D(radius, 8)
{
    getSceneNodeRenderState().setDrawState(false);
    
    RenderStateBlockDescriptor dummyDesc(GFX_DEVICE.getStateBlockDescriptor(getMaterial()->getRenderStateBlock(FINAL_STAGE)));
    dummyDesc.setFillMode(FILL_MODE_WIREFRAME);
    getMaterial()->setRenderStateBlock(dummyDesc, FINAL_STAGE);
    setName(name);
    setState(RES_LOADED);
}

Impostor::~Impostor(){

}