#include "PingPongScene.h"
#include "Managers/CameraManager.h"
#include "Managers/ResourceManager.h"
#include "Rendering/common.h"
#include "PhysX/PhysX.h"
#include "Importer/DVDConverter.h"
#include "Terrain/Sky.h"
#include "GUI/GUI.h"
#include "Geometry/Predefined/Box3D.h"
#include "Geometry/Predefined/Sphere3D.h"
using namespace std;

vec4 _lightPosition(0,10,2,1.0);

//begin copy-paste: randarea scenei
void PingPongScene::render()
{
	RenderState s(true,true,true,true);
	GFXDevice::getInstance().setRenderState(s);

	//Randam toata geometria
	GFXDevice::getInstance().renderElements(GeometryArray);
	getLights()[0]->draw();
	GUI::getInstance().draw();

	getLights()[0]->setLightProperties(string("position"),_lightPosition);
	
}
//end copy-paste

//begin copy-paste: Desenam un cer standard
void PingPongScene::preRender()
{
	
	vec2 _sunAngle = vec2(0.0f, RADIANS(45.0f));
	vec4 _sunVector = vec4(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );
	Sky& sky = Sky::getInstance();
	sky.setParams(CameraManager::getInstance().getActiveCamera()->getEye(),_sunVector,false,true,true);
	sky.draw();
	if(PhysX::getInstance().getScene() != NULL)	PhysX::getInstance().UpdateActors();
}
//<<end copy-paste

void PingPongScene::servesteMingea()
{
	GUI::getInstance().modifyText("insulte","");	
	getEvents().clear();

	if(getEvents().empty())//Maxim un singur eveniment
	{
		_minge->getTransform()->setPosition(vec3(0, 2 ,2));
		Event_ptr jocNou(new Event(30,true,false,boost::bind(&PingPongScene::test,this,rand() % 5,TYPE_INTEGER)));
		addEvent(jocNou);
	}
}


void PingPongScene::processEvents(F32 time)
{
	F32 FpsDisplay = 0.3f;
	if (time - _eventTimers[0] >= FpsDisplay)
	{
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		_eventTimers[0] += FpsDisplay;
	}
}

bool _directieAdversar = true;
bool _directieSus = false;
bool _atinsTeren = false;
bool _atinsTerenAdversar = false;
bool _pierdut = false;
F32 _miscareLaterala = 0;

