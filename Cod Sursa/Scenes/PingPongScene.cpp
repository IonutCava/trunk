#include "PingPongScene.h"
#include "Rendering/Framerate.h"
#include "Rendering/Camera.h"
#include "Rendering/common.h"
#include "PhysX/PhysX.h"
#include "Importer/DVDConverter.h"
#include "Terrain/Sky.h"
#include "GUI/GUI.h"

vec4 _lightPosition(0,20,6,1.0);





void PingPongScene::render()
{
	RenderState s(true,true,true,true);
	GFXDevice::getInstance().setRenderState(s);

	//Randam toata geometria
	if(PhysX::getInstance().getScene() != NULL)	PhysX::getInstance().UpdateActors();

	GFXDevice::getInstance().drawSphere3D(_minge);
	GFXDevice::getInstance().renderElements(ModelArray);
	GFXDevice::getInstance().renderElements(GeometryArray);

	GUI::getInstance().draw();

	_lights[0]->setLightProperties(string("position"),_lightPosition);
}


void PingPongScene::preRender()
{
	//Desenam un cer standard (copy/paste)
	vec2 _sunAngle = vec2(0.0f, RADIANS(45.0f));
	vec4 _sunVector = vec4(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );
	Sky& sky = Sky::getInstance();
	sky.setParams(Camera::getInstance().getEye(),_sunVector,false,true,true);
	sky.draw();
}

void PingPongScene::servesteMingea()
{
	GUI::getInstance().modifyText("insulte","");	
	while(!_events.empty())
	{
		_events[0]->stopEvent(); //Stop event se autodistruge/sterge
		_events.erase(_events.begin()); //Il scoatem din vector
	}

	if(_events.empty())//Maxim un singur eveniment
	{
		_minge->setPosition(vec3(0, 2 ,2));
		_events.push_back(new Event(30,true,false,boost::bind(&PingPongScene::test,this,rand() % 5,TYPE_INTEGER)));
	}
}


