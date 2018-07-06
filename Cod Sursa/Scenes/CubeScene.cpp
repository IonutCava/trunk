#include "CubeScene.h"
#include "Rendering/Framerate.h"
#include "Rendering/Camera.h"
#include "Rendering/common.h"
#include "PhysX/PhysX.h"

void CubeScene::render()
{
	GFXDevice::getInstance().pushMatrix();
		
		GFXDevice::getInstance().rotate(i,0.3f,0.6f,0);
		glutSolidCube(40);

		GFXDevice::getInstance().translate(-10.0f,-100.0f,0.0f);
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
	
	_terMgr->drawTerrains(true,false);

	if(PhysX::getInstance().getScene() != NULL)
		PhysX::getInstance().RenderActors();
	else
		for(ModelIterator = ModelArray.begin();  ModelIterator != ModelArray.end();  ModelIterator++)
		{
			GFXDevice::getInstance().pushMatrix();
			GFXDevice::getInstance().translate((*ModelIterator)->getPosition());
			GFXDevice::getInstance().rotate((*ModelIterator)->getOrientation());
			GFXDevice::getInstance().scale((*ModelIterator)->getScale());
			(*ModelIterator)->getShader()->bind();
				(*ModelIterator)->Draw();
			(*ModelIterator)->getShader()->unbind();
			GFXDevice::getInstance().popMatrix();
		}
}

void CubeScene::preRender()
{
	i >= 360 ? i = 0 : i += 0.1f;

	vec4 zeros(0.0f, 0.0f, 0.0f,0.0f);
	vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 black(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 orange(1.0f, 0.5f, 0.0f, 1.0f);
	vec4 yellow(1.0f, 1.0f, 0.8f, 1.0f);
	F32 sun_cosy = cosf(_sunAngle.y);
	vec4 vSunColor = white.lerp(orange, yellow, 0.25f + sun_cosy * 0.75f);

	_lights[0]->setLightProperties(string("spotDirection"),zeros);
	_lights[0]->setLightProperties(string("position"),_sunVector);
	_lights[0]->setLightProperties(string("ambient"),white);
	_lights[0]->setLightProperties(string("diffuse"),vSunColor);
	_lights[0]->setLightProperties(string("specular"),vSunColor);
	_lights[0]->update();

}

void CubeScene::processInput()
{
	angleLR = Engine::getInstance().angleLR;
	angleUD = Engine::getInstance().angleUD;
	moveFB = Engine::getInstance().moveFB;
	
	if(angleLR)	Camera::getInstance().RotateX(angleLR * Framerate::getInstance().getSpeedfactor());
	if(angleUD)	Camera::getInstance().RotateY(angleUD * Framerate::getInstance().getSpeedfactor());
	if(moveFB)	Camera::getInstance().PlayerMoveForward(moveFB * Framerate::getInstance().getSpeedfactor());
	if(moveLR)	Camera::getInstance().PlayerMoveStrafe(moveLR * Framerate::getInstance().getSpeedfactor());

}

bool CubeScene::load(const string& name)
{
	bool state = false;
	_terMgr->createTerrains(TerrainInfoArray);
	state = loadResources(true);	
	state = loadEvents(true);
	return state;
}

bool CubeScene::unload()
{
	clearObjects();
	clearEvents();
	return true;
}

bool CubeScene::loadResources(bool continueOnErrors)
{
	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;
	_sunAngle = vec2(0.0f, RADIANS(45.0f));
	_sunVector = vec4(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );
    i = 0;
	return true;
}
