#include "Headers/AITenisScene.h"
#include "Headers/AITenisSceneAIActionList.h"

#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Managers/Headers/SceneManager.h"
#include "Environment/Sky/Headers/Sky.h"
#include "Managers/Headers/AIManager.h"
#include "Rendering/Headers/Frustum.h"
#include "GUI/Headers/GUI.h"


//begin copy-paste: randarea scenei
void AITenisScene::render(){
	Sky& sky = Sky::getInstance();

	sky.setParams(_camera->getEye(),_sunVector,false,true,true);
	sky.draw();

	_sceneGraph->render();
}
//end copy-paste

void AITenisScene::preRender(){
	vec2<F32> _sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
	_sunVector = vec4<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );

	LightManager::getInstance().getLight(0)->setLightProperties(LIGHT_POSITION,_sunVector);

}

void AITenisScene::processEvents(F32 time){
	F32 FpsDisplay = 0.3f;
	if (time - _eventTimers[0] >= FpsDisplay){
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		GUI::getInstance().modifyText("RenderBinCount", "Number of items in Render Bin: %d", GFX_RENDER_BIN_SIZE);
		_eventTimers[0] += FpsDisplay;
	}
}

void AITenisScene::resetGame(){
	getEvents().clear();
	_directionTeam1ToTeam2 = true;
	_upwardsDirection = true;
	_touchedTerrainTeam1 = false;
	_touchedTerrainTeam2 = false;
	_lostTeam1 = false;
	_applySideImpulse = false;
	_sideImpulseFactor = 0;
	_ballPositionUpdate.lock();
	_ballSGN->getTransform()->setPosition(vec3<F32>(3.0f, 0.2f ,7.0f));
	_ballPositionUpdate.unlock();
}

void AITenisScene::startGame(){

	resetGame();

	if(getEvents().empty()){///A maximum of 1 events allowed
		Event_ptr jocNou(New Event(12,true,false,boost::bind(&AITenisScene::procesareJoc,this,rand() % 5,TYPE_INTEGER)));
		addEvent(jocNou);
	}
}


