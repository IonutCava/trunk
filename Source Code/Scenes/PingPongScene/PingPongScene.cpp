#include "Headers/PingPongScene.h"

#include "Managers/Headers/CameraManager.h"
#include "Core/Headers/Application.h"
#include "Rendering/Headers/Frustum.h"
#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Environment/Sky/Headers/Sky.h"
#include "GUI/Headers/GUI.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
using namespace std;

vec4 _lightPosition(0,6,6,1.0);

//begin copy-paste: randarea scenei
void PingPongScene::render(){

	Sky& sky = Sky::getInstance();
	//set a placeholder sun for estethic reasons
	vec2 sunAngle(0.0f, RADIANS(45.0f));
	_sunVector = vec4(-cosf(sunAngle.x) * sinf(sunAngle.y),-cosf(sunAngle.y),-sinf(sunAngle.x) * sinf(sunAngle.y),0.0f );
	sky.setParams(CameraManager::getInstance().getActiveCamera()->getEye(),_sunVector, false,true,true);
	sky.draw();

	_sceneGraph->render();
}
//end copy-paste

//begin copy-paste: Desenam un cer standard
bool _switchLightUpDown = true;
void PingPongScene::preRender(){
	Light* light = LightManager::getInstance().getLight(0);
	//_GFX.getActiveRenderState().lightingEnabled() = false;
	_switchLightUpDown ? _lightPosition.y += 0.01f : _lightPosition.y -= 0.01f;
	if(_lightPosition.y >= 10 ) _switchLightUpDown = false;
	if(_lightPosition.y <= 1 ) _switchLightUpDown = true;
	light->setLightProperties("position",_lightPosition);
	light->setLightProperties("spotDirection",-_lightPosition);
}
//<<end copy-paste

void PingPongScene::processEvents(F32 time){

	F32 FpsDisplay = 0.3f;
	if (time - _eventTimers[0] >= FpsDisplay){
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		_eventTimers[0] += FpsDisplay;
	}
}

void PingPongScene::reseteazaJoc(){
	_directieAdversar = true;
	_directieSus = false;
	_atinsTerenAdversar = false;
	_atinsTeren = false;
	_pierdut = false;
	_miscareLaterala = 0;
	getEvents().clear();
	_mingeSGN->getTransform()->setPosition(vec3(0, 2 ,2));
}

void PingPongScene::servesteMingea(){
	GUI::getInstance().modifyText("insulte","");
	reseteazaJoc();

	if(getEvents().empty()){//Maxim un singur eveniment
		Event_ptr jocNou(New Event(30,true,false,boost::bind(&PingPongScene::test,this,rand() % 5,TYPE_INTEGER)));
		addEvent(jocNou);
	}
}

