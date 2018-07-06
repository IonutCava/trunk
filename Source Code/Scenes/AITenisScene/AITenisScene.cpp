#include "Headers/AITenisScene.h"
#include "Headers/AITenisSceneAIActionList.h"

#include "Environment/Sky/Headers/Sky.h"
#include "Managers/Headers/CameraManager.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Headers/Frustum.h"
#include "GUI/Headers/GUI.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

using namespace std;

//begin copy-paste: randarea scenei
void AITenisScene::render(){
	Sky& sky = Sky::getInstance();

	sky.setParams(CameraManager::getInstance().getActiveCamera()->getEye(),_sunVector,false,true,true);
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
		GUI::getInstance().modifyText("RenderBinCount", "Number of items in Render Bin: %d", RenderQueue::getInstance().getRenderQueueStackSize());
		_eventTimers[0] += FpsDisplay;
	}
}

void AITenisScene::reseteazaJoc(){
	getEvents().clear();
	_directieEchipa1SpreEchipa2 = true;
	_directieAscendenta = true;
	_atinsTerenEchipa1 = false;
	_atinsTerenEchipa2 = false;
	_pierdutEchipa1 = false;
	_aplicaImpulsLateral = false;
	_impulsLateral = 0;
	_mingeSGN->getTransform()->setPosition(vec3<F32>(3.0f, 0.2f ,7.0f));
}

void AITenisScene::startJoc(){

	reseteazaJoc();

	if(getEvents().empty()){//Maxim un singur eveniment
		Event_ptr jocNou(New Event(12,true,false,boost::bind(&AITenisScene::procesareJoc,this,rand() % 5,TYPE_INTEGER)));
		addEvent(jocNou);
	}
}


