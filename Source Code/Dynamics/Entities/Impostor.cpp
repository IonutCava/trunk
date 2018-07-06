#include "Headers/Impostor.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

Impostor::Impostor(const std::string& name, F32 radius) : _visible(false){

	ResourceDescriptor materialDescriptor(name+"_impostor_material");
	materialDescriptor.setFlag(true); //No shader
	ResourceDescriptor impostorDesc(name+"_impostor");
	impostorDesc.setFlag(true); //No default material
	_dummy = CreateResource<Sphere3D>(impostorDesc);
	_dummy->setMaterial(CreateResource<Material>(materialDescriptor));
	if(GFX_DEVICE.getDeferredRendering()){
		_dummy->getMaterial()->setShaderProgram("DeferredShadingPass1.Impostor");
	}else{
		_dummy->getMaterial()->setShaderProgram("lighting.NoTexture");
	}
	_dummy->setDrawState(false);
	_dummy->setResolution(8);
	_dummy->setRadius(radius);
	RenderStateBlockDescriptor dummyDesc = _dummy->getMaterial()->getRenderState(FINAL_STAGE)->getDescriptor();
	dummyDesc._fixedLighting = false;
	dummyDesc.setFillMode(FILL_MODE_Wireframe);
	///get's deleted by the material
	_dummyStateBlock = _dummy->getMaterial()->setRenderStateBlock(dummyDesc,FINAL_STAGE);
}

Impostor::~Impostor(){
	//Do not delete dummy as it is added to the SceneGraph as the node's child. 
	//Only the SceneNode (by Uther's might) should delete the dummy
}

void Impostor::render(SceneGraphNode* const node){
	GFXDevice& gfx = GFX_DEVICE;
	if(gfx.getRenderStage() != FINAL_STAGE && gfx.getRenderStage() != DEFERRED_STAGE) return;
	SET_STATE_BLOCK(_dummyStateBlock);
	gfx.setObjectState(node->getTransform());
	gfx.setMaterial(_dummy->getMaterial());
	if(!_dummy->getMaterial()->getShaderProgram()) {
		_dummy->onDraw();
	}
	_dummy->getMaterial()->getShaderProgram()->bind();
	_dummy->getMaterial()->getShaderProgram()->Uniform("material",_dummy->getMaterial()->getMaterialMatrix());
		gfx.renderModel(_dummy);
	gfx.releaseObjectState(node->getTransform());
}

/// Render dummy at target transform
void Impostor::render(Transform* const transform){
	GFXDevice& gfx = GFX_DEVICE;
	if(gfx.getRenderStage() != FINAL_STAGE && gfx.getRenderStage() != DEFERRED_STAGE) return;
	SET_STATE_BLOCK(_dummyStateBlock);
	gfx.setObjectState(transform);
	//gfx.setMaterial(_dummy->getMaterial());
	//if(!_dummy->getMaterial()->getShaderProgram()) {
	//	_dummy->onDraw();
	//}
	//_dummy->getMaterial()->getShaderProgram()->bind();
	//_dummy->getMaterial()->getShaderProgram()->Uniform("material",_dummy->getMaterial()->getMaterialMatrix());
		gfx.renderModel(_dummy);
	gfx.releaseObjectState(transform);
}