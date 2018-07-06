#include "AITenisScene.h"
#include "Terrain/Sky.h"
#include "Managers/CameraManager.h"
#include "PhysX/PhysX.h"
#include "Rendering/Frustum.h"
#include "GUI/GUI.h"
#include "Geometry/Predefined/Sphere3D.h"
using namespace std;

//begin copy-paste: randarea scenei
void AITenisScene::render(){
	Sky& sky = Sky::getInstance();

	sky.setParams(CameraManager::getInstance().getActiveCamera()->getEye(),_sunVector, false,true,true);
	sky.draw();

	_sceneGraph->render();
}
//end copy-paste

//begin copy-paste: Desenam un cer standard

void AITenisScene::preRender(){
	Camera* cam = CameraManager::getInstance().getActiveCamera();

	vec2 sunAngle(0.0f, RADIANS(45.0f));
	_sunVector = vec4(-cosf(sunAngle.x) * sinf(sunAngle.y),-cosf(sunAngle.y),-sinf(sunAngle.x) * sinf(sunAngle.y),0.0f );
	getLights()[0]->setLightProperties(string("position"),_sunVector);
	getLights()[0]->setLightProperties(string("ambient"),vec4(1.0f,1.0f,1.0f,1.0f));
	getLights()[0]->setLightProperties(string("diffuse"),vec4(1.0f,1.0f,1.0f,1.0f));
	getLights()[0]->setLightProperties(string("specular"),vec4(1.0f,1.0f,1.0f,1.0f));
	getLights()[0]->onDraw(); //Force update between render passes


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
			_sceneGraph->render();
		_depthMap[i]->End();
	}

	_GFX.setOrthoProjection(vec4(-1.0f,1.0f,-1.0f,1.0f),zPlanes);
	Frustum::getInstance().Extract(sun_pos);
	_GFX.setLightProjectionMatrix(Frustum::getInstance().getModelViewProjectionMatrix());

	_GFX.setDepthMapRendering(false);
	_GFX.restoreLightCameraMatrices(true);
	
	Frustum::getInstance().Extract(eye_pos);
	
}
//<<end copy-paste