//Echipa 1: Player1 + Player2
//Echipa 2: Player3 + Player4
void AITenisScene::procesareJoc(boost::any a, CallbackParam b){

	if(getEvents().empty()) return;
	bool updated = false;
	string mesaj;

	//Shortcut catre nodurile din graph aferente agentilor
	SceneGraphNode* Player1 = _sceneGraph->findNode("Player1");
	SceneGraphNode* Player2 = _sceneGraph->findNode("Player2");
	SceneGraphNode* Player3 = _sceneGraph->findNode("Player3");
	SceneGraphNode* Player4 = _sceneGraph->findNode("Player4");


	//Validarea pointerilor
	assert(Player1);assert(Player2);assert(Player3);assert(Player4);
	assert(_fileu);assert(_podea);assert(_mingeSGN);

	//Memoram (prin copie. thread-safe) pozitia actuala a mingii
	Transform* mingeT = _mingeSGN->getTransform();
	vec3<F32> pozitieMinge = mingeT->getPosition();
	vec3<F32> pozitieFileu  = _fileu->getTransform()->getPosition();
	//Mingea se deplaseaza de la Echipa 1 la Echipa 2?
	_directieEchipa1SpreEchipa2 ? pozitieMinge.z -= 0.123f : pozitieMinge.z += 0.123f;
	//Mingea se deplaseaza de jos in sus sau e in cadere?
	_directieAscendenta ? pozitieMinge.y += 0.066f : pozitieMinge.y -= 0.066f;
	//In cazul unui impul lateral, modificam si pozitia mingii deasemenea
	if(_aplicaImpulsLateral){
		pozitieMinge.x += _impulsLateral*0.025f;
	}
	//Dupa finalizarea calculelor de pozitie, aplicam transformarea
	//ToDo: mutex aici!;
	mingeT->translate(pozitieMinge - mingeT->getPosition());
	//Rotim mingea doar de efect ...
	mingeT->rotateEuler(vec3<F32>(pozitieMinge.z,1,1));

	//----------------------COLIZIUNI------------------------------//
	//z = adancime. Descendent spre orizont;
	if(_podea->getBoundingBox().Collision(_mingeSGN->getBoundingBox())){
		_aplicaImpulsLateral = true;
		if(pozitieMinge.z > pozitieFileu.z){
			_atinsTerenEchipa1 = true;
			_atinsTerenEchipa2 = false;
		}else{
			_atinsTerenEchipa1 = false;
			_atinsTerenEchipa2 = true;
		}
		_directieAscendenta = true;
	}

	//Epuizam energia cinetica la acest punct
	if(pozitieMinge.y > 3.5) _directieAscendenta = false;
	
	//Am atins un jucator?
	bool coliziunePlayer1 = _mingeSGN->getBoundingBox().Collision(Player1->getBoundingBox());
	bool coliziunePlayer2 = _mingeSGN->getBoundingBox().Collision(Player2->getBoundingBox());
	bool coliziunePlayer3 = _mingeSGN->getBoundingBox().Collision(Player3->getBoundingBox());
	bool coliziunePlayer4 = _mingeSGN->getBoundingBox().Collision(Player4->getBoundingBox());

	bool coliziuneEchipa1 = coliziunePlayer1 || coliziunePlayer2;
	bool coliziuneEchipa2 = coliziunePlayer3 || coliziunePlayer4;

	//fiecare echipa are o sansa mica de a rata lovitura
	if(coliziuneEchipa1 && random(30) != 2){
		//if(pasa) {caz special}
		F32 decalajLateral = 0;
		coliziunePlayer1 ? decalajLateral = Player1->getTransform()->getPosition().x : decalajLateral = Player2->getTransform()->getPosition().x;
		_impulsLateral = pozitieMinge.x - decalajLateral;
		_directieEchipa1SpreEchipa2 = true;
	}

	if(coliziuneEchipa2 && random(30) != 2){
		//if(pasa) {caz special}
		F32 decalajLateral = 0;
		coliziunePlayer3 ? decalajLateral = Player3->getTransform()->getPosition().x : decalajLateral = Player4->getTransform()->getPosition().x;
		_impulsLateral = pozitieMinge.x - decalajLateral;
		_directieEchipa1SpreEchipa2 = false;
	}

	//-----------------VALIDARI REZULTATE----------------------//
	//A castigat Echipa 1 sau Echipa 2?
	if(pozitieMinge.z >= Player1->getTransform()->getPosition().z &&
	   pozitieMinge.z >= Player2->getTransform()->getPosition().z){

		_pierdutEchipa1 = true;
		updated = true;
	}

	if(pozitieMinge.z <= Player3->getTransform()->getPosition().z &&
	   pozitieMinge.z <= Player4->getTransform()->getPosition().z){
		_pierdutEchipa1 = false;
		updated = true;
	}

	//A lovit Echipa 1 sau Echipa 2 fileul?
	if(_mingeSGN->getBoundingBox().Collision(_fileu->getBoundingBox())){
		if(pozitieMinge.y < _fileu->getBoundingBox().getMax().y - 1){
			if(_directieEchipa1SpreEchipa2){
				_pierdutEchipa1 = true;
			}else{
				_pierdutEchipa1 = false;
			}
			updated = true;
		}
    }
	if(pozitieMinge.x + 1 < _fileu->getBoundingBox().getMin().x || pozitieMinge.x + 1 > _fileu->getBoundingBox().getMax().x){
		//Daca am lovit noi si am atins terenul adversarului
		//Sau daca a lovit adversarul si nu a atins terenul nostru
		if(_podea->getBoundingBox().Collision(_mingeSGN->getBoundingBox())){
			if((_atinsTerenEchipa2 && _directieEchipa1SpreEchipa2) || (!_directieEchipa1SpreEchipa2 && !_atinsTerenEchipa1)){
				_pierdutEchipa1 = false;
			}else{
				_pierdutEchipa1 = true;
			}
			updated = true;
		}
	}
	
	//-----------------AFISARE REZULTATE---------------------//
	if(updated){

		if(_pierdutEchipa1)	{
			mesaj = "Echipa 2 a punctat!";
			_scorEchipa2++;
		}else{
			mesaj = "Echipa 1 a punctat!";
			_scorEchipa1++;
		}
		
		GUI::getInstance().modifyText("ScorEchipa1","Scor Echipa 1: %d",_scorEchipa1);
		GUI::getInstance().modifyText("ScorEchipa2","Scor Echipa 2: %d",_scorEchipa2);
		GUI::getInstance().modifyText("Mesaj",(char*)mesaj.c_str());
		reseteazaJoc();
	}

}

