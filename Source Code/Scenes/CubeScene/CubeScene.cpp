#include "Headers/CubeScene.h"

#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/Headers/DeferredShadingRenderer.h"
#include "Core/Math/Headers/Transform.h"

REGISTER_SCENE(CubeScene);

void CubeScene::render(){
}

void CubeScene::processTasks(const U64 deltaTime){
	LightManager::LightMap& lights = LightManager::getInstance().getLights();
	D32 updateLights = getSecToMs(0.05);

	if(_taskTimers[0] >= updateLights){
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

		_taskTimers[0] = 0.0;
	}
    Scene::processTasks(deltaTime);
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

void CubeScene::processInput(const U64 deltaTime){
	if(state()._angleLR) renderState().getCamera().rotateYaw(state()._angleLR);
	if(state()._angleUD) renderState().getCamera().rotatePitch(state()._angleUD);
	if(state()._moveFB)  renderState().getCamera().moveForward(state()._moveFB);
	if(state()._moveLR)  renderState().getCamera().moveStrafe(state()._moveLR);
}

bool CubeScene::load(const std::string& name, CameraManager* const cameraMgr){
	GFX_DEVICE.setRenderer(New DeferredShadingRenderer());
	//Load scene resources
	return SCENE_LOAD(name,cameraMgr,true,true);
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
			light->setLightProperties(LIGHT_PROPERTY_BRIGHTNESS, 30.0f);
			light->setCastShadows(false); //ToDo: Shadows are ... for another time -Ionut
			_sceneGraph->getRoot()->addNode(light);
			addLight(light);
	}

	_taskTimers.push_back(0.0);

	return true;
}

void CubeScene::onKeyDown(const OIS::KeyEvent& key)
{
	Scene::onKeyDown(key);
	switch(key.key)	{
		default: break;
		case OIS::KC_W: state()._moveFB =  1; break;
		case OIS::KC_A:	state()._moveLR = -1; break;
		case OIS::KC_S:	state()._moveFB = -1; break;
		case OIS::KC_D:	state()._moveLR =  1; break;
		case OIS::KC_T:	_GFX.getRenderer()->toggleDebugView(); break;
	}
}

void CubeScene::onKeyUp(const OIS::KeyEvent& key)
{
	Scene::onKeyUp(key);
	switch(key.key)
	{
		case OIS::KC_W:
		case OIS::KC_S:	state()._moveFB = 0;	break;
		case OIS::KC_A:
		case OIS::KC_D:	state()._moveLR = 0;	break;
		default: break;
	}
}