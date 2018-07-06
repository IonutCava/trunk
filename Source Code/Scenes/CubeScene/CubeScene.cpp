#include "Headers/CubeScene.h"

#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/Headers/DeferredShadingRenderer.h"

REGISTER_SCENE(CubeScene);

void CubeScene::render(){
		
}

void CubeScene::processEvents(F32 time){
	LightManager::LightMap& lights = LightManager::getInstance().getLights();
	F32 updateLights = 0.005f;

	if(time - _eventTimers[0] >= updateLights){
		for(U8 row=0; row<3; row++)
			for(U8 col=0; col < lights.size()/3.0f; col++){
				F32 x = col * 150.0f - 5.0f + cos(GETMSTIME()*(col-row+2)*0.008f)*200.0f;
				F32 y = cos(GETTIME()*(col-row+2)*0.01f)*200.0f+20;;
				F32 z = row * 500.0f -500.0f - cos(GETMSTIME()*(col-row+2)*0.009f)*200.0f+10;
				F32 r = 1;
				F32 g = 1.0f-(row/3.0f);
				F32 b = col/(lights.size()/3.0f);
	
				lights[row*10+col]->setPosition(vec3<F32>(x,y,z));
				lights[row*10+col]->setLightProperties(LIGHT_PROPERTY_DIFFUSE,vec4<F32>(r,g,b,1));
			}

		_eventTimers[0] += updateLights;
	}

}

I8 j = 1;
F32 i = 0;
bool _switch = false;
void CubeScene::preRender() {

	if(i >= 360) _switch = true;
	else if(i <= 0) _switch = false;

	if(!_switch) i += 0.1f;
	else i -= 0.1f;

	i >= 180 ? j = -1 : j = 1;

	SceneGraphNode* cutia1 = _sceneGraph->findNode("Cutia1");
	SceneGraphNode* hellotext = _sceneGraph->findNode("HelloText");
	SceneGraphNode* bila = _sceneGraph->findNode("Bila");
	SceneGraphNode* dwarf = _sceneGraph->findNode("dwarf");
	cutia1->getTransform()->rotateEuler(vec3<F32>(0.3f*i, 0.6f*i,0));
	hellotext->getTransform()->rotate(vec3<F32>(0.6f,0.2f,0.4f),i);
	bila->getTransform()->translateY(j*0.25f);
	dwarf->getTransform()->rotate(vec3<F32>(0,1,0),i);

} 

void CubeScene::processInput(){

	if(state()->_angleLR) renderState()->getCamera()->RotateX(state()->_angleLR * FRAME_SPEED_FACTOR);
	if(state()->_angleUD) renderState()->getCamera()->RotateY(state()->_angleUD * FRAME_SPEED_FACTOR);
	if(state()->_moveFB)  renderState()->getCamera()->MoveForward(state()->_moveFB * (FRAME_SPEED_FACTOR/5));
	if(state()->_moveLR)  renderState()->getCamera()->MoveStrafe(state()->_moveLR * (FRAME_SPEED_FACTOR/5));

}

bool CubeScene::load(const std::string& name){	

	GFX_DEVICE.setRenderer(New DeferredShadingRenderer());
	///Load scene resources
	SCENE_LOAD(name,true,true);
	return loadState;
}

bool CubeScene::loadResources(bool continueOnErrors){

	//30 lights? >:)
	
	for(U8 row=0; row<3; row++)
		for(U8 col=0; col < 10; col++){
			U8 lightId = (U8)(row*10+col);
			std::stringstream ss; ss << (U32)lightId;
			ResourceDescriptor tempLight("Light Deferred " + ss.str());
			tempLight.setId(lightId);
			tempLight.setEnumValue(LIGHT_TYPE_POINT);
			Light* light = CreateResource<Light>(tempLight);
			light->setDrawImpostor(true);
			light->setRange(30);
			light->setCastShadows(false); //ToDo: Shadows are ... for another time -Ionut
			_sceneGraph->getRoot()->addNode(light);
			addLight(light);
	}
	
	_eventTimers.push_back(0.0f);
	
	return true;
}

void CubeScene::onKeyDown(const OIS::KeyEvent& key)
{
	Scene::onKeyDown(key);
	switch(key.key)
	{
		case OIS::KC_W:
			state()->_moveFB = 0.75f;
			break;
		case OIS::KC_A:
			state()->_moveLR = 0.75f;
			break;
		case OIS::KC_S:
			state()->_moveFB = -0.75f;
			break;
		case OIS::KC_D:
			state()->_moveLR = -0.75f;
			break;
		case OIS::KC_T:
			_GFX.getRenderer()->toggleDebugView();
			break;
		default:
			break;
	}
}

void CubeScene::onKeyUp(const OIS::KeyEvent& key)
{
	Scene::onKeyUp(key);
	switch(key.key)
	{
		case OIS::KC_W:
		case OIS::KC_S:
			state()->_moveFB = 0;
			break;
		case OIS::KC_A:
		case OIS::KC_D:
			state()->_moveLR = 0;
			break;
		default:
			break;
	}

}
