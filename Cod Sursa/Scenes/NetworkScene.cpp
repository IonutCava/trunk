#include "ASIO.h"

#include "NetworkScene.h"
#include "GUI/GUI.h"
#include "Rendering/Framerate.h"
#include "Rendering/common.h"
#include "Rendering/Camera.h"
#include "Terrain/Sky.h"

void NetworkScene::render()
{

	GUI &gui = GUI::getInstance();
	

	for(ModelIterator = ModelArray.begin();  ModelIterator != ModelArray.end();  ModelIterator++)
		GFXDevice::getInstance().renderModel(*ModelIterator);

	gui.draw();
	
}

void NetworkScene::preRender()
{
	vec3 zeros(0.0f, 0.0f, 0.0f);
	vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 black(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 orange(1.0f, 0.5f, 0.0f, 1.0f);
	vec4 yellow(1.0f, 1.0f, 0.8f, 1.0f);
	F32 sun_cosy = cosf(_sunAngle.y);

	vec4 vSunColor = white.lerp(orange, yellow, 0.25f + sun_cosy * 0.75f);

	Sky::getInstance().setParams(Camera::getInstance().getEye(),vec3(_sunVector),false,true,false);
	Sky::getInstance().draw();

	_lights[0]->setLightProperties(string("spotDirection"),zeros);
	_lights[0]->setLightProperties(string("position"),_sunVector);
	_lights[0]->setLightProperties(string("ambient"),white);
	_lights[0]->setLightProperties(string("diffuse"),vSunColor);
	_lights[0]->setLightProperties(string("specular"),vSunColor);
	_lights[0]->update();

}

void NetworkScene::processInput()
{
	moveFB  = Engine::getInstance().moveFB;
	moveLR  = Engine::getInstance().moveLR;
	angleLR = Engine::getInstance().angleLR;
	angleUD = Engine::getInstance().angleUD;
	
	if(angleLR)	Camera::getInstance().RotateX(angleLR * Framerate::getInstance().getSpeedfactor());
	if(angleUD)	Camera::getInstance().RotateY(angleUD * Framerate::getInstance().getSpeedfactor());
	if(moveFB)	Camera::getInstance().PlayerMoveForward(moveFB * Framerate::getInstance().getSpeedfactor());
	if(moveLR)	Camera::getInstance().PlayerMoveStrafe(moveLR * Framerate::getInstance().getSpeedfactor());
}

void NetworkScene::processEvents(F32 time)
{
	ParamHandler& par = ParamHandler::getInstance();
	F32 FpsDisplay = 0.3f;
	F32 TimeDisplay = 0.01f;
	F32 ServerPing = 1.0f;
	if (time - _eventTimers[0] >= FpsDisplay)
	{
		
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		_eventTimers[0] += FpsDisplay;
	}
    
	
	if (time - _eventTimers[1] >= TimeDisplay)
	{
		GUI::getInstance().modifyText("timeDisplay", "Elapsed time: %5.0f", time);
		_eventTimers[1] += TimeDisplay;
	}

	if (time - _eventTimers[2] >= ServerPing)
	{
		GUI::getInstance().modifyText("statusText", (char*)par.getParam<string>("asioStatus").c_str());
		GUI::getInstance().modifyText("serverMessage",(char*)par.getParam<string>("serverResponse").c_str());
		_eventTimers[2] += ServerPing;
	}

}

bool NetworkScene::unload()
{
	return true;
}

void NetworkScene::checkPatches()
{
	if(ModelDataArray.empty()) return;
	WorldPacket p(CMSG_GEOMETRY_LIST);
	p << string("NetworkScene");
	p << ModelDataArray.size();

	for(vector<FileData>::iterator _iter = ModelDataArray.begin(); _iter != ModelDataArray.end(); _iter++)
	{
		p << (*_iter).ItemName;
		p << (*_iter).ModelName;
		p << (*_iter).version;
	}
	ASIO::getInstance().sendPacket(p);
	bool _load = false;
}

bool NetworkScene::load(const string& name)
{
	ParamHandler& par = ParamHandler::getInstance();
	GFXDevice::getInstance().resizeWindow(640,384);
	ASIO::getInstance().init(par.getParam<string>("serverAddress"),string("443"));

	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f;
	bool state = loadResources(true);
	ParamHandler::getInstance().setParam("serverResponse",string("waiting"));
	_lights.push_back(new Light(0));
	return state;
}

void NetworkScene::test()
{
	WorldPacket p(CMSG_PING);
	p << GETMSTIME();
	ASIO::getInstance().sendPacket(p);
}

void NetworkScene::connect()
{
	ParamHandler& par = ParamHandler::getInstance();
	GUI::getInstance().modifyText("statusText",(char*)string("Connecting to server ...").c_str());
	ASIO::getInstance().connect(par.getParam<string>("serverAddress"),string("433"));
}

void NetworkScene::disconnect()
{
	if(!ASIO::getInstance().isConnected())
		GUI::getInstance().modifyText("statusText",(char*)string("Disconnecting to server ...").c_str());
	ASIO::getInstance().disconnect();
}

bool NetworkScene::loadResources(bool continueOnErrors)
{
	_sunAngle = vec2(0.0f, RADIANS(45.0f));
	_sunVector = vec4(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );

	GUI& gui = GUI::getInstance();
	ParamHandler& par = ParamHandler::getInstance();

	gui.addText("fpsDisplay",           //Unique ID
		                       vec3(60,60,0),          //Position
							   BITMAP_8_BY_13,    //Font
							   vec3(0.0f,0.6f, 1.0f),  //Color
							   "FPS: %s",0);    //Text and arguments
	gui.addText("timeDisplay",
								vec3(60,70,0),
								BITMAP_8_BY_13,
								vec3(0.6f,0.2f,0.2f),
								"Elapsed time: %5.0f",GETTIME());

	gui.addText("serverMessage",
								vec3(Engine::getInstance().getWindowWidth() / 4.0f,
								     Engine::getInstance().getWindowHeight() / 1.6f,
									 0),
								BITMAP_8_BY_13,
								vec3(0.5f,0.5f,0.2f),
								"Server says: %s", "<< nothing yet >>");
	gui.addText("statusText",
								vec3(Engine::getInstance().getWindowWidth() / 3.0f,
								     Engine::getInstance().getWindowHeight() / 1.2f,
									 0),
								BITMAP_HELVETICA_12,
								vec3(0.2f,0.5f,0.2f),
								"");

	gui.addButton("getPing", "ping me", vec2(60 , Engine::getInstance().getWindowHeight()/1.1f),
										vec2(100,25),vec3(0.6f,0.6f,0.6f),
										boost::bind(&NetworkScene::test,this));
	gui.addButton("disconnect", "disconnect", vec2(180 , Engine::getInstance().getWindowHeight()/1.1f),
										vec2(100,25),vec3(0.5f,0.5f,0.5f),
										boost::bind(&NetworkScene::disconnect,this));
	gui.addButton("connect", "connect", vec2(300 , Engine::getInstance().getWindowHeight()/1.1f),
										vec2(100,25),vec3(0.65f,0.65f,0.65f),
										boost::bind(&NetworkScene::connect,this));
	gui.addButton("patch", "patch",     vec2(420 , Engine::getInstance().getWindowHeight()/1.1f),
										vec2(100,25),vec3(0.65f,0.65f,0.65f),
										boost::bind(&NetworkScene::checkPatches,this));
		

	_eventTimers.push_back(0.0f); //Fps
	_eventTimers.push_back(0.0f); //Time
	_eventTimers.push_back(0.0f); //Server Ping
	return true;
}