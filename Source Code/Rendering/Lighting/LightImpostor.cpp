#include "Headers/LightImpostor.h"
#include "Managers/Headers/ResourceManager.h" 

LightImpostor::LightImpostor(const std::string& lightName, F32 radius) : _visible(false){

	ResourceDescriptor materialDescriptor(lightName+"_impostor_material");
	ResourceDescriptor light(lightName+"_impostor");
	light.setFlag(true);
	_dummy = ResourceManager::getInstance().loadResource<Sphere3D>(light);
	_dummy->setMaterial(ResourceManager::getInstance().loadResource<Material>(materialDescriptor));
	_dummy->setRenderState(false);
	_dummy->setResolution(8);
	_dummy->setSize(radius);
}

LightImpostor::~LightImpostor(){
	//Do not delete dummy as it is added to the SceneGraph as the Light's child. 
	//Only the Light (by Uther's might) should delete the dummy
}

void LightImpostor::render(SceneGraphNode* const lightNode){
	GFXDevice& gfx = GFXDevice::getInstance();
	RenderState s = gfx.getActiveRenderState();
	s.lightingEnabled() = false;
	gfx.setRenderState(s);
	gfx.setObjectState(lightNode->getTransform());
		 gfx.renderModel(_dummy);
	gfx.releaseObjectState(lightNode->getTransform());
	gfx.restoreRenderState();
}