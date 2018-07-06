#include "MainScene.h"
#include "Managers/CameraManager.h"
#include "Rendering/Application.h"
#include "PhysX/PhysX.h"
#include "Terrain/Sky.h"
#include "Terrain/Terrain.h"
#include "Terrain/Water.h"
#include "GUI/GUI.h"
#include "Rendering/Frustum.h"
#include "EngineGraphs/RenderQueue.h"
using namespace std;

bool MainScene::updateLights(){

	_sun_cosy = cosf(_sunAngle.y);
	_sunColor = _white.lerp(vec4(1.0f, 0.5f, 0.0f, 1.0f), vec4(1.0f, 1.0f, 0.8f, 1.0f),
								0.25f + _sun_cosy * 0.75f);

	getLights()[0]->setLightProperties(string("position"),_sunVector);
	getLights()[0]->setLightProperties(string("ambient"),_white);
	getLights()[0]->setLightProperties(string("diffuse"),_sunColor);
	getLights()[0]->setLightProperties(string("specular"),_sunColor);
	getLights()[0]->onDraw(); //Force update between render passes
	return true;
}

void MainScene::preRender(){

	Camera* cam = CameraManager::getInstance().getActiveCamera();
	vec3 eye_pos = cam->getEye();
	vec3 sun_pos = eye_pos - vec3(_sunVector);
	F32 zNear = _paramHandler.getParam<F32>("zNear");
	F32 zFar = _paramHandler.getParam<F32>("zFar");
	vec2 zPlanes(zNear,zFar);
	D32 tabOrtho[2] = {20.0, 100.0};
	vec3 lightPos(eye_pos.x - 500*_sunVector.x,	
				  eye_pos.y - 500*_sunVector.y,
				  eye_pos.z - 500*_sunVector.z);
	
	_GFX.setLightCameraMatrices(lightPos,eye_pos,true);
	//SHADOW MAPPING
	
	_GFX.setDepthMapRendering(true);

	for(U8 i=0; i<2; i++){
		_GFX.setOrthoProjection(vec4(-tabOrtho[i], tabOrtho[i], -tabOrtho[i], tabOrtho[i]),zPlanes);
		Frustum::getInstance().Extract(sun_pos);
	
		_depthMap[i]->Begin();
			_GFX.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
			renderEnvironment(false, true);
		_depthMap[i]->End();
	}

	_GFX.setOrthoProjection(vec4(-1.0f,1.0f,-1.0f,1.0f),zPlanes);
	Frustum::getInstance().Extract(sun_pos);
	_GFX.setLightProjectionMatrix(Frustum::getInstance().getModelViewProjectionMatrix());

	_GFX.setDepthMapRendering(false);
	_GFX.restoreLightCameraMatrices(true);
	
	Frustum::getInstance().Extract(eye_pos);
	
	//SHADOW MAPPING

	//WATER REFLECTION
	if(_water != NULL){
		_water->getReflectionFBO()->Begin();
			_GFX.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
			renderEnvironment(true,false);
		_water->getReflectionFBO()->End();
	}
}

void MainScene::render(){
	renderEnvironment(false,false);
	//renderEnvironment(true,false); //water reflection
}

bool _underwater = false;
void MainScene::renderEnvironment(bool waterReflection, bool depthMap){
	_GFX.ignoreStateChanges(true);
	
	if(!depthMap){
		Camera* cam = CameraManager::getInstance().getActiveCamera();
		if(cam->getEye().y < getWaterLevel()){
			waterReflection = false;
			_underwater = true;
			_paramHandler.setParam("underwater",true);
		}
		else{
			_underwater = false;
			_paramHandler.setParam("underwater",false);
		}
		for(U8 i = 0; i < _visibleTerrains.size(); i++){
			_visibleTerrains[i]->setRenderingOptions(waterReflection);
			_visibleTerrains[i]->getMaterial()->setAmbient(_sunColor/1.5f);
		}

		Sky &sky = Sky::getInstance();
		sky.setParams(cam->getEye(),vec3(_sunVector),waterReflection,true,true);
		sky.draw();

		if(waterReflection){
			RenderState s = _GFX.getActiveRenderState();
			s.cullingEnabled() = false;
			_GFX.setRenderState(s);
			cam->RenderLookAt(true,true,getWaterLevel());
			
		}
		updateLights();
	}
	_waterGraphNode->setActive(!waterReflection && !depthMap);
	_sceneGraph->render(); //render the rest of the stuff
	_GFX.ignoreStateChanges(false);
}

