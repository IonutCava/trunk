#include "Headers/MainScene.h"

#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/SceneManager.h"
#include "Environment/Water/Headers/Water.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

REGISTER_SCENE(MainScene);

bool MainScene::updateLights(){
	Light* light = LightManager::getInstance().getLight(0);
	_sun_cosy = cosf(_sunAngle.y);
	_sunColor = WHITE().lerp(vec4<F32>(1.0f, 0.5f, 0.0f, 1.0f),
		                     vec4<F32>(1.0f, 1.0f, 0.8f, 1.0f),
							 0.25f + _sun_cosy * 0.75f);

	light->setPosition(_sunvector);
	light->setLightProperties(LIGHT_PROPERTY_DIFFUSE,_sunColor);
	light->setLightProperties(LIGHT_PROPERTY_SPECULAR,_sunColor);
	return true;
}

void MainScene::preRender(){
	updateLights();
}

bool _underwater = false;
void MainScene::renderEnvironment(bool waterReflection){

	const vec3<F32>& eyePos = renderState()->getCamera()->getEye();
	_underwater = _water->isCameraUnderWater();
	_paramHandler.setParam("scene.camera.underwater", _underwater);
	if(_underwater) waterReflection = false;

	vec3<F32> eyeTemp(eyePos);
	if(waterReflection){
		renderState()->getCamera()->RenderLookAt(true,true,state()->getWaterLevel());
		eyeTemp.y = 2.0f*state()->getWaterLevel()-eyePos.y;
	}
	getSkySGN(0)->getNode<Sky>()->setRenderingOptions(eyeTemp,_sunvector,waterReflection);
	_water->setRenderingOptions(eyePos);
	for_each(Terrain* ter, _visibleTerrains){
		ter->setRenderingOptions(waterReflection, eyePos);
	}
	GFX_DEVICE.render(SCENE_GRAPH_UPDATE(_sceneGraph),renderState());
}

void MainScene::processInput(){

	if(state()->_angleLR) renderState()->getCamera()->RotateX(state()->_angleLR * FRAME_SPEED_FACTOR/5);
	if(state()->_angleUD) renderState()->getCamera()->RotateY(state()->_angleUD * FRAME_SPEED_FACTOR/5);

	if(state()->_moveFB || state()->_moveLR){
		if(state()->_moveFB)  renderState()->getCamera()->MoveForward(state()->_moveFB * (FRAME_SPEED_FACTOR));
		if(state()->_moveLR)  renderState()->getCamera()->MoveStrafe(state()->_moveLR * (FRAME_SPEED_FACTOR));
		GUI::getInstance().modifyText("camPosition","Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ]",
									  renderState()->getCamera()->getEye().x, 
									  renderState()->getCamera()->getEye().y,
									  renderState()->getCamera()->getEye().z);
	}

}


void MainScene::processEvents(F32 time){
	time /= 1000; ///<convert to seconds
	F32 SunDisplay = 1.50f;
	F32 FpsDisplay = 0.5f;
	F32 TimeDisplay = 1.0f;
	if (time - _eventTimers[0] >= SunDisplay){
		_sunAngle.y += 0.0005f;
		_sunvector = vec4<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
								-cosf(_sunAngle.y),
								-sinf(_sunAngle.x) * sinf(_sunAngle.y),
								0.0f );
		_eventTimers[0] += SunDisplay;
	}

	if (time - _eventTimers[1] >= FpsDisplay){
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		
		GUI::getInstance().modifyText("underwater","Underwater [ %s ] | WaterLevel [%f] ]", _underwater ? "true" : "false", state()->getWaterLevel());
		GUI::getInstance().modifyText("RenderBinCount", "Number of items in Render Bin: %d", GFX_RENDER_BIN_SIZE);
		_eventTimers[1] += FpsDisplay;

	}
    
	
	if (time - _eventTimers[2] >= TimeDisplay){
		GUI::getInstance().modifyText("timeDisplay", "Elapsed time: %5.0f", time);
		_eventTimers[2] += TimeDisplay;
	}
}