//Team 1: Player1 + Player2
//Team 2: Player3 + Player4
void AITenisScene::procesareJoc(boost::any a, CallbackParam b){

	if(getEvents().empty()) return;
	bool updated = false;
	std::string mesaj;

	///Shortcut to the scene graph nodes containing our agents
	SceneGraphNode* Player1 = _sceneGraph->findNode("Player1");
	SceneGraphNode* Player2 = _sceneGraph->findNode("Player2");
	SceneGraphNode* Player3 = _sceneGraph->findNode("Player3");
	SceneGraphNode* Player4 = _sceneGraph->findNode("Player4");


	///Santy checks
	assert(Player1);assert(Player2);assert(Player3);assert(Player4);
	assert(_net);assert(_floor);assert(_ballSGN);

	///Store by copy (thread-safe) current ball position
	_ballPositionQuery.lock();
	Transform* ballTransform = _ballSGN->getTransform();
	vec3<F32> ballPosition = ballTransform->getPosition();
	_ballPositionQuery.unlock();
	vec3<F32> netPosition  = _net->getTransform()->getPosition();
	///Is the ball traveling from team 1 to team 2?
	_directionTeam1ToTeam2 ? ballPosition.z -= 0.123f : ballPosition.z += 0.123f;
	///Is the ball traveling upwards or is it falling?
	_upwardsDirection ? ballPosition.y += 0.066f : ballPosition.y -= 0.066f;
	///In case of a side drift, update accordingly
	if(_applySideImpulse){
		ballPosition.x += _sideImpulseFactor*0.025f;
	}
	///After we finish our computations, apply the new transform
	_ballPositionUpdate.lock();
	_ballPositionQuery.lock();
	ballTransform->translate(ballPosition - ballTransform->getPosition());
	_ballPositionQuery.unlock();
	///Add a spin to the ball just for fin ...
	ballTransform->rotateEuler(vec3<F32>(ballPosition.z,1,1));
	_ballPositionUpdate.unlock();
	//----------------------COLLISIONS------------------------------//
	//z = depth. Descending to the horizon
	if(_floor->getBoundingBox().Collision(_ballSGN->getBoundingBox())){
		_applySideImpulse = true;
		if(ballPosition.z > netPosition.z){
			_touchedTerrainTeam1 = true;
			_touchedTerrainTeam2 = false;
		}else{
			_touchedTerrainTeam1 = false;
			_touchedTerrainTeam2 = true;
		}
		_upwardsDirection = true;
	}

	///Where does the Kinetic  energy of the ball run out?
	if(ballPosition.y > 3.5) _upwardsDirection = false;
	
	///Did we hit a player?
	bool collisionPlayer1 = _ballSGN->getBoundingBox().Collision(Player1->getBoundingBox());
	bool collisionPlayer2 = _ballSGN->getBoundingBox().Collision(Player2->getBoundingBox());
	bool collisionPlayer3 = _ballSGN->getBoundingBox().Collision(Player3->getBoundingBox());
	bool collisionPlayer4 = _ballSGN->getBoundingBox().Collision(Player4->getBoundingBox());

	bool collisionTeam1 = collisionPlayer1 || collisionPlayer2;
	bool collisionTeam2 = collisionPlayer3 || collisionPlayer4;

	if(collisionTeam1){
		F32 sideDrift = 0;
		collisionPlayer1 ? sideDrift = Player1->getTransform()->getPosition().x : sideDrift = Player2->getTransform()->getPosition().x;
		_sideImpulseFactor = ballPosition.x - sideDrift;
		_directionTeam1ToTeam2 = true;
	}

	if(collisionTeam2){
		F32 sideDrift = 0;
		collisionPlayer3 ? sideDrift = Player3->getTransform()->getPosition().x : sideDrift = Player4->getTransform()->getPosition().x;
		_sideImpulseFactor = ballPosition.x - sideDrift;
		_directionTeam1ToTeam2 = false;
	}

	//-----------------VALIDATING THE RESULTS----------------------//
	///Which team won?
	if(ballPosition.z >= Player1->getTransform()->getPosition().z &&
	   ballPosition.z >= Player2->getTransform()->getPosition().z){
		_lostTeam1 = true;
		updated = true;
	}

	if(ballPosition.z <= Player3->getTransform()->getPosition().z &&
	   ballPosition.z <= Player4->getTransform()->getPosition().z){
		_lostTeam1 = false;
		updated = true;
	}

	///Which team kicked the ball in the net?
	if(_ballSGN->getBoundingBox().Collision(_net->getBoundingBox())){
		if(ballPosition.y < _net->getBoundingBox().getMax().y - 1){
			if(_directionTeam1ToTeam2){
				_lostTeam1 = true;
			}else{
				_lostTeam1 = false;
			}
			updated = true;
		}
    }
	if(ballPosition.x + 0.5f < _net->getBoundingBox().getMin().x || ballPosition.x + 0.5f > _net->getBoundingBox().getMax().x){
		///If hit the ball and it touched the opposing teams terrain
		///Or if the opposing team hit the ball but it didn't land in our terrain
		if(_floor->getBoundingBox().Collision(_ballSGN->getBoundingBox())){
			if((_touchedTerrainTeam2 && _directionTeam1ToTeam2) || (!_directionTeam1ToTeam2 && !_touchedTerrainTeam1)){
				_lostTeam1 = false;
			}else{
				_lostTeam1 = true;
			}
			updated = true;
		}
	}
	
	//-----------------DISPLAY RESULTS---------------------//
	if(updated){

		if(_lostTeam1)	{
			mesaj = "Team 2 scored!";
			_scoreTeam2++;
		}else{
			mesaj = "Team 1 scored!";
			_scoreTeam1++;
		}
		
		GUI::getInstance().modifyText("Team1Score","Team 1 Score: %d",_scoreTeam1);
		GUI::getInstance().modifyText("Team2Score","Team 1 Score: %d",_scoreTeam2);
		GUI::getInstance().modifyText("Message",(char*)mesaj.c_str());
		resetGame();
	}

}