void PingPongScene::test(boost::any a, CallbackParam b)
{
	if(getEvents().empty()) return;
	bool updated = false;
	string mesaj;
	boost::mutex::scoped_lock l(_mutex);
	vec3 pozitieMinge    = _minge->getTransform()->getPosition();
	vec3 pozitiePaleta   = GeometryArray["paleta"]->getTransform()->getPosition();
	vec3 pozitieMasa     = GeometryArray["masa"]->getTransform()->getPosition();
	vec3 pozitieAdversar = GeometryArray["perete"]->getTransform()->getPosition();
	//Miscare minge si detectie coliziuni

	if(pozitieMinge.y + 0.75f < GeometryArray["masa"]->getBoundingBox().getMax().y)
	{
		//Daca am lovit noi si am atins terenul adversarului
		//Sau daca a lovit adversarul si nu a atins terenul nostrue
		if((_atinsTerenAdversar && _directieAdversar) || (!_directieAdversar && !_atinsTeren))		
			_pierdut = false;
		else
			_pierdut = true;
		
		updated = true;
		getEvents()[0]->stopEvent();
	}
	//Am castigat sau am pierdut?
	if(pozitieMinge.z >= pozitiePaleta.z)
	{
		_pierdut = true;
		updated = true;
		getEvents()[0]->stopEvent();
	}
	if(pozitieMinge.z <= pozitieAdversar.z)
	{
		_pierdut = false;
		updated = true;
		getEvents()[0]->stopEvent();
	}

	//Mergem spre adversar sau spre noi
	_directieAdversar ? pozitieMinge.z -= 0.10f : pozitieMinge.z += 0.10f;
	//Sus sau jos?
	_directieSus ? 	pozitieMinge.y += 0.074f : 	pozitieMinge.y -= 0.074f;


	//Mingea se deplaseaza spre stanga sau spre dreapta?
	pozitieMinge.x += _miscareLaterala*0.15f;
	if(pozitieAdversar.x != pozitieMinge.x)
		GeometryArray["perete"]->getTransform()->translateX(pozitieMinge.x - pozitieAdversar.x);

	_minge->getTransform()->translate(pozitieMinge - _minge->getTransform()->getPosition());


	//Am lovit masa? Ricosam
	if(GeometryArray["masa"]->getBoundingBox().Collision(_minge->getBoundingBox()))
	{
		if(pozitieMinge.z > pozitieMasa.z) 
		{
			_atinsTeren = true;
			_atinsTerenAdversar = false;
		}
		else
		{
			_atinsTeren = false;
			_atinsTerenAdversar = true;
		}
		_directieSus = true;
	}
	//Epuizam energia cinetica la acest punct
	if(pozitieMinge.y > 2.1f) _directieSus = false;

	//Am lovit paleta?
	if(_minge->getBoundingBox().Collision(GeometryArray["paleta"]->getBoundingBox()))
	{
		_miscareLaterala = pozitieMinge.x - pozitiePaleta.x;
		//Dca am lovit cu partea de sus a paletei, mai atasam un mic impuls miscarii mingii
		if(pozitieMinge.y >= pozitiePaleta.y) pozitieMinge.z -= 0.11f;

		_directieAdversar = true;
	}

	//Am lovit plasa?
	if(_minge->getBoundingBox().Collision(GeometryArray["plasa"]->getBoundingBox()) &&	_directieAdversar)
	{
		_pierdut = true;
		updated = true;
		getEvents()[0]->stopEvent();
    }
	//A lovit adversarul plasa
	if(_minge->getBoundingBox().Collision(GeometryArray["plasa"]->getBoundingBox()) &&	!_directieAdversar)
	{
		_pierdut = false;
		updated = true;
		getEvents()[0]->stopEvent();
    }
	//Am lovit adversarul? Ne intoarcem ... DAR
	//... adaugam o mica sansa sa castigam meciul .. dar mica ...
	if(random(30) != 2)
	if(_minge->getBoundingBox().Collision(GeometryArray["perete"]->getBoundingBox()))
	{
		_miscareLaterala = pozitieMinge.x - GeometryArray["perete"]->getTransform()->getPosition().x;
		_directieAdversar = false;
	}
	//Rotim mingea doar de efect ...
	_minge->getTransform()->rotateEuler(vec3(pozitieMinge.z,1,1));

	if(updated)
	{
		if(_pierdut)
		{
			mesaj = "Ai pierdut!";
			_scor--;

			if(b == TYPE_INTEGER)
			{
				I32 quote = boost::any_cast<I32>(a);
				if(_scor % 3 == 0 ) GUI::getInstance().modifyText("insulte",(char*)_quotes[quote].c_str());
			}
		}
		else
		{
			mesaj = "Ai castigat!";
			_scor++;

		}
		_directieAdversar = true;
		_directieSus = false;
		_atinsTerenAdversar = false;
		_atinsTeren = false;
		_pierdut = false;
		_miscareLaterala = 0;
		GUI::getInstance().modifyText("Scor","Scor: %d",_scor);
		GUI::getInstance().modifyText("Mesaj",(char*)mesaj.c_str());
	}
}


void PingPongScene::processInput()
{
	_inputManager.tick();
	Camera* cam = CameraManager::getInstance().getActiveCamera();
	//Move FB = Forward/Back = sus/jos
	//Move LR = Left/Right = stanga/dreapta
	moveFB  = Engine::getInstance().moveFB;
	moveLR  = Engine::getInstance().moveLR;

	//Miscarea camerei
	angleLR = Engine::getInstance().angleLR;
	angleUD = Engine::getInstance().angleUD;

	if(angleLR)	cam->RotateX(angleLR * Framerate::getInstance().getSpeedfactor());
	if(angleUD)	cam->RotateY(angleUD * Framerate::getInstance().getSpeedfactor());

	vec3 pos = GeometryArray["paleta"]->getTransform()->getPosition();
	//Miscarea pe ambele directii de deplasare este limitata in intervalul [-3,3] cu exceptia coborarii pe Y;
	if(moveFB)
	{
		if((moveFB > 0 && pos.y >= 3) || (moveFB < 0 && pos.y <= 0.5f)) return;
		GeometryArray["paleta"]->getTransform()->translateY((moveFB * Framerate::getInstance().getSpeedfactor())/6);
		
	}
	if(moveLR)
	{
		//Miscarea stanga dreapta necesita inversarea directiei de deplasare.
		if((moveLR < 0 && pos.x >= 3) || (moveLR > 0 && pos.x <= -3)) return;
		GeometryArray["paleta"]->getTransform()->translateX((-moveLR * Framerate::getInstance().getSpeedfactor())/6);
	}
}

