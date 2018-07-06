#include "Headers/MainScene.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers/Application.h"
#include "Rendering/Headers/Frustum.h"
#include "Environment/Sky/Headers/Sky.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Environment/Water/Headers/Water.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"



bool MainScene::updateLights(){
	Light* light = LightManager::getInstance().getLight(0);
	_sun_cosy = cosf(_sunAngle.y);
	_sunColor = _white.lerp(vec4<F32>(1.0f, 0.5f, 0.0f, 1.0f), vec4<F32>(1.0f, 1.0f, 0.8f, 1.0f),
									  0.25f + _sun_cosy * 0.75f);

	light->setLightProperties(LIGHT_POSITION,_sunVector);
	light->setLightProperties(LIGHT_DIFFUSE,_sunColor);
	light->setLightProperties(LIGHT_SPECULAR,_sunColor);
	return true;
}

void MainScene::preRender(){
	updateLights();
}

void MainScene::render(){
	renderEnvironment(false);
}

bool _underwater = false;
void MainScene::renderEnvironment(bool waterReflection){

	if(_water->isCameraUnderWater()){
		waterReflection = false;
		_underwater = true;
		_paramHandler.setParam("underwater",true);
	}
	else{
		_underwater = false;
		_paramHandler.setParam("underwater",false);
	}
	_water->setRenderingOptions(_camera->getEye());
	for_each(Terrain* ter, _visibleTerrains){
		ter->setRenderingOptions(waterReflection, _camera->getEye());
		ter->getMaterial()->setAmbient(_sunColor/1.5f);
	}

	Sky::getInstance().setParams(_camera->getEye(),vec3<F32>(_sunVector),waterReflection,true,true);
	Sky::getInstance().draw();

	if(waterReflection){
		_camera->RenderLookAt(true,true,getWaterLevel());
	}
	_sceneGraph->render(); //render the rest of the stuff
}

void MainScene::processInput(){
	Scene::processInput();

	if(_angleLR) _camera->RotateX(_angleLR * Framerate::getInstance().getSpeedfactor()/5);
	if(_angleUD) _camera->RotateY(_angleUD * Framerate::getInstance().getSpeedfactor()/5);

	if(_moveFB || _moveLR){
		if(_moveFB)  _camera->MoveForward(_moveFB * (Framerate::getInstance().getSpeedfactor()));
		if(_moveLR)	 _camera->MoveStrafe(_moveLR * (Framerate::getInstance().getSpeedfactor()));
		GUI::getInstance().modifyText("camPosition","Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ]",
									  _camera->getEye().x, _camera->getEye().y,_camera->getEye().z);
	}

}


void MainScene::processEvents(F32 time){
	F32 SunDisplay = 1.10f;
	F32 FpsDisplay = 0.3f;
	F32 TimeDisplay = 0.01f;
	if (time - _eventTimers[0] >= SunDisplay){
		_sunAngle.y += 0.0005f;
		_sunVector = vec4<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
								-cosf(_sunAngle.y),
								-sinf(_sunAngle.x) * sinf(_sunAngle.y),
								0.0f );
		_eventTimers[0] += SunDisplay;
	}

	if (time - _eventTimers[1] >= FpsDisplay){
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		
		GUI::getInstance().modifyText("underwater","Underwater [ %s ] | WaterLevel [%f] ]", _underwater ? "true" : "false", getWaterLevel());
		GUI::getInstance().modifyText("RenderBinCount", "Number of items in Render Bin: %d", GFX_RENDER_BIN_SIZE);
		_eventTimers[1] += FpsDisplay;

	}
    
	
	if (time - _eventTimers[2] >= TimeDisplay){
		GUI::getInstance().modifyText("timeDisplay", "Elapsed time: %5.0f", time);
		_eventTimers[2] += TimeDisplay;
	}
}