void AITenisScene::processInput(){
	Scene::processInput();
	
	if(_angleLR)	_camera->RotateX(_angleLR * Framerate::getInstance().getSpeedfactor()/5);
	if(_angleUD)	_camera->RotateY(_angleUD * Framerate::getInstance().getSpeedfactor()/5);
	if(_moveFB || _moveLR){
		if(_moveFB) _camera->MoveForward(_moveFB * Framerate::getInstance().getSpeedfactor());
		if(_moveLR) _camera->MoveStrafe(_moveLR * Framerate::getInstance().getSpeedfactor());
	}
}

bool AITenisScene::load(const std::string& name){

	setInitialData();

	bool state = false;
	//Add a light
	Light* light = addDefaultLight();
	light->setLightProperties(LIGHT_AMBIENT,_white);
	light->setLightProperties(LIGHT_DIFFUSE,_white);
	light->setLightProperties(LIGHT_SPECULAR,_white);
												
	///Load scene resources
	state = loadResources(true);	
	
	//Position camera
	_camera->RotateX(RADIANS(45));
	_camera->RotateY(RADIANS(25));
	_camera->setEye(vec3<F32>(14,5.5f,11.5f));
	
	//------------------------ Load up game elements -----------------------------///
	_net = _sceneGraph->findNode("Net");
	_floor = _sceneGraph->findNode("Floor");
	_floor->getNode<SceneNode>()->getMaterial()->setCastsShadows(false);

	state = loadEvents(true);
	return state;
}

bool AITenisScene::initializeAI(bool continueOnErrors){
	bool state = false;
	//----------------------------ARTIFICIAL INTELLIGENCE------------------------------//
    _aiPlayer1 = New AIEntity("Player1");
	_aiPlayer2 = New AIEntity("Player2");
	_aiPlayer3 = New AIEntity("Player3");
	_aiPlayer4 = New AIEntity("Player4");

	_aiPlayer1->attachNode(_sceneGraph->findNode("Player1"));
	_aiPlayer2->attachNode(_sceneGraph->findNode("Player2"));
	_aiPlayer3->attachNode(_sceneGraph->findNode("Player3"));
	_aiPlayer4->attachNode(_sceneGraph->findNode("Player4"));

	_aiPlayer1->addSensor(VISUAL_SENSOR,New VisualSensor());
	_aiPlayer1->addSensor(COMMUNICATION_SENSOR, New CommunicationSensor(_aiPlayer1));
	_aiPlayer2->addSensor(VISUAL_SENSOR,New VisualSensor());
	_aiPlayer2->addSensor(COMMUNICATION_SENSOR, New CommunicationSensor(_aiPlayer2));
	_aiPlayer3->addSensor(VISUAL_SENSOR,New VisualSensor());
	_aiPlayer3->addSensor(COMMUNICATION_SENSOR, New CommunicationSensor(_aiPlayer3));
	_aiPlayer4->addSensor(VISUAL_SENSOR,New VisualSensor());
	_aiPlayer4->addSensor(COMMUNICATION_SENSOR, New CommunicationSensor(_aiPlayer4));

	_aiPlayer1->addActionProcessor(New AITenisSceneAIActionList(_ballSGN));
	_aiPlayer2->addActionProcessor(New AITenisSceneAIActionList(_ballSGN));
	_aiPlayer3->addActionProcessor(New AITenisSceneAIActionList(_ballSGN));
	_aiPlayer4->addActionProcessor(New AITenisSceneAIActionList(_ballSGN));

	_team1 = New AICoordination(1);
	_team2 = New AICoordination(2);

	_aiPlayer1->setTeam(_team1);
	state = _aiPlayer2->addFriend(_aiPlayer1);
	if(state || continueOnErrors){
		_aiPlayer3->setTeam(_team2);
		state = _aiPlayer4->addFriend(_aiPlayer3);
	}
	if(state || continueOnErrors){
		state = AIManager::getInstance().addEntity(_aiPlayer1);
	}
	if(state || continueOnErrors) {
		state = AIManager::getInstance().addEntity(_aiPlayer2);
	}
	if(state || continueOnErrors) {
		state = AIManager::getInstance().addEntity(_aiPlayer3);
	}
	if(state || continueOnErrors) {
		state = AIManager::getInstance().addEntity(_aiPlayer4);
	}
	if(state || continueOnErrors){
		//----------------------- AI controlled units (NPC's) ---------------------//
		_player1 = New NPC(_aiPlayer1);
		_player2 = New NPC(_aiPlayer2);
		_player3 = New NPC(_aiPlayer3);
		_player4 = New NPC(_aiPlayer4);

		_player1->setMovementSpeed(6);
		_player2->setMovementSpeed(6);
		_player3->setMovementSpeed(6);
		_player4->setMovementSpeed(6);
	}

	return state;
}

