#include "Headers/Impostor.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"

Impostor::Impostor(const std::string& name, F32 radius) : _visible(false){
    ResourceDescriptor materialDescriptor(name+"_impostor_material");
    materialDescriptor.setFlag(true); //No shader

    ResourceDescriptor impostorDesc(name+"_impostor");
    impostorDesc.setFlag(true); //No default material

    _dummy = CreateResource<Sphere3D>(impostorDesc);
    _dummy->setMaterial(CreateResource<Material>(materialDescriptor));
    if(GFX_DEVICE.getRenderer()->getType() != RENDERER_FORWARD){
        _dummy->getMaterial()->setShaderProgram("DeferredShadingPass1.Impostor");
    }else{
        _dummy->getMaterial()->setShaderProgram("lighting");
    }
    _dummy->getSceneNodeRenderState().setDrawState(false);
    _dummy->setResolution(8);
    _dummy->setRadius(radius);
    RenderStateBlockDescriptor dummyDesc(_dummy->getMaterial()->getRenderStateBlock(FINAL_STAGE).getDescriptor());
    dummyDesc.setFillMode(FILL_MODE_WIREFRAME);
    _dummy->getMaterial()->setRenderStateBlock(dummyDesc, FINAL_STAGE);
}

Impostor::~Impostor(){
    //Do not delete dummy as it is added to the SceneGraph as the node's child.
    //Only the SceneNode (by Uther's might) should delete the dummy
}

void Impostor::render(SceneGraphNode* const node, const SceneRenderState& sceneRenderState){
    GFXDevice& gfx = GFX_DEVICE;
    if(!gfx.isCurrentRenderStage(DISPLAY_STAGE)) return;
    SET_STATE_BLOCK(_dummy->getMaterial()->getRenderStateBlock(FINAL_STAGE));
    _dummy->renderInstance()->transform(node->getTransform());

    _dummy->onDraw(node, gfx.getRenderStage());
    _dummy->getMaterial()->getShaderProgram()->bind();
    _dummy->getMaterial()->getShaderProgram()->Uniform("material",_dummy->getMaterial()->getMaterialMatrix());

    gfx.renderInstance(_dummy->renderInstance());
}

/// Render dummy at target transform
void Impostor::render(Transform* const transform, const SceneRenderState& sceneRenderState){
    GFXDevice& gfx = GFX_DEVICE;
    if(gfx.isCurrentRenderStage(DISPLAY_STAGE)) return;
    SET_STATE_BLOCK(_dummy->getMaterial()->getRenderStateBlock(FINAL_STAGE));
    _dummy->renderInstance()->transform(transform);

    _dummy->onDraw(nullptr, gfx.getRenderStage());
    _dummy->getMaterial()->getShaderProgram()->bind();
    _dummy->getMaterial()->getShaderProgram()->Uniform("material",_dummy->getMaterial()->getMaterialMatrix());

    gfx.renderInstance(_dummy->renderInstance());
}