void MainScene::processInput(){
	Scene::processInput();

	Camera* cam = CameraManager::getInstance().getActiveCamera();
	moveFB  = Application::getInstance().moveFB;
	moveLR  = Application::getInstance().moveLR;
	angleLR = Application::getInstance().angleLR;
	angleUD = Application::getInstance().angleUD;
	
	if(angleLR)	cam->RotateX(angleLR * Framerate::getInstance().getSpeedfactor()/5);
	if(angleUD)	cam->RotateY(angleUD * Framerate::getInstance().getSpeedfactor()/5);
	if(moveFB || moveLR){
		if(moveFB) cam->PlayerMoveForward(moveFB * Framerate::getInstance().getSpeedfactor());
		if(moveLR) cam->PlayerMoveStrafe(moveLR * Framerate::getInstance().getSpeedfactor());
		GUI::getInstance().modifyText("camPosition","Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ]",cam->getEye().x, cam->getEye().y,cam->getEye().z);
	}

}


void MainScene::processEvents(F32 time){
	F32 SunDisplay = 1.10f;
	F32 FpsDisplay = 0.3f;
	F32 TimeDisplay = 0.01f;
	if (time - _eventTimers[0] >= SunDisplay){
		_sunAngle.y += 0.0005f;
		_sunVector = vec4(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );
		_eventTimers[0] += SunDisplay;
	}

	if (time - _eventTimers[1] >= FpsDisplay){
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		
		GUI::getInstance().modifyText("underwater","Underwater [ %s ] | WaterLevel [%f] ]", _underwater ? "true" : "false", getWaterLevel());
		GUI::getInstance().modifyText("RenderBinCount", "Number of items in Render Bin: %d", RenderQueue::getInstance().getRenderQueueStackSize());
		_eventTimers[1] += FpsDisplay;

	}
    
	
	if (time - _eventTimers[2] >= TimeDisplay){
		GUI::getInstance().modifyText("timeDisplay", "Elapsed time: %5.0f", time);
		_eventTimers[2] += TimeDisplay;
	}

	if(PhysX::getInstance().getScene() != NULL)	PhysX::getInstance().UpdateActors();
}

bool MainScene::load(const string& name){
	bool state = false;
	 _mousePressed = false;
	state = Scene::load(name);
	bool computeWaterHeight = false;
	if(getWaterLevel() == RAND_MAX) computeWaterHeight = true;
	addDefaultLight();
	state = loadResources(true);	
	state = loadEvents(true);
	for(U8 i = 0; i < TerrainInfoArray.size(); i++){
		SceneGraphNode* terrainNode = _sceneGraph->findNode(TerrainInfoArray[i]->getVariable("terrainName"));
		if(terrainNode){ //We might have an unloaded terrain in the Array, and thus, not present in the graph
			Terrain* tempTerrain = terrainNode->getNode<Terrain>();
			Console::getInstance().printfn("Found terrain:  %s!", tempTerrain->getName().c_str());
			if(terrainNode->isActive()){
				Console::getInstance().printfn("Previous found terrain is active!");
				_visibleTerrains.push_back(tempTerrain);
				if(computeWaterHeight){
					F32 tempMin = terrainNode->getBoundingBox().getMin().y;
					if(_waterHeight > tempMin) _waterHeight = tempMin;
				}
			}
		}else{
			Console::getInstance().errorfn("Could not find terrain [ %s ] in scene graph!", TerrainInfoArray[i]->getVariable("terrainName"));
		}
	}
	ResourceDescriptor infiniteWater("waterEntity");
	_water = _resManager.loadResource<WaterPlane>(infiniteWater);
	_water->setParams(50.0f,10,0.1f,0.5f);
	_waterGraphNode = _sceneGraph->getRoot()->addNode(_water);
	return state;
}

bool MainScene::unload(){
	SFXDevice::getInstance().stopMusic();
	SFXDevice::getInstance().stopAllSounds();
	RemoveResource(_backgroundMusic);
	RemoveResource(_beep);
	Sky::getInstance().DestroyInstance();
	return Scene::unload();
}