bool MainScene::load(const std::string& name){

	setInitialData();
	bool state = false;
	 _mousePressed = false;
	state = Scene::load(name);
	bool computeWaterHeight = false;
	if(getWaterLevel() == RAND_MAX) computeWaterHeight = true;
	Light* light = addDefaultLight();
	light->setLightProperties(LIGHT_AMBIENT,_white);
	light->setLightProperties(LIGHT_DIFFUSE,_white);
	light->setLightProperties(LIGHT_SPECULAR,_white);
												
	//Incarcam resursele scenei
	state = loadResources(true);	
	state = loadEvents(true);
	for(U8 i = 0; i < TerrainInfoArray.size(); i++){
		SceneGraphNode* terrainNode = _sceneGraph->findNode(TerrainInfoArray[i]->getVariable("terrainName"));
		if(terrainNode){ //We might have an unloaded terrain in the Array, and thus, not present in the graph
			Terrain* tempTerrain = terrainNode->getNode<Terrain>();
			D_PRINT_FN("Found terrain:  %s!", tempTerrain->getName().c_str());
			if(terrainNode->isActive()){
				D_PRINT_FN("Previous found terrain is active!");
				_visibleTerrains.push_back(tempTerrain);
				if(computeWaterHeight){
					F32 tempMin = terrainNode->getBoundingBox().getMin().y;
					if(_waterHeight > tempMin) _waterHeight = tempMin;
				}
			}
		}else{
			ERROR_FN("Could not find terrain [ %s ] in scene graph!", TerrainInfoArray[i]->getVariable("terrainName"));
		}
	}
	ResourceDescriptor infiniteWater("waterEntity");
	_water = CreateResource<WaterPlane>(infiniteWater);
	_water->setParams(50.0f,50,0.2f,0.5f);
	_water->setRenderCallback(boost::bind(&MainScene::renderEnvironment, this, true));
	_waterGraphNode = _sceneGraph->getRoot()->addNode(_water);
	_waterGraphNode->useDefaultTransform(false);
	_waterGraphNode->setTransform(NULL);
	return state;
}

bool MainScene::unload(){
	SFX_DEVICE.stopMusic();
	SFX_DEVICE.stopAllSounds();
	RemoveResource(_backgroundMusic);
	RemoveResource(_beep);
	Sky::getInstance().DestroyInstance();
	return Scene::unload();
}

bool _switchAB = false;
void MainScene::test(boost::any a, CallbackParam b){

	vec3<F32> pos;
	SceneGraphNode* boxNode = _sceneGraph->findNode("box");
	Object3D* box = NULL;
	if(boxNode) box = boxNode->getNode<Object3D>();
	if(box) pos = boxNode->getTransform()->getPosition();

	if(!_switchAB){
		if(pos.x < 300 && pos.z == 0)		   pos.x++;
		if(pos.x == 300)
		{
			if(pos.y < 800 && pos.z == 0)      pos.y++;
			if(pos.y == 800)
			{
				if(pos.z > -500)   pos.z--;
				if(pos.z == -500)  _switchAB = true;
			}
			
		}
	} else {
		if(pos.x > -300 && pos.z ==  -500)      pos.x--;
		if(pos.x == -300)
		{
			if(pos.y > 100 && pos.z == -500)    pos.y--;
			if(pos.y == 100) {
				if(pos.z < 0)    pos.z++;
				if(pos.z == 0)   _switchAB = false;
			}
		}
	}
	if(box)	boxNode->getTransform()->setPosition(pos);
}