void AITenisScene::processEvents(F32 time){
	F32 FpsDisplay = 0.3f;
	if (time - _eventTimers[0] >= FpsDisplay)
	{
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
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
	_mingeSGN->getTransform()->setPosition(vec3(3.2f, 0.2f ,7.0f));
}

void AITenisScene::startJoc(){

	reseteazaJoc();

	if(getEvents().empty()){//Maxim un singur eveniment
		Event_ptr jocNou(New Event(10,true,false,boost::bind(&AITenisScene::procesareJoc,this,rand() % 5,TYPE_INTEGER)));
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

	//Shortcut catre elementele din graph aferente spatiului de joc
	SceneGraphNode* Fileu = _sceneGraph->findNode("Fileu");
	SceneGraphNode* Podea = _sceneGraph->findNode("Podea");

	//Validarea pointerilor
	assert(Player1);assert(Player2);assert(Player3);assert(Player4);
	assert(Fileu);assert(Podea);assert(_mingeSGN);

	//Memoram (prin copie. thread-safe) pozitia actuala a mingii
	Transform* mingeT = _mingeSGN->getTransform();
	vec3 pozitieMinge = mingeT->getPosition();
	vec3 pozitieFileu  = Fileu->getTransform()->getPosition();
	//Mingea se deplaseaza de la Echipa 1 la Echipa 2?
	_directieEchipa1SpreEchipa2 ? pozitieMinge.z -= 0.123f : pozitieMinge.z += 0.123f;
	//Mingea se deplaseaza de jos in sus sau e in cadere?
	_directieAscendenta ? pozitieMinge.y += 0.065f : pozitieMinge.y -= 0.065f;
	//In cazul unui impul lateral, modificam si pozitia mingii deasemenea
	if(_aplicaImpulsLateral){
		//pozitieMinge.x += _impulsLateral*0.05f;
	}
	//Dupa finalizarea calculelor de pozitie, aplicam transformarea
	//ToDo: mutex aici!;
	mingeT->translate(pozitieMinge - mingeT->getPosition());
	//Rotim mingea doar de efect ...
	mingeT->rotateEuler(vec3(pozitieMinge.z,1,1));

	//----------------------COLIZIUNI------------------------------//
	//z = adancime. Descendent spre orizont;
	if(Podea->getBoundingBox().Collision(_mingeSGN->getBoundingBox())){
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
	if(_mingeSGN->getBoundingBox().Collision(Fileu->getBoundingBox())){
		if(_directieEchipa1SpreEchipa2){
			_pierdutEchipa1 = true;
		}else{
			_pierdutEchipa1 = false;
		}
		updated = true;
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
	_inputManager.tick();

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
	bool state = false;
	//Adaugam o lumina
	addDefaultLight();
	//Incarcam resursele scenei
	state = loadResources(true);	
	state = loadEvents(true);
	_depthMap[0] =_GFX.newFBO();
	_depthMap[0]->Create(FrameBufferObject::FBO_2D_DEPTH,2048,2048);
	_depthMap[1] = _GFX.newFBO();
	_depthMap[1]->Create(FrameBufferObject::FBO_2D_DEPTH,2048,2048);
	//Pozitionam camera
	CameraManager::getInstance().getActiveCamera()->RotateX(RADIANS(45));
	CameraManager::getInstance().getActiveCamera()->RotateY(RADIANS(25));
	CameraManager::getInstance().getActiveCamera()->setEye(vec3(14,5.5f,11.5f));
	
	return state;
}

bool AITenisScene::loadResources(bool continueOnErrors){
	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;_scor = 0;

	//Cream o minge (Sa o facem din Chrome?)
	ResourceDescriptor minge("Minge Tenis");
	_minge = _resManager.loadResource<Sphere3D>(minge);
	_mingeSGN = addGeometry(_minge);
	_minge->getResolution() = 16;
	_minge->getSize() = 0.3f;
	_mingeSGN->getTransform()->translate(vec3(3.2f, 0.2f ,7.0f));
	_minge->getMaterial()->setDiffuse(vec4(0.4f,0.5f,0.5f,1.0f));
	_minge->getMaterial()->setAmbient(vec4(0.5f,0.5f,0.5f,1.0f));
	_minge->getMaterial()->setShininess(0.2f);
	_minge->getMaterial()->setSpecular(vec4(0.7f,0.7f,0.7f,1.0f));

	GUI::getInstance().addButton("Serveste", "Serveste", vec2(Application::getInstance().getWindowDimensions().width-220 ,
															 Application::getInstance().getWindowDimensions().height/1.1f),
													    	 vec2(100,25),vec3(0.65f,0.65f,0.65f),
															 boost::bind(&AITenisScene::startJoc,this));

	GUI::getInstance().addText("ScorEchipa1",vec3(Application::getInstance().getWindowDimensions().width - 250, Application::getInstance().getWindowDimensions().height/1.3f, 0),
								BITMAP_8_BY_13,vec3(0,0.5,0.5), "Scor Echipa 1: %d",0);
	GUI::getInstance().addText("ScorEchipa2",vec3(Application::getInstance().getWindowDimensions().width - 250, Application::getInstance().getWindowDimensions().height/1.5f, 0),
								BITMAP_8_BY_13,vec3(0.5,0.5,0), "Scor Echipa 2: %d",0);
	GUI::getInstance().addText("Mesaj",vec3(Application::getInstance().getWindowDimensions().width - 250, Application::getInstance().getWindowDimensions().height/1.7f, 0),
							   BITMAP_8_BY_13,vec3(1,0,0), "");
	GUI::getInstance().addText("fpsDisplay",           //Unique ID
		                       vec3(60,60,0),          //Position
							   BITMAP_8_BY_13,    //Font
							   vec3(0.0f,0.2f, 1.0f),  //Color
							   "FPS: %s",0);    //Text and arguments

	
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