bool PingPongScene::load(const string& name)
{
	bool state = false;
	//Adaugam o lumina
	addDefaultLight();
	getLights()[0]->getRadius() = 0.1f;
	//Incarcam resursele scenei
	state = loadResources(true);	
	state = loadEvents(true);

	//Pozitionam camera
	CameraManager::getInstance().getActiveCamera()->setAngleX(RADIANS(-90));
	CameraManager::getInstance().getActiveCamera()->setEye(vec3(0,2.5f,6.5f));
	
	return state;
}

bool PingPongScene::loadResources(bool continueOnErrors)
{
	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;_scor = 0;

		//Cream o minge
	_minge = ResourceManager::getInstance().LoadResource<Sphere3D>("Minge");
	_minge->getResolution() = 16;
	_minge->getSize() = 0.1f;
	_minge->getTransform()->translate(vec3(0, 2 ,2));
	_minge->getTransform()->scale(vec3(1, 1 ,1));
	_minge->getTransform()->rotateEuler(vec3(0, 0 ,0));
	_minge->getMaterial().diffuse = vec3(0.5f,0.2f,0.2f);
	addGeometry(_minge);

	//Adaugam butoane si text labels
	GUI::getInstance().addButton("Serveste", "Serveste", vec2(Engine::getInstance().getWindowDimensions().width-120 ,
															 Engine::getInstance().getWindowDimensions().height/1.1f),
													    	 vec2(100,25),vec3(0.65f,0.65f,0.65f),
															 boost::bind(&PingPongScene::servesteMingea,this));

	GUI::getInstance().addText("Scor",vec3(Engine::getInstance().getWindowDimensions().width - 120, Engine::getInstance().getWindowDimensions().height/1.3f, 0),
							   BITMAP_8_BY_13,vec3(1,0,0), "Scor: %d",0);

	GUI::getInstance().addText("Mesaj",vec3(Engine::getInstance().getWindowDimensions().width - 120, Engine::getInstance().getWindowDimensions().height/1.5f, 0),
							   BITMAP_8_BY_13,vec3(1,0,0), "");
	GUI::getInstance().addText("insulte",vec3(Engine::getInstance().getWindowDimensions().width/4, Engine::getInstance().getWindowDimensions().height/3, 0),
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

void PingPongScene::onKeyDown(const OIS::KeyEvent& key)
{
	Scene::onKeyDown(key);
	switch(key.key)
	{
		case OIS::KC_W:
			Engine::getInstance().moveFB = 0.25f;
			break;
		case OIS::KC_A:
			Engine::getInstance().moveLR = 0.25f;
			break;
		case OIS::KC_S:
			Engine::getInstance().moveFB = -0.25f;
			break;
		case OIS::KC_D:
			Engine::getInstance().moveLR = -0.25f;
			break;
		default:
			break;
	}
}

void PingPongScene::onKeyUp(const OIS::KeyEvent& key)
{
	Scene::onKeyUp(key);
	switch(key.key)
	{
		case OIS::KC_W:
		case OIS::KC_S:
			Engine::getInstance().moveFB = 0;
			break;
		case OIS::KC_A:
		case OIS::KC_D:
			Engine::getInstance().moveLR = 0;
			break;
		default:
			break;
	}

}

void PingPongScene::OnJoystickMovePOV(const OIS::JoyStickEvent& key,I8 pov)
{
	Scene::OnJoystickMovePOV(key,pov);
	if( key.state.mPOV[pov].direction & OIS::Pov::North ) //Going up
		Engine::getInstance().moveFB = 0.25f;
	else if( key.state.mPOV[pov].direction & OIS::Pov::South ) //Going down
		Engine::getInstance().moveFB = -0.25f;

	if( key.state.mPOV[pov].direction & OIS::Pov::East ) //Going right
		Engine::getInstance().moveLR = -0.25f;
	else if( key.state.mPOV[pov].direction & OIS::Pov::West ) //Going left
		Engine::getInstance().moveLR = 0.25f;

	if( key.state.mPOV[pov].direction == OIS::Pov::Centered ) //stopped/centered out
	{
		Engine::getInstance().moveLR = 0;
		Engine::getInstance().moveFB = 0;
	}
}

void PingPongScene::OnJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis)
{
	Scene::OnJoystickMoveAxis(key,axis);
}

void PingPongScene::OnJoystickButtonUp(const OIS::JoyStickEvent& key, I8 button)
{
	if(button == 0)  servesteMingea();
}