bool MainScene::loadResources(bool continueOnErrors){
	GUI& gui = GUI::getInstance();

	gui.addText("fpsDisplay",           //Unique ID
		                       vec3<F32>(60,60,0),          //Position
							   BITMAP_8_BY_13,    //Font
							   vec3<F32>(0.0f,0.2f, 1.0f),  //Color
							   "HELLO! FPS: %s",0);    //Text and arguments

	gui.addText("timeDisplay",
								vec3<F32>(60,70,0),
								BITMAP_8_BY_13,
								vec3<F32>(0.6f,0.2f,0.2f),
								"Elapsed time: %5.0f",GETTIME());
	gui.addText("camPosition",
								vec3<F32>(60,80,0),
								BITMAP_8_BY_13,
								vec3<F32>(0.2f,0.8f,0.2f),
								"Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ]",0.0f,0.0f,0.0f);
	gui.addText("underwater",
								vec3<F32>(60,90,0),
								BITMAP_8_BY_13,
								vec3<F32>(0.2f,0.8f,0.2f),
								"Underwater [ %s ] | WaterLevel [%f] ]","false", 0);
	gui.addText("RenderBinCount",
								vec3<F32>(60,100,0),
								BITMAP_8_BY_13,
								vec3<F32>(0.6f,0.2f,0.2f),
								"Number of items in Render Bin: %d",0);
	_eventTimers.push_back(0.0f); //Sun
	_eventTimers.push_back(0.0f); //Fps
	_eventTimers.push_back(0.0f); //Time


	_sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
	_sunVector = vec4<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );


	Event_ptr boxMove(New Event(30,true,false,boost::bind(&MainScene::test,this,std::string("test"),TYPE_STRING)));
	addEvent(boxMove);
	ResourceDescriptor backgroundMusic("background music");
	backgroundMusic.setResourceLocation(_paramHandler.getParam<std::string>("assetsLocation")+"/music/background_music.ogg");
	backgroundMusic.setFlag(true);

	ResourceDescriptor beepSound("beep sound");
	beepSound.setResourceLocation(_paramHandler.getParam<std::string>("assetsLocation")+"/sounds/beep.wav");
	beepSound.setFlag(false);
	_backgroundMusic = CreateResource<AudioDescriptor>(backgroundMusic);
	_beep = CreateResource<AudioDescriptor>(beepSound);
	PRINT_FN("Scene resources loaded OK");
	return true;
}


void MainScene::onKeyDown(const OIS::KeyEvent& key){
	Scene::onKeyDown(key);
	switch(key.key)	{
		case OIS::KC_W:
			_moveFB = 0.25f;
			break;
		case OIS::KC_A:
			_moveLR = 0.25f;
			break;
		case OIS::KC_S:
			_moveFB = -0.25f;
			break;
		case OIS::KC_D:
			_moveLR = -0.25f;
			break;
		default:
			break;
	}
}

bool _playMusic = false;
void MainScene::onKeyUp(const OIS::KeyEvent& key){
	Scene::onKeyUp(key);
	switch(key.key)	{
		case OIS::KC_W:
		case OIS::KC_S:
			_moveFB = 0;
			break;
		case OIS::KC_A:
		case OIS::KC_D:
			_moveLR = 0;
			break;
		case OIS::KC_X:
			SFX_DEVICE.playSound(_beep);
			break;
		case OIS::KC_M:{
			_playMusic = !_playMusic;
			if(_playMusic){
				SFX_DEVICE.playMusic(_backgroundMusic);
			}else{
				SFX_DEVICE.stopMusic();
			}
			}break;
		case OIS::KC_F1:
			_sceneGraph->print();
			break;
		case OIS::KC_T:
			for_each(Terrain* ter, _visibleTerrains){
				ter->toggleBoundingBoxes();
			}
			break;
		default:
			break;
	}

}

void MainScene::onMouseMove(const OIS::MouseEvent& key){
	Scene::onMouseMove(key);
	if(_mousePressed){
		if(_prevMouse.x - key.state.X.abs > 1 )
			_angleLR = -0.15f;
		else if(_prevMouse.x - key.state.X.abs < -1 )
			_angleLR = 0.15f;
		else
			_angleLR = 0;

		if(_prevMouse.y - key.state.Y.abs > 1 )
			_angleUD = -0.1f;
		else if(_prevMouse.y - key.state.Y.abs < -1 )
			_angleUD = 0.1f;
		else
			_angleUD = 0;
	}
	
	_prevMouse.x = key.state.X.abs;
	_prevMouse.y = key.state.Y.abs;
}

void MainScene::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	Scene::onMouseClickDown(key,button);
	if(button == 0) 
		_mousePressed = true;
}

void MainScene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	Scene::onMouseClickUp(key,button);
	if(button == 0)	{
		_mousePressed = false;
		_angleUD = 0;
		_angleLR = 0;
	}
}