#include "ASIO.h"
#include "Headers/NetworkScene.h"

#include "GUI/Headers/GUI.h"
#include "Environment/Sky/Headers/Sky.h"
#include "Rendering/Camera/Headers/Camera.h"

void NetworkScene::render(){

	_sceneGraph->render();
}

void NetworkScene::preRender(){
	Light* light = LightManager::getInstance().getLight(0);
	vec4<F32> vSunColor = _white.lerp(vec4<F32>(1.0f, 0.5f, 0.0f, 1.0f), vec4<F32>(1.0f, 1.0f, 0.8f, 1.0f),
												0.25f + cosf(_sunAngle.y) * 0.75f);

	light->setLightProperties(LIGHT_POSITION,_sunVector);
	light->setLightProperties(LIGHT_AMBIENT,_white);
	light->setLightProperties(LIGHT_DIFFUSE,vSunColor);
	light->setLightProperties(LIGHT_SPECULAR,vSunColor);

	Sky::getInstance().setParams(_camera->getEye(),vec3<F32>(_sunVector),false,true,false);
	Sky::getInstance().draw();

}

void NetworkScene::processInput(){

	if(_angleLR) _camera->RotateX(_angleLR * Framerate::getInstance().getSpeedfactor());
	if(_angleUD) _camera->RotateY(_angleUD * Framerate::getInstance().getSpeedfactor());
	if(_moveFB)  _camera->MoveForward(_moveFB * (Framerate::getInstance().getSpeedfactor()/5));
	if(_moveLR)	 _camera->MoveStrafe(_moveLR * (Framerate::getInstance().getSpeedfactor()/5));
}

void NetworkScene::processEvents(F32 time){

	F32 FpsDisplay = 0.3f;
	F32 TimeDisplay = 0.01f;
	F32 ServerPing = 1.0f;
	if (time - _eventTimers[0] >= FpsDisplay){
		
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		_eventTimers[0] += FpsDisplay;
	}
    
	
	if (time - _eventTimers[1] >= TimeDisplay){ 
		GUI::getInstance().modifyText("timeDisplay", "Elapsed time: %5.0f", time);
		_eventTimers[1] += TimeDisplay;
	}

	if (time - _eventTimers[2] >= ServerPing){

		GUI::getInstance().modifyText("statusText", (char*)_paramHandler.getParam<std::string>("asioStatus").c_str());
		GUI::getInstance().modifyText("serverMessage",(char*)_paramHandler.getParam<std::string>("serverResponse").c_str());
		_eventTimers[2] += ServerPing;
	}

}

void NetworkScene::checkPatches(){

	if(ModelDataArray.empty()) return;
	WorldPacket p(CMSG_GEOMETRY_LIST);
	p << std::string("NetworkScene");
	p << ModelDataArray.size();

	for(std::vector<FileData>::iterator _iter = ModelDataArray.begin(); _iter != ModelDataArray.end(); ++_iter)	{
		p << (*_iter).ItemName;
		p << (*_iter).ModelName;
		p << (*_iter).version;
	}
	ASIO::getInstance().sendPacket(p);
}

bool NetworkScene::load(const std::string& name){

	setInitialData();
	_GFX.changeResolution(640,384);
	ASIO::getInstance().init(_paramHandler.getParam<std::string>("serverAddress"),std::string("443"));

	bool state = loadResources(true);
	_paramHandler.setParam("serverResponse",std::string("waiting"));
	addDefaultLight();
	_camera->setEye(vec3<F32>(0,30,-30));
	return state;
}

bool NetworkScene::unload(){
	Sky::getInstance().DestroyInstance();
	return Scene::unload();
}

void NetworkScene::test(){

	WorldPacket p(CMSG_PING);
	p << GETMSTIME();
	ASIO::getInstance().sendPacket(p);
}

void NetworkScene::connect(){

	GUI::getInstance().modifyText("statusText",(char*)std::string("Connecting to server ...").c_str());
	ASIO::getInstance().connect(_paramHandler.getParam<std::string>("serverAddress"),std::string("433"));
}

void NetworkScene::disconnect()
{
	if(!ASIO::getInstance().isConnected())
		GUI::getInstance().modifyText("statusText",(char*)std::string("Disconnecting to server ...").c_str());
	ASIO::getInstance().disconnect();
}

bool NetworkScene::loadResources(bool continueOnErrors)
{
	_sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
	_sunVector = vec4<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );

	GUI& gui = GUI::getInstance();

	gui.addText("fpsDisplay",           //Unique ID
		                       vec3<F32>(60,60,0),          //Position
							   BITMAP_8_BY_13,    //Font
							   vec3<F32>(0.0f,0.6f, 1.0f),  //Color
							   "FPS: %s",0);    //Text and arguments
	gui.addText("timeDisplay",
								vec3<F32>(60,70,0),
								BITMAP_8_BY_13,
								vec3<F32>(0.6f,0.2f,0.2f),
								"Elapsed time: %5.0f",GETTIME());

	gui.addText("serverMessage", vec3<F32>(_cachedResolution.width / 4.0f,
		                                   _cachedResolution.height / 1.6f,
										   0),
								BITMAP_8_BY_13,
								vec3<F32>(0.5f,0.5f,0.2f),
								"Server says: %s", "<< nothing yet >>");
	gui.addText("statusText",
								vec3<F32>(_cachedResolution.width / 3.0f,
								          _cachedResolution.height / 1.2f,
									      0),
								BITMAP_HELVETICA_12,
								vec3<F32>(0.2f,0.5f,0.2f),
								"");

	gui.addButton("getPing", "ping me", vec2<F32>(60 , _cachedResolution.height/1.1f),
										vec2<U16>(100,25),vec3<F32>(0.6f,0.6f,0.6f),
										boost::bind(&NetworkScene::test,this));
	gui.addButton("disconnect", "disconnect", vec2<F32>(180 , _cachedResolution.height/1.1f),
										vec2<U16>(100,25),vec3<F32>(0.5f,0.5f,0.5f),
										boost::bind(&NetworkScene::disconnect,this));
	gui.addButton("connect", "connect", vec2<F32>(300 , _cachedResolution.height/1.1f),
										vec2<U16>(100,25),vec3<F32>(0.65f,0.65f,0.65f),
										boost::bind(&NetworkScene::connect,this));
	gui.addButton("patch", "patch",     vec2<F32>(420 , _cachedResolution.height/1.1f),
										vec2<U16>(100,25),vec3<F32>(0.65f,0.65f,0.65f),
										boost::bind(&NetworkScene::checkPatches,this));
		

	_eventTimers.push_back(0.0f); //Fps
	_eventTimers.push_back(0.0f); //Time
	_eventTimers.push_back(0.0f); //Server Ping
	return true;
}