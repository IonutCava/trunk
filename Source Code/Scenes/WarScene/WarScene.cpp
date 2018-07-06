#include "Headers/WarScene.h"
#include "Headers/WarSceneAIActionList.h"

#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/AIManager.h"
#include "Rendering/Headers/Frustum.h"

REGISTER_SCENE(WarScene);

void WarScene::preRender(){
	vec2<F32> _sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
	_sunvector = vec3<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y));

	//LightManager::getInstance().getLight(0)->setPosition(_sunvector);
	getSkySGN(0)->getNode<Sky>()->setSunVector(_sunvector);
}

void WarScene::processTasks(const U32 time){
	F32 FpsDisplay = 0.3f;
	if (time - _taskTimers[0] >= FpsDisplay){
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f", Framerate::getInstance().getFps(), Framerate::getInstance().getFrameTime());
		GUI::getInstance().modifyText("RenderBinCount", "Number of items in Render Bin: %d", GFX_RENDER_BIN_SIZE);
		_taskTimers[0] += FpsDisplay;
	}
}

void WarScene::resetSimulation(){
	removeTasks();
}

void WarScene::startSimulation(){
	resetSimulation();

	if(getTasks().empty()){
		Kernel* kernel = Application::getInstance().getKernel();
		Task_ptr newSim(New Task(kernel->getThreadPool(),12,true,false,DELEGATE_BIND(&WarScene::processSimulation,this,rand() % 5,TYPE_INTEGER)));
		addTask(newSim);
	}
}

void WarScene::processSimulation(boost::any a, CallbackParam b){
	if(getTasks().empty()) return;
	//SceneGraphNode* Soldier1 = _sceneGraph->findNode("Soldier1");
	//assert(Soldier1);
    //assert(_groundPlaceholder);
}

void WarScene::processInput(){
	if(state()->_angleLR) renderState()->getCamera()->RotateX(state()->_angleLR);
	if(state()->_angleUD) renderState()->getCamera()->RotateY(state()->_angleUD);
	if(state()->_moveFB)  renderState()->getCamera()->MoveForward(state()->_moveFB /5);
	if(state()->_moveLR)  renderState()->getCamera()->MoveStrafe(state()->_moveLR /5);
}

bool WarScene::load(const std::string& name){
	///Load scene resources
	SCENE_LOAD(name,true,true);
	//Add a light
	Light* light = addDefaultLight();
	light->setLightProperties(LIGHT_PROPERTY_AMBIENT,WHITE());
	light->setLightProperties(LIGHT_PROPERTY_DIFFUSE,WHITE());
	light->setLightProperties(LIGHT_PROPERTY_SPECULAR,WHITE());
	//Add a skybox
	addDefaultSky();
	//Position camera
	renderState()->getCamera()->RotateX(RADIANS(45));
	renderState()->getCamera()->RotateY(RADIANS(25));
	renderState()->getCamera()->setEye(vec3<F32>(14,5.5f,11.5f));
	//Create 2 AI teams
	_faction1 = New AICoordination(1);
	_faction2 = New AICoordination(2);

	//------------------------ The rest of the scene elements -----------------------------///
//	_groundPlaceholder = _sceneGraph->findNode("Ground_placeholder");
//	_groundPlaceholder->getNode<SceneNode>()->getMaterial()->setCastsShadows(false);

	return loadState;
}

