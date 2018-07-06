#include "Headers/LightImpostor.h"
#include "Managers/Headers/ResourceManager.h" 

LightImpostor::LightImpostor(const std::string& lightName, F32 radius) : _visible(false){

	ResourceDescriptor materialDescriptor(lightName+"_impostor_material");
	materialDescriptor.setFlag(true); //No shader
	ResourceDescriptor light(lightName+"_impostor");
	light.setFlag(true); //No default material
	_dummy = ResourceManager::getInstance().loadResource<Sphere3D>(light);
	_dummy->setMaterial(ResourceManager::getInstance().loadResource<Material>(materialDescriptor));
	_dummy->setRenderState(false);
	_dummy->setResolution(8);
	_dummy->setRadius(radius);
	_dummy->getMaterial()->getRenderState().lightingEnabled() = false;
}

LightImpostor::~LightImpostor(){
	//Do not delete dummy as it is added to the SceneGraph as the Light's child. 
	//Only the Light (by Uther's might) should delete the dummy
}

void LightImpostor::render(SceneGraphNode* const lightNode){
	GFXDevice& gfx = GFXDevice::getInstance();
	//if(gfx.getRenderStage() != FINAL_STAGE) return;
	gfx.setObjectState(lightNode->getTransform());
	gfx.setRenderState(_dummy->getMaterial()->getRenderState());
	gfx.setMaterial(_dummy->getMaterial());
		gfx.renderModel(_dummy);
	gfx.restoreRenderState();
	gfx.releaseObjectState(lightNode->getTransform());
}