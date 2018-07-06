#include "MainScene.h"


#include "Utility/Headers/ParamHandler.h"
#include "Managers/ResourceManager.h"
#include "Managers/TerrainManager.h"
#include "Rendering/Framerate.h"
#include "Rendering/Camera.h"
#include "PhysX/PhysX.h"
#include "Terrain/Sky.h"
#include "GUI/GUI.h"

MainScene::MainScene(string name, string mainScript) : 
		  Scene(name,mainScript),
		  _cameraEye(Camera::getInstance().getEye())
{
	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f;
	update_time = 0.0f;
}


void MainScene::preRender()
{
	ResourceManager& res = ResourceManager::getInstance();
	Sky &sky = Sky::getInstance();
	Camera& cam = Camera::getInstance();
	
	ParamHandler &par = ParamHandler::getInstance();
	_skyFBO->Begin();

	_GFX.clearBuffers(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 black(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 orange(1.0f, 0.5f, 0.0f, 1.0f);
	vec4 yellow(1.0f, 1.0f, 0.8f, 1.0f);
	F32 sun_cosy = cosf(_sunAngle.y);
	glPushAttrib(GL_ENABLE_BIT);
	vec4 vSunColor = white.lerp(orange, yellow, 0.25f + sun_cosy * 0.75f);
	sky.setParams(_cameraEye,vec3(_sunVector),true,true,true);
	sky.draw();
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	vec3 zeros(0.0f, 0.0f, 0.0f);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, zeros);
	glLightfv(GL_LIGHT0, GL_POSITION, _sunVector);
	glLightfv(GL_LIGHT0, GL_AMBIENT, white);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, vSunColor);
	glLightfv(GL_LIGHT0, GL_SPECULAR, vSunColor);
	
	vec4 vGroundAmbient = white.lerp(white*0.2f, black, sun_cosy);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, vGroundAmbient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0f);
	

	TerrainManager::getInstance().drawTerrains(true,true);
	glPopAttrib();
	_skyFBO->End();
}

void MainScene::render()
{
	Sky &sky = Sky::getInstance();
	GUI &gui = GUI::getInstance();
	Camera& cam = Camera::getInstance();

	vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 black(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 orange(1.0f, 0.5f, 0.0f, 1.0f);
	vec4 yellow(1.0f, 1.0f, 0.8f, 1.0f);
	F32 sun_cosy = cosf(_sunAngle.y);

	glPushAttrib(GL_ENABLE_BIT);
	

	vec4 vSunColor = white.lerp(orange, yellow, 0.25f + sun_cosy * 0.75f);

	sky.setParams(_cameraEye,vec3(_sunVector),false,true,true);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	vec3 zeros(0.0f, 0.0f, 0.0f);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, zeros);
	glLightfv(GL_LIGHT0, GL_POSITION, _sunVector);
	glLightfv(GL_LIGHT0, GL_AMBIENT, white);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, vSunColor);
	glLightfv(GL_LIGHT0, GL_SPECULAR, vSunColor);
	
	vec4 vGroundAmbient = white.lerp(white*0.2f, black, sun_cosy);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, vGroundAmbient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0f);

	
	sky.draw();
	TerrainManager::getInstance().drawTerrains(true,false);
	TerrainManager::getInstance().drawInfinitePlane(Camera::getInstance().getEye(), 15.0f*1500,*_skyFBO);
	glPopAttrib();
	renderActors();
	gui.draw();
}

void MainScene::renderActors()
{
	if(PhysX::getInstance().getScene() != NULL)
		PhysX::getInstance().RenderActors();
	else
		for(ModelIterator = ModelArray.begin();  ModelIterator != ModelArray.end();  ModelIterator++)
			glCallList((*ModelIterator)->ListID);
}

void MainScene::processInput()
{
	angleLR = Engine::getInstance().angleLR;
	angleUD = Engine::getInstance().angleUD;
	moveFB = Engine::getInstance().moveFB;
	
	if(angleLR)	Camera::getInstance().RotateX(angleLR * Framerate::getInstance().getSpeedfactor());
	if(angleUD)	Camera::getInstance().RotateY(angleUD * Framerate::getInstance().getSpeedfactor());
	if(moveFB)
	{
		Camera::getInstance().PlayerMoveForward(moveFB * Framerate::getInstance().getSpeedfactor());
		_cameraEye = Camera::getInstance().getPrevEye();
	}
	else
	{
		_cameraEye = Camera::getInstance().getEye();
	}

}

F32 SunDisplay = 0.005f;
void MainScene::processEvents(F32 time)
{
	
	if (time - update_time >= SunDisplay)
	{
		_sunAngle.y += 0.0005f;
		_sunVector = vec4(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );
		update_time += SunDisplay;
	}
}
bool MainScene::load(const string& name)
{
	_skyFBO = GFXDevice::getInstance().newFBO();
	_skyFBO->Create(FrameBufferObject::FBO_2D_COLOR, 1024, 1024);
	_sunAngle = vec2(0.0f, RADIANS(45.0f));
	_sunVector = vec4(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );
	return true;
}

bool MainScene::unload()
{
	return true;
}
