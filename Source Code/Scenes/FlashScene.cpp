#include "FlashScene.h"
#include "GUI/GUI.h"
#include "PhysX/PhysX.h"
#include "Managers/CameraManager.h"
using namespace std;

void FlashScene::render()
{
	
	RenderState s(true,true,true,true);
	GFXDevice::getInstance().setRenderState(s);

	_sceneGraph->render();

	GUI::getInstance().draw();
	
}


void FlashScene::preRender()
{
	if(PhysX::getInstance().getScene() != NULL)	
		PhysX::getInstance().UpdateActors();
} 

void FlashScene::processInput()
{
	Camera* cam = CameraManager::getInstance().getActiveCamera();

	moveFB  = Engine::getInstance().moveFB;
	moveLR  = Engine::getInstance().moveLR;
	angleLR = Engine::getInstance().angleLR;
	angleUD = Engine::getInstance().angleUD;
	
	if(angleLR)	cam->RotateX(angleLR * Framerate::getInstance().getSpeedfactor());
	if(angleUD)	cam->RotateY(angleUD * Framerate::getInstance().getSpeedfactor());
	if(moveFB)	cam->PlayerMoveForward(moveFB * (Framerate::getInstance().getSpeedfactor()/5));
	if(moveLR)	cam->PlayerMoveStrafe(moveLR * (Framerate::getInstance().getSpeedfactor()/5));

}

void FlashScene::processEvents(F32 time)
{
	F32 FpsDisplay = 0.3f;
	if (time - _eventTimers[0] >= FpsDisplay)
	{
		
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		_eventTimers[0] += FpsDisplay;
	}
}

bool FlashScene::load(const string& name)
{
	bool state = false;
	addDefaultLight();
	state = loadResources(true);	
	state = loadEvents(true);

	return state;
}

bool FlashScene::loadResources(bool continueOnErrors)
{
	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;
	_sunAngle = vec2(0.0f, RADIANS(45.0f));
	_sunVector = vec4(-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );
		GUI::getInstance().addText("fpsDisplay",           //Unique ID
		                       vec3(60,60,0),          //Position
							   BITMAP_8_BY_13,    //Font
							   vec3(0.0f,0.2f, 1.0f),  //Color
							   "FPS: %s",0);    //Text and arguments
		_eventTimers.push_back(0.0f);
    i = 0;
	return true;
}