bool AITenisScene::deinitializeAI(bool continueOnErrors){
	SAFE_DELETE(_player1);
	SAFE_DELETE(_player2);
	SAFE_DELETE(_player3);
	SAFE_DELETE(_player4);
	SAFE_DELETE(_team1);
	SAFE_DELETE(_team2);
	return true;
}

bool AITenisScene::loadResources(bool continueOnErrors){

	///Create our ball
	ResourceDescriptor minge("Tenis Ball");
	_ball = CreateResource<Sphere3D>(minge);
	_ballSGN = addGeometry(_ball);
	_ball->setResolution(16);
	_ball->setRadius(0.3f);
	_ballSGN->getTransform()->translate(vec3<F32>(3.0f, 0.2f ,7.0f));
	_ball->getMaterial()->setDiffuse(vec4<F32>(0.4f,0.5f,0.5f,1.0f));
	_ball->getMaterial()->setAmbient(vec4<F32>(0.5f,0.5f,0.5f,1.0f));
	_ball->getMaterial()->setShininess(0.2f);
	_ball->getMaterial()->setSpecular(vec4<F32>(0.7f,0.7f,0.7f,1.0f));

	GUI::getInstance().addButton("Serve", "Serve", vec2<F32>(Application::getInstance().getWindowDimensions().width-220,
															 Application::getInstance().getWindowDimensions().height/1.1f),
													     vec2<F32>(100,25),
														 vec3<F32>(0.65f,0.65f,0.65f),
														 boost::bind(&AITenisScene::startGame,this));

	GUI::getInstance().addText("Team1Score",vec3<F32>(Application::getInstance().getWindowDimensions().width - 250,
												      Application::getInstance().getWindowDimensions().height/1.3f, 0),
											 BITMAP_8_BY_13,vec3<F32>(0,0.8f,0.8f), "Team 1 Score:: %d",0);

	GUI::getInstance().addText("Team2Score",vec3<F32>(Application::getInstance().getWindowDimensions().width - 250,
													  Application::getInstance().getWindowDimensions().height/1.5f, 0),
								             BITMAP_8_BY_13,vec3<F32>(0.2f,0.8f,0), "Team 2 Score:: %d",0);

	GUI::getInstance().addText("Message",vec3<F32>(Application::getInstance().getWindowDimensions().width - 250,
		                                           Application::getInstance().getWindowDimensions().height/1.7f, 0),
									   BITMAP_8_BY_13,vec3<F32>(0,1,0), "");

	GUI::getInstance().addText("fpsDisplay",              //Unique ID
		                       vec3<F32>(60,60,0),        //Position
							   BITMAP_8_BY_13,            //Font
							   vec3<F32>(0.0f,0.2f, 1.0f),//Color
							   "FPS: %s",0);              //Text and arguments

	GUI::getInstance().addText("RenderBinCount",
								vec3<F32>(60,70,0),
								BITMAP_8_BY_13,
								vec3<F32>(0.6f,0.2f,0.2f),
								"Number of items in Render Bin: %d",0);
	_eventTimers.push_back(0.0f); //Fps
	return true;
}

bool AITenisScene::unload(){
	Sky::getInstance().DestroyInstance();
	return Scene::unload();
}


void AITenisScene::onKeyDown(const OIS::KeyEvent& key){
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

void AITenisScene::onKeyUp(const OIS::KeyEvent& key){
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
		case OIS::KC_F1:
			_sceneGraph->print();
			break;
		default:
			break;
	}

}
void AITenisScene::onMouseMove(const OIS::MouseEvent& key){
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

void AITenisScene::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	Scene::onMouseClickDown(key,button);
	if(button == 0) 
		_mousePressed = true;
}

void AITenisScene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	Scene::onMouseClickUp(key,button);
	if(button == 0)	{
		_mousePressed = false;
		_angleUD = 0;
		_angleLR = 0;
	}
}