void AITenisScene::processInput(){
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

bool AITenisScene::load(const string& name){
	setInitialData();

	bool state = false;
	//Adaugam o lumina
	Light* light = addDefaultLight();
	light->setLightProperties(LIGHT_AMBIENT,_white);
	light->setLightProperties(LIGHT_DIFFUSE,_white);
	light->setLightProperties(LIGHT_SPECULAR,_white);
												
	//Incarcam resursele scenei
	state = loadResources(true);	
	
	//Pozitionam camera
	CameraManager::getInstance().getActiveCamera()->RotateX(RADIANS(45));
	CameraManager::getInstance().getActiveCamera()->RotateY(RADIANS(25));
	CameraManager::getInstance().getActiveCamera()->setEye(vec3<F32>(14,5.5f,11.5f));
	
	//----------------------------INTELIGENTA ARTIFICIALA------------------------------//
    _aiPlayer1 = New AIEntity("Player1");
	_aiPlayer2 = New AIEntity("Player2");
	_aiPlayer3 = New AIEntity("Player3");
	_aiPlayer4 = New AIEntity("Player4");

	_aiPlayer1->attachNode(_sceneGraph->findNode("Player1"));
	_aiPlayer2->attachNode(_sceneGraph->findNode("Player2"));
	_aiPlayer3->attachNode(_sceneGraph->findNode("Player3"));
	_aiPlayer4->attachNode(_sceneGraph->findNode("Player4"));
	
	
	_aiPlayer1->addFriend(_aiPlayer2);
	_aiPlayer3->addFriend(_aiPlayer4);

	_aiPlayer1->addEnemyTeam(_aiPlayer3->getTeam());
	_aiPlayer3->addEnemyTeam(_aiPlayer1->getTeam());


	_aiPlayer1->addSensor(VISUAL_SENSOR,New VisualSensor());
	_aiPlayer1->addSensor(COMMUNICATION_SENSOR, New CommunicationSensor(_aiPlayer1));
	_aiPlayer2->addSensor(VISUAL_SENSOR,New VisualSensor());
	_aiPlayer2->addSensor(COMMUNICATION_SENSOR, New CommunicationSensor(_aiPlayer2));
	_aiPlayer3->addSensor(VISUAL_SENSOR,New VisualSensor());
	_aiPlayer3->addSensor(COMMUNICATION_SENSOR, New CommunicationSensor(_aiPlayer3));
	_aiPlayer4->addSensor(VISUAL_SENSOR,New VisualSensor());
	_aiPlayer4->addSensor(COMMUNICATION_SENSOR, New CommunicationSensor(_aiPlayer4));

	_aiPlayer1->addActionProcessor(New AITenisSceneAIActionList(_mingeSGN));
	_aiPlayer2->addActionProcessor(New AITenisSceneAIActionList(_mingeSGN));
	_aiPlayer3->addActionProcessor(New AITenisSceneAIActionList(_mingeSGN));
	_aiPlayer4->addActionProcessor(New AITenisSceneAIActionList(_mingeSGN));

	_aiPlayer1->setTeamID(1);
	_aiPlayer2->setTeamID(1);
	_aiPlayer3->setTeamID(2);
	_aiPlayer4->setTeamID(2);

	AIManager::getInstance().addEntity(_aiPlayer1);
	AIManager::getInstance().addEntity(_aiPlayer2);
	AIManager::getInstance().addEntity(_aiPlayer3);
	AIManager::getInstance().addEntity(_aiPlayer4);

	//----------------------- Unitati ce vor fi controlate de AI ---------------------//
	_player1 = New NPC(_aiPlayer1);
	_player2 = New NPC(_aiPlayer2);
	_player3 = New NPC(_aiPlayer3);
	_player4 = New NPC(_aiPlayer4);

	_player1->setMovementSpeed(2); /// 2 m/s
	_player2->setMovementSpeed(2); /// 2 m/s
	_player3->setMovementSpeed(2); /// 2 m/s
	_player4->setMovementSpeed(2); /// 2 m/s

	//------------------------ Restul elementelor jocului -----------------------------///
	_fileu = _sceneGraph->findNode("Fileu");
	_podea = _sceneGraph->findNode("Podea");
	_podea->getNode()->getMaterial()->setCastsShadows(false);

	state = loadEvents(true);
	return state;
}

bool AITenisScene::loadResources(bool continueOnErrors){
	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;_scor = 0;

	//Cream o minge (Sa o facem din Chrome?)
	ResourceDescriptor minge("Minge Tenis");
	_minge = CreateResource<Sphere3D>(minge);
	_mingeSGN = addGeometry(_minge);
	_minge->setResolution(16);
	_minge->setRadius(0.3f);
	_mingeSGN->getTransform()->translate(vec3<F32>(3.0f, 0.2f ,7.0f));
	_minge->getMaterial()->setDiffuse(vec4<F32>(0.4f,0.5f,0.5f,1.0f));
	_minge->getMaterial()->setAmbient(vec4<F32>(0.5f,0.5f,0.5f,1.0f));
	_minge->getMaterial()->setShininess(0.2f);
	_minge->getMaterial()->setSpecular(vec4<F32>(0.7f,0.7f,0.7f,1.0f));

	GUI::getInstance().addButton("Serveste", "Serveste", vec2<F32>(Application::getInstance().getWindowDimensions().width-220 ,
															 Application::getInstance().getWindowDimensions().height/1.1f),
													    	 vec2<F32>(100,25),vec3<F32>(0.65f,0.65f,0.65f),
															 boost::bind(&AITenisScene::startJoc,this));

	GUI::getInstance().addText("ScorEchipa1",vec3<F32>(Application::getInstance().getWindowDimensions().width - 250, Application::getInstance().getWindowDimensions().height/1.3f, 0),
								BITMAP_8_BY_13,vec3<F32>(0,0.8f,0.8f), "Scor Echipa 1: %d",0);
	GUI::getInstance().addText("ScorEchipa2",vec3<F32>(Application::getInstance().getWindowDimensions().width - 250, Application::getInstance().getWindowDimensions().height/1.5f, 0),
								BITMAP_8_BY_13,vec3<F32>(0.2f,0.8f,0), "Scor Echipa 2: %d",0);
	GUI::getInstance().addText("Mesaj",vec3<F32>(Application::getInstance().getWindowDimensions().width - 250, Application::getInstance().getWindowDimensions().height/1.7f, 0),
							   BITMAP_8_BY_13,vec3<F32>(0,1,0), "");
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

bool AITenisScene::unload(){
	Sky::getInstance().DestroyInstance();
	return Scene::unload();
}


void AITenisScene::onKeyDown(const OIS::KeyEvent& key){
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

void AITenisScene::onKeyUp(const OIS::KeyEvent& key){
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
void AITenisScene::onMouseMove(const OIS::MouseEvent& key){
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

void AITenisScene::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	Scene::onMouseClickDown(key,button);
	if(button == 0) 
		_mousePressed = true;
}

void AITenisScene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	Scene::onMouseClickUp(key,button);
	if(button == 0)	{
		_mousePressed = false;
		Application::getInstance().angleUD = 0;
		Application::getInstance().angleLR = 0;
	}
}