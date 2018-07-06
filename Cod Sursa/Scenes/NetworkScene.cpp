#include "NetworkScene.h"
#include "GUI/GUI.h"
#include "Rendering/Framerate.h"
#include "Rendering/common.h"
#include "Hardware/Network/ASIO.h"
#include "Rendering/Camera.h"

void NetworkScene::render()
{

	_box->draw();
	GUI &gui = GUI::getInstance();
	gui.draw();
	for(ModelIterator = ModelArray.begin();  ModelIterator != ModelArray.end();  ModelIterator++)
		(*ModelIterator)->Draw();
}

void NetworkScene::preRender()
{
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
	WorldPacket p(CMSG_GEOMERTY_LIST);
	p << string("NetworkScene");
	p << ModelDataArray.size();

	for(vector<FileData>::iterator _iter = ModelDataArray.begin(); _iter != ModelDataArray.end(); _iter++)
	{
		p << (*_iter).ModelName;
		p << (*_iter).version;

	}
	ASIO::getInstance().sendPacket(p);
	bool _load = false;
}

bool NetworkScene::load(const string& name)
{
	GFXDevice::getInstance().resizeWindow(640,384);
	ASIO& asio = ASIO::getInstance();

	checkPatches();
	_box = new Box3D(40);

	bool state = loadResources(true);
	ParamHandler::getInstance().setParam("serverResponse",string("waiting"));
	
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
	GUI::getInstance().modifyText("statusText",(char*)string("Connecting to server ...").c_str());
	ASIO::getInstance().connect();
}

void NetworkScene::disconnect()
{
	if(!ASIO::getInstance().isConnected())
		GUI::getInstance().modifyText("statusText",(char*)string("Disconnecting to server ...").c_str());
	ASIO::getInstance().disconnect();
}

bool NetworkScene::loadResources(bool continueOnErrors)
{
	GUI& gui = GUI::getInstance();
	ParamHandler& par = ParamHandler::getInstance();

	gui.addText("fpsDisplay",           //Unique ID
		                       vec3(60,60,0),          //Position
							   GLUT_BITMAP_8_BY_13,    //Font
							   vec3(0.0f,0.6f, 1.0f),  //Color
							   "FPS: %s",0);    //Text and arguments
	gui.addText("timeDisplay",
								vec3(60,70,0),
								GLUT_BITMAP_8_BY_13,
								vec3(0.6f,0.2f,0.2f),
								"Elapsed time: %5.0f",GETTIME());

	gui.addText("serverMessage",
								vec3(Engine::getInstance().getWindowWidth() / 4.0f,
								     Engine::getInstance().getWindowHeight() / 2.0f,
									 0),
								GLUT_BITMAP_TIMES_ROMAN_24,
								vec3(0.5f,0.5f,0.2f),
								"Server says: %s", "<< nothing yet >>");
	gui.addText("statusText",
								vec3(Engine::getInstance().getWindowWidth() / 3.0f,
								     Engine::getInstance().getWindowHeight() / 1.2f,
									 0),
								GLUT_BITMAP_HELVETICA_12,
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

	_eventTimers.push_back(0.0f); //Fps
	_eventTimers.push_back(0.0f); //Time
	_eventTimers.push_back(0.0f); //Server Ping
	return true;
}