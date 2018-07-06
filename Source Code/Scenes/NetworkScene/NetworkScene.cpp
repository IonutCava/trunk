#include "ASIO.h"

#include "Headers/NetworkScene.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers//Application.h"
#include "Managers/Headers/CameraManager.h"
#include "Environment/Sky/Headers/Sky.h"

using namespace std;

void NetworkScene::render(){

	_sceneGraph->render();
}

void NetworkScene::preRender(){
	Light* light = LightManager::getInstance().getLight(0);
	vec4 vSunColor = _white.lerp(vec4(1.0f, 0.5f, 0.0f, 1.0f), vec4(1.0f, 1.0f, 0.8f, 1.0f),
								0.25f + cosf(_sunAngle.y) * 0.75f);

	light->setLightProperties(LIGHT_POSITION,_sunVector);
	light->setLightProperties(LIGHT_AMBIENT,_white);
	light->setLightProperties(LIGHT_DIFFUSE,vSunColor);
	light->setLightProperties(LIGHT_SPECULAR,vSunColor);

	Sky::getInstance().setParams(CameraManager::getInstance().getActiveCamera()->getEye(),vec3(_sunVector),false,true,false);
	Sky::getInstance().draw();

}

void NetworkScene::processInput(){
	Scene::processInput();
	Camera* cam = CameraManager::getInstance().getActiveCamera();
	moveFB  = Application::getInstance().moveFB;
	moveLR  = Application::getInstance().moveLR;
	angleLR = Application::getInstance().angleLR;
	angleUD = Application::getInstance().angleUD;
	
	if(angleLR)	cam->RotateX(angleLR * Framerate::getInstance().getSpeedfactor());
	if(angleUD)	cam->RotateY(angleUD * Framerate::getInstance().getSpeedfactor());
	if(moveFB)	cam->PlayerMoveForward(moveFB * (Framerate::getInstance().getSpeedfactor()/5));
	if(moveLR)	cam->PlayerMoveStrafe(moveLR * (Framerate::getInstance().getSpeedfactor()/5));
}

void NetworkScene::processEvents(F32 time)
{
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
		GUI::getInstance().modifyText("statusText", (char*)_paramHandler.getParam<string>("asioStatus").c_str());
		GUI::getInstance().modifyText("serverMessage",(char*)_paramHandler.getParam<string>("serverResponse").c_str());
		_eventTimers[2] += ServerPing;
	}

}

void NetworkScene::checkPatches()
{
	if(ModelDataArray.empty()) return;
	WorldPacket p(CMSG_GEOMETRY_LIST);
	p << string("NetworkScene");
	p << ModelDataArray.size();

	for(vector<FileData>::iterator _iter = ModelDataArray.begin(); _iter != ModelDataArray.end(); ++_iter)
	{
		p << (*_iter).ItemName;
		p << (*_iter).ModelName;
		p << (*_iter).version;
	}
	ASIO::getInstance().sendPacket(p);
}

bool NetworkScene::load(const string& name){
	setInitialData();
	_GFX.resizeWindow(640,384);
	ASIO::getInstance().init(_paramHandler.getParam<string>("serverAddress"),string("443"));

	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f;
	bool state = loadResources(true);
	_paramHandler.setParam("serverResponse",string("waiting"));
	addDefaultLight();
	CameraManager::getInstance().getActiveCamera()->setEye(vec3(0,30,-30));
	return state;
}

bool NetworkScene::unload(){
	Sky::getInstance().DestroyInstance();
	return Scene::unload();
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
	ASIO::getInstance().connect(_paramHandler.getParam<string>("serverAddress"),string("433"));
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
								vec3(Application::getInstance().getWindowDimensions().width / 4.0f,
								     Application::getInstance().getWindowDimensions().height / 1.6f,
									 0),
								BITMAP_8_BY_13,
								vec3(0.5f,0.5f,0.2f),
								"Server says: %s", "<< nothing yet >>");
	gui.addText("statusText",
								vec3(Application::getInstance().getWindowDimensions().width / 3.0f,
								     Application::getInstance().getWindowDimensions().height / 1.2f,
									 0),
								BITMAP_HELVETICA_12,
								vec3(0.2f,0.5f,0.2f),
								"");

	gui.addButton("getPing", "ping me", vec2(60 , Application::getInstance().getWindowDimensions().height/1.1f),
										vec2(100,25),vec3(0.6f,0.6f,0.6f),
										boost::bind(&NetworkScene::test,this));
	gui.addButton("disconnect", "disconnect", vec2(180 , Application::getInstance().getWindowDimensions().height/1.1f),
										vec2(100,25),vec3(0.5f,0.5f,0.5f),
										boost::bind(&NetworkScene::disconnect,this));
	gui.addButton("connect", "connect", vec2(300 , Application::getInstance().getWindowDimensions().height/1.1f),
										vec2(100,25),vec3(0.65f,0.65f,0.65f),
										boost::bind(&NetworkScene::connect,this));
	gui.addButton("patch", "patch",     vec2(420 , Application::getInstance().getWindowDimensions().height/1.1f),
										vec2(100,25),vec3(0.65f,0.65f,0.65f),
										boost::bind(&NetworkScene::checkPatches,this));
		

	_eventTimers.push_back(0.0f); //Fps
	_eventTimers.push_back(0.0f); //Time
	_eventTimers.push_back(0.0f); //Server Ping
	return true;
}