#include "Headers/WarScene.h"
#include "Headers/WarSceneAIActionList.h"

#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "Managers/Headers/CameraManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Environment/Sky/Headers/Sky.h"
#include "Managers/Headers/AIManager.h"
#include "Rendering/Headers/Frustum.h"
#include "GUI/Headers/GUI.h"

//begin copy-paste: randarea scenei
void WarScene::render(){
	Sky& sky = Sky::getInstance();

	sky.setParams(CameraManager::getInstance().getActiveCamera()->getEye(),_sunVector,false,true,true);
	sky.draw();

	_sceneGraph->render();
}
//end copy-paste

void WarScene::preRender(){
	vec2<F32> _sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
	_sunVector = vec4<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );

	LightManager::getInstance().getLight(0)->setLightProperties(LIGHT_POSITION,_sunVector);

}

void WarScene::processEvents(F32 time){
	F32 FpsDisplay = 0.3f;
	if (time - _eventTimers[0] >= FpsDisplay){
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		GUI::getInstance().modifyText("RenderBinCount", "Number of items in Render Bin: %d", GFX_RENDER_BIN_SIZE);
		_eventTimers[0] += FpsDisplay;
	}
}

void WarScene::resetSimulation(){
	getEvents().clear();
}

void WarScene::startSimulation(){

	resetSimulation();

	if(getEvents().empty()){//Maxim un singur eveniment
		Event_ptr newSim(New Event(12,true,false,boost::bind(&WarScene::processSimulation,this,rand() % 5,TYPE_INTEGER)));
		addEvent(newSim);
	}
}

void WarScene::processSimulation(boost::any a, CallbackParam b){

	if(getEvents().empty()) return;
	bool updated = false;
	std::string mesaj;
	SceneGraphNode* Soldier1 = _sceneGraph->findNode("Soldier1");

	assert(Soldier1);assert(_groundPlaceholder);
}

void WarScene::processInput(){
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
	}
}

bool WarScene::load(const std::string& name){

	setInitialData();

	bool state = false;
	//Add a light
	Light* light = addDefaultLight();
	light->setLightProperties(LIGHT_AMBIENT,_white);
	light->setLightProperties(LIGHT_DIFFUSE,_white);
	light->setLightProperties(LIGHT_SPECULAR,_white);
												
	//Load scene resources
	state = loadResources(true);	
	
	//Position camera
	CameraManager::getInstance().getActiveCamera()->RotateX(RADIANS(45));
	CameraManager::getInstance().getActiveCamera()->RotateY(RADIANS(25));
	CameraManager::getInstance().getActiveCamera()->setEye(vec3<F32>(14,5.5f,11.5f));
	_faction1 = New AICoordination(1);
	_faction2 = New AICoordination(2);


	//------------------------ Restul elementelor jocului -----------------------------///
	_groundPlaceholder = _sceneGraph->findNode("Ground_placeholder");
	_groundPlaceholder->getNode<SceneNode>()->getMaterial()->setCastsShadows(false);

	state = loadEvents(true);
	return state;
}

bool WarScene::initializeAI(bool continueOnErrors){
		//----------------------------INTELIGENTA ARTIFICIALA------------------------------//

	AIEntity* aiSoldier = NULL;
	SceneGraphNode* soldierMesh = _sceneGraph->findNode("Soldier1");
	if(soldierMesh){
		AIEntity* aiSoldier = New AIEntity("Soldier1");
		aiSoldier->attachNode(soldierMesh);
		aiSoldier->addSensor(VISUAL_SENSOR,New VisualSensor());
		aiSoldier->addSensor(COMMUNICATION_SENSOR, New CommunicationSensor(aiSoldier));
		aiSoldier->addActionProcessor(New WarSceneAIActionList());
		aiSoldier->setTeam(_faction1);
		_army1.push_back(aiSoldier);
	}

	soldierMesh = _sceneGraph->findNode("Soldier2");
	if(soldierMesh){
		aiSoldier = New AIEntity("Soldier2");
		aiSoldier->attachNode(soldierMesh);
		aiSoldier->addSensor(VISUAL_SENSOR,New VisualSensor());
		aiSoldier->addSensor(COMMUNICATION_SENSOR, New CommunicationSensor(aiSoldier));
		aiSoldier->addActionProcessor(New WarSceneAIActionList());
		aiSoldier->setTeam(_faction2);
		_army2.push_back(aiSoldier);
	}
	//----------------------- Unitati ce vor fi controlate de AI ---------------------//
	for(U8 i = 0; i < _army1.size(); i++){
		NPC* soldier = New NPC(_army1[i]);
		soldier->setMovementSpeed(2); /// 2 m/s
		_army1NPCs.push_back(soldier);
		AIManager::getInstance().addEntity(_army1[i]);
	}

	for(U8 i = 0; i < _army2.size(); i++){
		NPC* soldier = New NPC(_army2[i]);
		soldier->setMovementSpeed(2); /// 2 m/s
		_army2NPCs.push_back(soldier);
		AIManager::getInstance().addEntity(_army2[i]);
	}
	return !(_army1.empty() || _army2.empty());
}

bool WarScene::deinitializeAI(bool continueOnErrors){
	for(U8 i = 0; i < _army1NPCs.size(); i++){
		SAFE_DELETE(_army1NPCs[i]);
	}
	_army1NPCs.clear();
	for(U8 i = 0; i < _army2NPCs.size(); i++){
		SAFE_DELETE(_army2NPCs[i]);
	}
	_army2NPCs.clear();
	SAFE_DELETE(_faction1);
	SAFE_DELETE(_faction2);
	return true;
}

bool WarScene::loadResources(bool continueOnErrors){
	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;

	
	GUI::getInstance().addButton("Simulate", "Simulate", vec2<F32>(Application::getInstance().getWindowDimensions().width-220 ,
															 Application::getInstance().getWindowDimensions().height/1.1f),
													    	 vec2<F32>(100,25),vec3<F32>(0.65f,0.65f,0.65f),
															 boost::bind(&WarScene::startSimulation,this));

	GUI::getInstance().addText("fpsDisplay",           //Unique ID
		                       vec3<F32>(60,60,0),          //Position
							   BITMAP_8_BY_13,    //Font
							   vec3<F32>(0.0f,0.2f, 1.0f),  //Color
							   "FPS: %s",0);    //Text and arguments
	GUI::getInstance().addText("RenderBinCount",
								vec3<F32>(60,70,0),
								BITMAP_8_BY_13,
								vec3<F32>(0.6f,0.2f,0.2f),
								"Number of items in Render Bin: %d",0);

	
	_eventTimers.push_back(0.0f); //Fps
	return true;
}

bool WarScene::unload(){
	Sky::getInstance().DestroyInstance();
	return Scene::unload();
}


void WarScene::onKeyDown(const OIS::KeyEvent& key){
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

void WarScene::onKeyUp(const OIS::KeyEvent& key){
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
		case OIS::KC_F1:
			_sceneGraph->print();
			break;
		default:
			break;
	}

}
void WarScene::onMouseMove(const OIS::MouseEvent& key){
	Scene::onMouseMove(key);
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

void WarScene::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	Scene::onMouseClickDown(key,button);
	if(button == 0) 
		_mousePressed = true;
}

void WarScene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	Scene::onMouseClickUp(key,button);
	if(button == 0)	{
		_mousePressed = false;
		Application::getInstance().angleUD = 0;
		Application::getInstance().angleLR = 0;
	}
}