bool MainScene::load(const std::string& name){

	 _mousePressed = false;
	bool computeWaterHeight = false;
	
	///Load scene resources
	SCENE_LOAD(name,true,true);

	if(state()->getWaterLevel() == RAND_MAX) computeWaterHeight = true;
	Light* light = addDefaultLight();
	light->setLightProperties(LIGHT_PROPERTY_AMBIENT,WHITE());
	light->setLightProperties(LIGHT_PROPERTY_DIFFUSE,WHITE());
	light->setLightProperties(LIGHT_PROPERTY_SPECULAR,WHITE());
	addDefaultSky();								

	for(U8 i = 0; i < _terrainInfoArray.size(); i++){
		SceneGraphNode* terrainNode = _sceneGraph->findNode(_terrainInfoArray[i]->getVariable("terrainName"));
		if(terrainNode){ //We might have an unloaded terrain in the Array, and thus, not present in the graph
			Terrain* tempTerrain = terrainNode->getNode<Terrain>();
			if(terrainNode->isActive()){
				_visibleTerrains.push_back(tempTerrain);
				if(computeWaterHeight){
					F32 tempMin = terrainNode->getBoundingBox().getMin().y;
					if(state()->_waterHeight > tempMin) state()->_waterHeight = tempMin;
				}
			}
		}else{
			ERROR_FN(Locale::get("ERROR_MISSING_TERRAIN"), _terrainInfoArray[i]->getVariable("terrainName"));
		}
	}
	ResourceDescriptor infiniteWater("waterEntity");
	_water = CreateResource<WaterPlane>(infiniteWater);
	_water->setParams(50.0f,50,0.2f,0.5f);
	_waterGraphNode = _sceneGraph->getRoot()->addNode(_water);
	_waterGraphNode->useDefaultTransform(false);
	_waterGraphNode->setTransform(NULL);
	///General rendering callback
	renderCallback(boost::bind(&MainScene::renderEnvironment, this,false));
	///Render the scene for water reflection FBO generation
	_water->setRenderCallback(boost::bind(&MainScene::renderEnvironment, this, true));

	return loadState;
}

bool MainScene::unload(){
	SFX_DEVICE.stopMusic();
	SFX_DEVICE.stopAllSounds();
	RemoveResource(_beep);
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

	gui.addText("fpsDisplay",                               //Unique ID
		                       vec2<F32>(60,60),            //Position
							    Font::DIVIDE_DEFAULT,       //Font
							   vec3<F32>(0.0f,0.2f, 1.0f),  //Color
							   "HELLO! FPS: %s",0);    //Text and arguments

	gui.addText("timeDisplay",
								vec2<F32>(60,80),
								Font::DIVIDE_DEFAULT,
								vec3<F32>(0.6f,0.2f,0.2f),
								"Elapsed time: %5.0f",GETTIME());
	gui.addText("underwater",
								vec2<F32>(60,115),
								Font::DIVIDE_DEFAULT,
								vec3<F32>(0.2f,0.8f,0.2f),
								"Underwater [ %s ] | WaterLevel [%f] ]","false", 0);
	gui.addText("RenderBinCount",
								vec2<F32>(60,135),
								Font::BATANG,
								vec3<F32>(0.6f,0.2f,0.2f),
								"Number of items in Render Bin: %d",0);
	_eventTimers.push_back(0.0f); //Sun
	_eventTimers.push_back(0.0f); //Fps
	_eventTimers.push_back(0.0f); //Time


	_sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
	_sunvector = vec4<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
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
	state()->_backgroundMusic["generalTheme"] = CreateResource<AudioDescriptor>(backgroundMusic);
	_beep = CreateResource<AudioDescriptor>(beepSound);

	gui.addText("camPosition",  vec2<F32>(60,100),
								Font::DIVIDE_DEFAULT,
								vec3<F32>(0.2f,0.8f,0.2f),
								"Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ]",
								renderState()->getCamera()->getEye().x,
								renderState()->getCamera()->getEye().y,
								renderState()->getCamera()->getEye().z);

	return true;
}


void MainScene::onKeyDown(const OIS::KeyEvent& key){
	Scene::onKeyDown(key);
	switch(key.key)	{
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
			state()->_moveFB = 0;
			break;
		case OIS::KC_A:
		case OIS::KC_D:
			state()->_moveLR = 0;
			break;
		case OIS::KC_X:
			SFX_DEVICE.playSound(_beep);
			break;
		case OIS::KC_M:{
			_playMusic = !_playMusic;
			if(_playMusic){
				SFX_DEVICE.playMusic(state()->_backgroundMusic["generalTheme"]);
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

	if(_mousePressed){
		if(_prevMouse.x - key.state.X.abs > 1 )
			state()->_angleLR = -0.15f;
		else if(_prevMouse.x - key.state.X.abs < -1 )
			state()->_angleLR = 0.15f;
		else
			state()->_angleLR = 0;

		if(_prevMouse.y - key.state.Y.abs > 1 )
			state()->_angleUD = -0.1f;
		else if(_prevMouse.y - key.state.Y.abs < -1 )
			state()->_angleUD = 0.1f;
		else
			state()->_angleUD = 0;
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
		state()->_angleUD = 0;
		state()->_angleLR = 0;
	}
}