void PingPongScene::processEvents(F32 time)
{
	F32 refresh = 0.25f;
	if (time - _eventTimers[0] >= refresh)
	{
		/*
		WorldPacket p(CMSG_MISCARE_MINGE);
		p << _minge->getPosition().x;
		p << _minge->getPosition().y;
		p << _minge->getPosition().z;
		_asio.sendPacket(p);

		WorldPacket p(CMSG_MISCARE_PALETA);
		p << ModelArray["paleta"]->getPosition().x;
		p << ModelArray["paleta"]->getPosition().y;
		p << ModelArray["paleta"]->getPosition().z;
		_asio.sendPacket(p);
		*/
		_eventTimers[0] += refresh;
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
	bool updated = false;
	string mesaj;

	vec3 pozitieMinge = _minge->getPosition();
	vec3& pozitiePaleta = ModelArray["paleta"]->getPosition();
	vec3& pozitieMasa  = ModelArray["masa"]->getPosition();
	vec3 pozitieAdversar = ModelArray["perete"]->getPosition();
	//Miscare minge si detectie coliziuni

	if(pozitieMinge.y + 0.75f < ModelArray["masa"]->getBoundingBox().max.y)
	{
		//Daca am lovit noi si am atins terenul adversarului
		//Sau daca a lovit adversarul si nu a atins terenul nostrue
		if((_atinsTerenAdversar && _directieAdversar) || (!_directieAdversar && !_atinsTeren))		
			_pierdut = false;
		else
			_pierdut = true;
		
		updated = true;
		_events[0]->stopEvent();
	}
	//Am castigat sau am pierdut?
	if(pozitieMinge.z >= pozitiePaleta.z)
	{
		_pierdut = true;
		updated = true;
		_events[0]->stopEvent();
	}
	if(pozitieMinge.z <= pozitieAdversar.z)
	{
		_pierdut = false;
		updated = true;
		_events[0]->stopEvent();
	}

	//Mergem spre adversar sau spre noi
	_directieAdversar ? pozitieMinge.z -= 0.11f : pozitieMinge.z += 0.11f;
	//Sus sau jos?
	_directieSus ? 	pozitieMinge.y += 0.075f : 	pozitieMinge.y -= 0.075f;


	//Mingea se deplaseaza spre stanga sau spre dreapta?
	pozitieMinge.x += _miscareLaterala*0.15f;
	if(pozitieAdversar.x != pozitieMinge.x)
		ModelArray["perete"]->translateX(pozitieMinge.x - pozitieAdversar.x);

	_minge->setPosition(pozitieMinge);


	//Am lovit masa? Ricosam
	if(ModelArray["masa"]->getBoundingBox().Collision(_minge->getBoundingBox()))
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
	if(_minge->getBoundingBox().Collision(ModelArray["paleta"]->getBoundingBox()))
	{
		_miscareLaterala = pozitieMinge.x - pozitiePaleta.x;
		//Dca am lovit cu partea de sus a paletei, mai atasam un mic impuls miscarii mingii
		if(pozitieMinge.y >= pozitiePaleta.y) pozitieMinge.z -= 0.11f;

		_directieAdversar = true;
	}

	//Am lovit plasa?
	if(_minge->getBoundingBox().Collision(ModelArray["plasa"]->getBoundingBox()) &&	_directieAdversar)
	{
		_pierdut = true;
		updated = true;
		_events[0]->stopEvent();
    }
	//A lovit adversarul plasa
	if(_minge->getBoundingBox().Collision(ModelArray["plasa"]->getBoundingBox()) &&	!_directieAdversar)
	{
		_pierdut = false;
		updated = true;
		_events[0]->stopEvent();
    }
	//Am lovit adversarul? Ne intoarcem ... DAR
	//... adaugam o mica sansa sa castigam meciul .. dar mica ...
	if(random(30) != 2)
	if(_minge->getBoundingBox().Collision(ModelArray["perete"]->getBoundingBox()))
	{
		_miscareLaterala = pozitieMinge.x - ModelArray["perete"]->getPosition().x;
		_directieAdversar = false;
	}
	//Rotim mingea doar de efect ...
	_minge->getOrientation() = vec3(pozitieMinge.z,1,1);
	if(updated)
	{
		if(_pierdut)
		{
			mesaj = "Ai pierdut!";
			_scor--;

			if(b == TYPE_INTEGER)
			{
				I32 quote = boost::any_cast<int>(a);
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
	//Move FB = Forward/Back = sus/jos
	//Move LR = Left/Right = stanga/dreapta
	moveFB  = Engine::getInstance().moveFB;
	moveLR  = Engine::getInstance().moveLR;

	//Miscarea camerei
	angleLR = Engine::getInstance().angleLR;
	angleUD = Engine::getInstance().angleUD;

	if(angleLR)	Camera::getInstance().RotateX(angleLR * Framerate::getInstance().getSpeedfactor());
	if(angleUD)	Camera::getInstance().RotateY(angleUD * Framerate::getInstance().getSpeedfactor());

	vec3 pos = ModelArray["paleta"]->getPosition();
	//Miscarea pe ambele directii de deplasare este limitata in intervalul [-3,3] cu exceptia coborarii pe Y;
	if(moveFB)
	{
		if((moveFB > 0 && pos.y >= 3) || (moveFB < 0 && pos.y <= 0.5f)) return;
		ModelArray["paleta"]->translateY((moveFB * Framerate::getInstance().getSpeedfactor())/6);
		
	}
	if(moveLR)
	{
		//Miscarea stanga dreapta necesita inversarea directiei de deplasare.
		if((moveLR < 0 && pos.x >= 3) || (moveLR > 0 && pos.x <= -3)) return;
		ModelArray["paleta"]->translateX((-moveLR * Framerate::getInstance().getSpeedfactor())/6);
	}

	

}

bool PingPongScene::load(const string& name)
{
	bool state = false;
	//Adaugam o lumina
	addDefaultLight();

	//Incarcam resursele scenei
	state = loadResources(true);	
	state = loadEvents(true);

	//Cream o minge
	_minge = new Minge(0.1f,16);
	_minge->getPosition() = vec3(0, 2 ,2);
	_minge->getColor() = vec3(1,0.2f,0.2f);
	_minge->computeBoundingBox();
	_minge->getName() = "Minge";

	//Pozitionam camera
	Camera::getInstance().setAngleX(RADIANS(-90));
	Camera::getInstance().setEye(vec3(0,2.5f,6.5f));
	
	return state;
}

bool PingPongScene::unload()
{
	clearObjects();
	clearEvents();
	return true;
}

bool PingPongScene::loadResources(bool continueOnErrors)
{
	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;_scor = 0;

	//Adaugam butoane si text labels
	GUI::getInstance().addButton("Serveste", "Serveste", vec2(Engine::getInstance().getWindowWidth()-120 ,
															 Engine::getInstance().getWindowHeight()/1.1f),
													    	 vec2(100,25),vec3(0.65f,0.65f,0.65f),
															 boost::bind(&PingPongScene::servesteMingea,this));

	GUI::getInstance().addText("Scor",vec3(Engine::getInstance().getWindowWidth() - 120, Engine::getInstance().getWindowHeight()/1.3f, 0),
							   BITMAP_8_BY_13,vec3(1,0,0), "Scor: %d",0);

	GUI::getInstance().addText("Mesaj",vec3(Engine::getInstance().getWindowWidth() - 120, Engine::getInstance().getWindowHeight()/1.5f, 0),
							   BITMAP_8_BY_13,vec3(1,0,0), "");
	GUI::getInstance().addText("insulte",vec3(Engine::getInstance().getWindowWidth()/4, Engine::getInstance().getWindowHeight()/3, 0),
							   BITMAP_TIMES_ROMAN_24,vec3(0,1,0), "");

	//Cateva replici motivationale
	_quotes.push_back("Ha ha ... pana si Odin rade de tine!");
	_quotes.push_back("Daca tu esti jucator de ping-pong, eu sunt Jimmy Page");
	_quotes.push_back("Ooolee, ole ole ole, uite mingea ... nu mai e");
	_quotes.push_back("Ai noroc ca sala-i goala, ca eu unul as fi murit de rusine");
	_quotes.push_back("Nu e atat de greu; si o maimuta poate juca jocul asta.");

	
	_eventTimers.push_back(0.0f);
	return true;
}