void PingPongScene::test(boost::any a, CallbackParam b){
	if(getEvents().empty()) return;
	bool updated = false;
	string mesaj;
	Transform* mingeT = _mingeSGN->getTransform();
	vec3 pozitieMinge  = mingeT->getPosition();

	
	SceneGraphNode* masa = _sceneGraph->findNode("masa");
	SceneGraphNode* plasa = _sceneGraph->findNode("plasa");
	SceneGraphNode* perete = _sceneGraph->findNode("perete");
	SceneGraphNode* paleta = _sceneGraph->findNode("paleta");
	vec3 pozitiePaleta   = paleta->getTransform()->getPosition();
	vec3 pozitieAdversar = perete->getTransform()->getPosition();
	vec3 pozitieMasa     = masa->getTransform()->getPosition();
	//Miscare minge si detectie coliziuni


	//Mergem spre adversar sau spre noi
	_directieAdversar ? pozitieMinge.z -= 0.11f : pozitieMinge.z += 0.11f;
	//Sus sau jos?
	_directieSus ? 	pozitieMinge.y += 0.084f : 	pozitieMinge.y -= 0.084f;


	//Mingea se deplaseaza spre stanga sau spre dreapta?
	pozitieMinge.x += _miscareLaterala*0.15f;
	if(pozitieAdversar.x != pozitieMinge.x)
		perete->getTransform()->translateX(pozitieMinge.x - pozitieAdversar.x);

	mingeT->translate(pozitieMinge - mingeT->getPosition());


	//Am lovit masa? Ricosam
	if(masa->getBoundingBox().Collision(_mingeSGN->getBoundingBox())){
		if(pozitieMinge.z > pozitieMasa.z){
			_atinsTeren = true;
			_atinsTerenAdversar = false;
		}else{
			_atinsTeren = false;
			_atinsTerenAdversar = true;
		}
		_directieSus = true;
	}
	//Epuizam energia cinetica la acest punct
	if(pozitieMinge.y > 2.1f) _directieSus = false;

	//Am lovit paleta?
	if(_mingeSGN->getBoundingBox().Collision(paleta->getBoundingBox())){
		_miscareLaterala = pozitieMinge.x - pozitiePaleta.x;
		//Dca am lovit cu partea de sus a paletei, mai atasam un mic impuls miscarii mingii
		if(pozitieMinge.y >= pozitiePaleta.y) pozitieMinge.z -= 0.12f;

		_directieAdversar = true;
	}

	if(pozitieMinge.y + 0.75f < masa->getBoundingBox().getMax().y){
		//Daca am lovit noi si am atins terenul adversarului
		//Sau daca a lovit adversarul si nu a atins terenul nostrue
		if((_atinsTerenAdversar && _directieAdversar) || (!_directieAdversar && !_atinsTeren))		
			_pierdut = false;
		else
			_pierdut = true;
		
		updated = true;
	}
	//Am castigat sau am pierdut?
	if(pozitieMinge.z >= pozitiePaleta.z){
		_pierdut = true;
		updated = true;
	}
	if(pozitieMinge.z <= pozitieAdversar.z){
		_pierdut = false;
		updated = true;
	}

	if(_mingeSGN->getBoundingBox().Collision(plasa->getBoundingBox())){
		if(_directieAdversar){
			//Am lovit plasa?
			_pierdut = true;
		}else{
			//A lovit adversarul plasa
			_pierdut = false;
		}
		updated = true;
		
    }
	
	//Am lovit adversarul? Ne intoarcem ... DAR
	//... adaugam o mica sansa sa castigam meciul .. dar mica ...
	if(random(30) != 2)
	if(_mingeSGN->getBoundingBox().Collision(perete->getBoundingBox())){
		_miscareLaterala = pozitieMinge.x - perete->getTransform()->getPosition().x;
		_directieAdversar = false;
	}
	//Rotim mingea doar de efect ...
	mingeT->rotateEuler(vec3(pozitieMinge.z,1,1));

	if(updated){
		if(_pierdut){
			mesaj = "Ai pierdut!";
			_scor--;

			if(b == TYPE_INTEGER){
				I32 quote = boost::any_cast<I32>(a);
				if(_scor % 3 == 0 ) GUI::getInstance().modifyText("insulte",(char*)_quotes[quote].c_str());
			}
		}else{
			mesaj = "Ai castigat!";
			_scor++;
		}
		
		GUI::getInstance().modifyText("Scor","Scor: %d",_scor);
		GUI::getInstance().modifyText("Mesaj",(char*)mesaj.c_str());
		reseteazaJoc();
	}
}

void PingPongScene::processInput(){
	Scene::processInput();

	Camera* cam = CameraManager::getInstance().getActiveCamera();
	//Move FB = Forward/Back = sus/jos
	//Move LR = Left/Right = stanga/dreapta
	moveFB  = Application::getInstance().moveFB;
	moveLR  = Application::getInstance().moveLR;

	//Miscarea camerei
	angleLR = Application::getInstance().angleLR;
	angleUD = Application::getInstance().angleUD;

	if(angleLR)	cam->RotateX(angleLR * Framerate::getInstance().getSpeedfactor());
	if(angleUD)	cam->RotateY(angleUD * Framerate::getInstance().getSpeedfactor());

	SceneGraphNode* paleta = _sceneGraph->findNode("paleta");

	vec3 pos = paleta->getTransform()->getPosition();

	//Miscarea pe ambele directii de deplasare este limitata in intervalul [-3,3] cu exceptia coborarii pe Y;
	if(moveFB){
		if((moveFB > 0 && pos.y >= 3) || (moveFB < 0 && pos.y <= 0.5f)) return;
		paleta->getTransform()->translateY((moveFB * Framerate::getInstance().getSpeedfactor())/6);
	}

	if(moveLR){
		//Miscarea stanga dreapta necesita inversarea directiei de deplasare.
		if((moveLR < 0 && pos.x >= 3) || (moveLR > 0 && pos.x <= -3)) return;
		paleta->getTransform()->translateX((-moveLR * Framerate::getInstance().getSpeedfactor())/6);
	}
}

bool PingPongScene::load(const string& name){
	bool state = false;
	//Adaugam o lumina
	Light* light = addDefaultLight();
	light->setRadius(0.1f);
	light->setDrawImpostor(false);
	light->setLightType(LIGHT_SPOT);
	//Incarcam resursele scenei
	state = loadResources(true);	
	state = loadEvents(true);
	//Pozitionam camera
	CameraManager::getInstance().getActiveCamera()->setAngleX(RADIANS(-90));
	CameraManager::getInstance().getActiveCamera()->setEye(vec3(0,2.5f,6.5f));
	
	return state;
}

