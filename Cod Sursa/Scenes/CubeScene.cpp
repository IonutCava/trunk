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
	/*GFXDevice::getInstance().rotate(1.2f,0.3f,0.6f,0);
	glutSolidCube(40);*/

	//X = 10; y = 20;
		//GFXDevice::getInstance().translate(-10.0f,10.0f,0.0f);
		GFXDevice::getInstance().setColor(0.5f,0.1f,0.3f);
		glBegin(GL_TRIANGLE_STRIP );
			glVertex3f( 0, 0,0 );
			glVertex3f( 10, 0,0 );
			glVertex3f( 0, 20,0);
			glVertex3f( 10, 20,0 );
		glEnd();
		GFXDevice::getInstance().setColor(0.5f,0.2f,0.9f);
	    glBegin( GL_TRIANGLE_STRIP );
			glVertex3f( 1, 1,0 );
			glVertex3f( 10-1, 1,0 );
			glVertex3f( 1, 20-1,0 );
			glVertex3f( 10-1, 20-1,0 );  
		glEnd();
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
