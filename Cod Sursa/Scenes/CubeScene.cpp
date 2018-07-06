#include "CubeScene.h"
#include "Rendering/Framerate.h"
#include "Rendering/Camera.h"
#include "Rendering/common.h"

CubeScene::CubeScene(string name, string mainScript) : 
		  Scene(name,mainScript),
		  _GFX(GFXDevice::getInstance()),
		  _cameraEye(Camera::getInstance().getEye())
		  {
			  angleLR=0.0f,angleUD=0.0f,moveFB=0.0f;
		  }

void CubeScene::render()
{
	GFXDevice::getInstance().pushMatrix();
	GFXDevice::getInstance().rotate(1.2f,0.3f,0.6f,0);
	glutSolidCube(40);
	GFXDevice::getInstance().popMatrix();
}

void CubeScene::preRender()
{

}

void CubeScene::processInput()
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

bool CubeScene::load(const string& name)
{
	loadResources(true);
	loadEvents(true);
	return true;
}

bool CubeScene::unload()
{
	clearObjects();
	clearEvents();
	return true;
}