bool PingPongScene::loadResources(bool continueOnErrors){

	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;_scor = 0;

	//Cream o minge (Sa o facem din Chrome?)
	ResourceDescriptor minge("Minge Ping Pong");
	_minge = _resManager.loadResource<Sphere3D>(minge);
	_mingeSGN = addGeometry(_minge);
	_minge->setResolution(16);
	_minge->setRadius(0.1f);
	_mingeSGN->getTransform()->translate(vec3(0, 2 ,2));
	_minge->getMaterial()->setDiffuse(vec4(0.4f,0.4f,0.4f,1.0f));
	_minge->getMaterial()->setAmbient(vec4(0.25f,0.25f,0.25f,1.0f));
	_minge->getMaterial()->setShininess(36.8f);
	_minge->getMaterial()->setSpecular(vec4(0.774597f,0.774597f,0.774597f,1.0f));

	//Adaugam butoane si text labels
	GUI::getInstance().addButton("Serveste", "Serveste", vec2(Application::getInstance().getWindowDimensions().width-120 ,
															 Application::getInstance().getWindowDimensions().height/1.1f),
													    	 vec2(100,25),vec3(0.65f,0.65f,0.65f),
															 boost::bind(&PingPongScene::servesteMingea,this));

	GUI::getInstance().addText("Scor",vec3(Application::getInstance().getWindowDimensions().width - 120, Application::getInstance().getWindowDimensions().height/1.3f, 0),
							   BITMAP_8_BY_13,vec3(1,0,0), "Scor: %d",0);

	GUI::getInstance().addText("Mesaj",vec3(Application::getInstance().getWindowDimensions().width - 120, Application::getInstance().getWindowDimensions().height/1.5f, 0),
							   BITMAP_8_BY_13,vec3(1,0,0), "");
	GUI::getInstance().addText("insulte",vec3(Application::getInstance().getWindowDimensions().width/4, Application::getInstance().getWindowDimensions().height/3, 0),
							   BITMAP_TIMES_ROMAN_24,vec3(0,1,0), "");
	GUI::getInstance().addText("fpsDisplay",           //Unique ID
		                       vec3(60,60,0),          //Position
							   BITMAP_8_BY_13,    //Font
							   vec3(0.0f,0.2f, 1.0f),  //Color
							   "FPS: %s",0);    //Text and arguments
	//Cateva replici motivationale
	_quotes.push_back("Ha ha ... pana si Odin rade de tine!");
	_quotes.push_back("Daca tu esti jucator de ping-pong, eu sunt Jimmy Page");
	_quotes.push_back("Ooolee, ole ole ole, uite mingea ... nu mai e");
	_quotes.push_back("Ai noroc ca sala-i goala, ca eu unul as fi murit de rusine");
	_quotes.push_back("Nu e atat de greu; si o maimuta poate juca jocul asta.");
	
	_eventTimers.push_back(0.0f); //Fps
	_eventTimers.push_back(0.0f); //Light
	return true;
}

bool PingPongScene::unload(){
	Sky::getInstance().DestroyInstance();
	return Scene::unload();
}

void PingPongScene::onKeyDown(const OIS::KeyEvent& key){

	Scene::onKeyDown(key);
	switch(key.key){

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

void PingPongScene::onKeyUp(const OIS::KeyEvent& key){

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
		default:
			break;
	}

}

void PingPongScene::OnJoystickMovePOV(const OIS::JoyStickEvent& key,I8 pov){

	Scene::OnJoystickMovePOV(key,pov);
	if( key.state.mPOV[pov].direction & OIS::Pov::North ) //Going up
		Application::getInstance().moveFB = 0.25f;
	else if( key.state.mPOV[pov].direction & OIS::Pov::South ) //Going down
		Application::getInstance().moveFB = -0.25f;

	if( key.state.mPOV[pov].direction & OIS::Pov::East ) //Going right
		Application::getInstance().moveLR = -0.25f;

	else if( key.state.mPOV[pov].direction & OIS::Pov::West ) //Going left
		Application::getInstance().moveLR = 0.25f;

	if( key.state.mPOV[pov].direction == OIS::Pov::Centered ){ //stopped/centered out
		Application::getInstance().moveLR = 0;
		Application::getInstance().moveFB = 0;
	}
}

void PingPongScene::OnJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis){

	Scene::OnJoystickMoveAxis(key,axis);
}

void PingPongScene::OnJoystickButtonUp(const OIS::JoyStickEvent& key, I8 button){

	if(button == 0)  servesteMingea();
}