bool WarScene::initializeAI(bool continueOnErrors){
	//----------------------------Artificial Intelligence------------------------------//

	AIEntity* aiSoldier = NULL;
	SceneGraphNode* soldierMesh = _sceneGraph->findNode("Soldier1");
	if(soldierMesh){
		AIEntity* aiSoldier = New AIEntity("Soldier1");
		aiSoldier->attachNode(soldierMesh);
		aiSoldier->addSensor(VISUAL_SENSOR,New VisualSensor());
		aiSoldier->setComInterface();
		aiSoldier->addActionProcessor(New WarSceneAIActionList());
		aiSoldier->setTeam(_faction1);
		_army1.push_back(aiSoldier);
	}

	soldierMesh = _sceneGraph->findNode("Soldier2");
	if(soldierMesh){
		aiSoldier = New AIEntity("Soldier2");
		aiSoldier->attachNode(soldierMesh);
		aiSoldier->addSensor(VISUAL_SENSOR,New VisualSensor());
		aiSoldier->setComInterface();
		aiSoldier->addActionProcessor(New WarSceneAIActionList());
		aiSoldier->setTeam(_faction2);
		_army2.push_back(aiSoldier);
	}
	//----------------------- Unitati ce vor fi controlate de AI ---------------------//
	for(U8 i = 0; i < _army1.size(); i++){
		NPC* soldier = New NPC(_army1[i]);
		soldier->setMovementSpeed(1.2f); /// 1.2 m/s
		_army1NPCs.push_back(soldier);
		AIManager::getInstance().addEntity(_army1[i]);
	}

	for(U8 i = 0; i < _army2.size(); i++){
		NPC* soldier = New NPC(_army2[i]);
		soldier->setMovementSpeed(1.23f); /// 1.23 m/s
		_army2NPCs.push_back(soldier);
		AIManager::getInstance().addEntity(_army2[i]);
	}

	bool state = !(_army1.empty() || _army2.empty());

	if(state || continueOnErrors) Scene::initializeAI(continueOnErrors);

	return state;
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
	for(U8 i = 0; i < _army1.size(); i++){
		AIManager::getInstance().destroyEntity(_army1[i]->getGUID());
	}
	_army1.clear();
	for(U8 i = 0; i < _army2.size(); i++){
		AIManager::getInstance().destroyEntity(_army2[i]->getGUID());
	}
	_army2.clear();
	SAFE_DELETE(_faction1);
	SAFE_DELETE(_faction2);
	return Scene::deinitializeAI(continueOnErrors);
}

bool WarScene::loadResources(bool continueOnErrors){
	GUI::getInstance().addButton("Simulate", "Simulate", vec2<I32>(renderState()->cachedResolution().width-220 ,
																   renderState()->cachedResolution().height/1.1f),
													     vec2<U32>(100,25),vec3<F32>(0.65f,0.65f,0.65f),
														 DELEGATE_BIND(&WarScene::startSimulation,this));

	GUI::getInstance().addText("fpsDisplay",           //Unique ID
		                       vec2<I32>(60,60),          //Position
							    Font::DIVIDE_DEFAULT,    //Font
							   vec3<F32>(0.0f,0.2f, 1.0f),  //Color
							   "FPS: %s",0);    //Text and arguments
	GUI::getInstance().addText("RenderBinCount",
								vec2<I32>(60,70),
								 Font::DIVIDE_DEFAULT,
								vec3<F32>(0.6f,0.2f,0.2f),
								"Number of items in Render Bin: %d",0);

	_taskTimers.push_back(0.0f); //Fps
	return true;
}

void WarScene::onKeyDown(const OIS::KeyEvent& key){
	Scene::onKeyDown(key);
	switch(key.key)	{
		case OIS::KC_W:
			state()->_moveFB = 0.25f;
			break;
		case OIS::KC_A:
			state()->_moveLR = 0.25f;
			break;
		case OIS::KC_S:
			state()->_moveFB = -0.25f;
			break;
		case OIS::KC_D:
			state()->_moveLR = -0.25f;
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
			state()->_moveFB = 0;
			break;
		case OIS::KC_A:
		case OIS::KC_D:
			state()->_moveLR = 0;
			break;
		case OIS::KC_F1:
			_sceneGraph->print();
			break;
		default:
			break;
	}
}
void WarScene::onMouseMove(const OIS::MouseEvent& key){
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

void WarScene::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	Scene::onMouseClickDown(key,button);
	if(button == 0)
		_mousePressed = true;
}

void WarScene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	Scene::onMouseClickUp(key,button);
	if(button == 0)	{
		_mousePressed = false;
		state()->_angleUD = 0;
		state()->_angleLR = 0;
	}
}