bool _switchAB = false;
void MainScene::test(boost::any a, CallbackParam b){

	vec3 pos;
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

	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f;
	gui.addText("fpsDisplay",           //Unique ID
		                       vec3(60,60,0),          //Position
							   BITMAP_8_BY_13,    //Font
							   vec3(0.0f,0.2f, 1.0f),  //Color
							   "HELLO! FPS: %s",0);    //Text and arguments

	gui.addText("timeDisplay",
								vec3(60,70,0),
								BITMAP_8_BY_13,
								vec3(0.6f,0.2f,0.2f),
								"Elapsed time: %5.0f",GETTIME());
	gui.addText("camPosition",
								vec3(60,80,0),
								BITMAP_8_BY_13,
								vec3(0.2f,0.8f,0.2f),
								"Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ]",0.0f,0.0f,0.0f);
	gui.addText("underwater",
								vec3(60,90,0),
								BITMAP_8_BY_13,
								vec3(0.2f,0.8f,0.2f),
								"Underwater [ %s ] | WaterLevel [%f] ]","false", 0);
	gui.addText("RenderBinCount",
								vec3(60,100,0),
								BITMAP_8_BY_13,
								vec3(0.6f,0.2f,0.2f),
								"Number of items in Render Bin: %d",0);
	_eventTimers.push_back(0.0f); //Sun
	_eventTimers.push_back(0.0f); //Fps
	_eventTimers.push_back(0.0f); //Time

	Console::getInstance().printfn("Creating Frame Buffer Objects for Shadow Mapping");
	_depthMap[0] = _GFX.newFBO();
	_depthMap[1] = _GFX.newFBO();
	
	Console::getInstance().printfn("Initializing Frame Buffer Objects for Shadow Mapping");
	_depthMap[0]->Create(FrameBufferObject::FBO_2D_DEPTH,2048,2048);
	_depthMap[1]->Create(FrameBufferObject::FBO_2D_DEPTH,2048,2048);
	
	_sunAngle = vec2(0.0f, RADIANS(45.0f));
	_sunVector = vec4(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );


	Event_ptr boxMove(New Event(30,true,false,boost::bind(&MainScene::test,this,string("test"),TYPE_STRING)));
	addEvent(boxMove);
	ResourceDescriptor backgroundMusic("background music");
	backgroundMusic.setResourceLocation(_paramHandler.getParam<string>("assetsLocation")+"/music/background_music.ogg");
	backgroundMusic.setFlag(true);

	ResourceDescriptor beepSound("beep sound");
	beepSound.setResourceLocation(_paramHandler.getParam<string>("assetsLocation")+"/sounds/beep.wav");
	beepSound.setFlag(false);
	_backgroundMusic = _resManager.loadResource<AudioDescriptor>(backgroundMusic);
	_beep = _resManager.loadResource<AudioDescriptor>(beepSound);
	//SFXDevice::getInstance().playMusic(_backgroundMusic);
	Console::getInstance().printfn("Scene resources loaded OK");
	return true;
}


void MainScene::onKeyDown(const OIS::KeyEvent& key){
	Scene::onKeyDown(key);
	switch(key.key)	{
		case OIS::KC_W:
			Application::getInstance().moveFB = 0.25f;
			break;
		case OIS::KC_A:
			Application::getInstance().moveLR = 0.25f;
			break;
		case OIS::KC_S:
			Application::getInstance().moveFB = -0.25f;
			break;
		case OIS::KC_D:
			Application::getInstance().moveLR = -0.25f;
			break;
		default:
			break;
	}
}

void MainScene::onKeyUp(const OIS::KeyEvent& key){
	Scene::onKeyUp(key);
	switch(key.key)	{
		case OIS::KC_W:
		case OIS::KC_S:
			Application::getInstance().moveFB = 0;
			break;
		case OIS::KC_A:
		case OIS::KC_D:
			Application::getInstance().moveLR = 0;
			break;
		case OIS::KC_X:
			SFXDevice::getInstance().playSound(_beep);
			break;
		case OIS::KC_F1:
			_sceneGraph->print();
			break;
		default:
			break;
	}

}

void MainScene::onMouseMove(const OIS::MouseEvent& key){
	if(_mousePressed){
		if(_prevMouse.x - key.state.X.abs > 1 )
			Application::getInstance().angleLR = -0.15f;
		else if(_prevMouse.x - key.state.X.abs < -1 )
			Application::getInstance().angleLR = 0.15f;
		else
			Application::getInstance().angleLR = 0;

		if(_prevMouse.y - key.state.Y.abs > 1 )
			Application::getInstance().angleUD = -0.1f;
		else if(_prevMouse.y - key.state.Y.abs < -1 )
			Application::getInstance().angleUD = 0.1f;
		else
			Application::getInstance().angleUD = 0;
	}
	
	_prevMouse.x = key.state.X.abs;
	_prevMouse.y = key.state.Y.abs;
}

void MainScene::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	if(button == 0) 
		_mousePressed = true;
}

void MainScene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	if(button == 0)	{
		_mousePressed = false;
		Application::getInstance().angleUD = 0;
		Application::getInstance().angleLR = 0;
	}
}