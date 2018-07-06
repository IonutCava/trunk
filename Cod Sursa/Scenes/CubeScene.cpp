#include "CubeScene.h"
#include "Rendering/Framerate.h"
#include "Rendering/Camera.h"
#include "Rendering/common.h"
#include "PhysX/PhysX.h"
#include "Importer/DVDConverter.h"
#include "Terrain/Sky.h"

Shader *s;
void CubeScene::render()
{
	GFXDevice::getInstance().pushMatrix();
	GFXDevice::getInstance().rotate(i,vec3(0.3f,0.6f,0));
	_box->draw();
	GFXDevice::getInstance().popMatrix();

	_innerQuad->draw();
	_outterQuad->draw();	

	if(PhysX::getInstance().getScene() != NULL)
		PhysX::getInstance().RenderActors();
	else
		for(ModelIterator = ModelArray.begin();  ModelIterator != ModelArray.end();  ModelIterator++)
			(*ModelIterator)->Draw();

	_terMgr->drawTerrains(true,false);
}

void CubeScene::preRender()
{
	i >= 360 ? i = 0 : i += 0.1f;

	
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

void CubeScene::processInput()
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

bool CubeScene::load(const string& name)
{
	bool state = false;
	_terMgr->createTerrains(TerrainInfoArray);
	_lights.push_back(new Light(0));
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
	_box = new Box3D(40);
	_innerQuad = new Quad3D(vec3(1,1,0),vec3(10-1,0,0),vec3(0,20-1,0),vec3(10-1,20-1,0));
	_outterQuad = new Quad3D(vec3(0,0,0),vec3(10,0,0),vec3(0,20,0),vec3(10,20,0));

	
	_box->getColor() = vec3(0.5f,0.1f,0.3f);
	_innerQuad->getColor() = vec3(0.5f,0.2f,0.9f);
	_outterQuad->getColor() = vec3(0.2f,0.1f,0.3f);

	_innerQuad->getPosition() = vec3(-10.0f,-100.0f,-0.1f);
	_outterQuad->getPosition() = vec3(-10.0f,-100.0f,0.0f);
	return true;
}
