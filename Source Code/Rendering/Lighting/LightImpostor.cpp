#include "Headers/LightImpostor.h"
#include "Managers/Headers/ResourceManager.h" 
#include "Hardware/Video/RenderStateBlock.h"

LightImpostor::LightImpostor(const std::string& lightName, F32 radius) : _visible(false){

	ResourceDescriptor materialDescriptor(lightName+"_impostor_material");
	materialDescriptor.setFlag(true); //No shader
	ResourceDescriptor light(lightName+"_impostor");
	light.setFlag(true); //No default material
	_dummy = CreateResource<Sphere3D>(light);
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
	///get's deleted by the material
	_dummyStateBlock = _dummy->getMaterial()->setRenderStateBlock(dummyDesc,FINAL_STAGE);
}

LightImpostor::~LightImpostor(){
	//Do not delete dummy as it is added to the SceneGraph as the Light's child. 
	//Only the Light (by Uther's might) should delete the dummy
}

void LightImpostor::render(SceneGraphNode* const lightNode){
	GFXDevice& gfx = GFX_DEVICE;
	if(gfx.getRenderStage() != FINAL_STAGE && gfx.getRenderStage() != DEFERRED_STAGE) return;
	SET_STATE_BLOCK(_dummyStateBlock);
	gfx.setObjectState(lightNode->getTransform());
	gfx.setMaterial(_dummy->getMaterial());
	_dummy->getMaterial()->getShaderProgram()->bind();
	_dummy->getMaterial()->getShaderProgram()->Uniform("material",_dummy->getMaterial()->getMaterialMatrix());
		gfx.renderModel(_dummy);
	gfx.releaseObjectState(lightNode->getTransform());
}