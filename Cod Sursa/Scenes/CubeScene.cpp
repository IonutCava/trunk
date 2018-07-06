#include "CubeScene.h"
#include "Rendering/Framerate.h"
#include "Rendering/Camera.h"
#include "Rendering/common.h"
#include "PhysX/PhysX.h"
#include "Importer/DVDConverter.h"
#include "Terrain/Sky.h"

void CubeScene::render()
{
	RenderState s(true,true,true,true);
	GFXDevice::getInstance().setRenderState(s);

	if(PhysX::getInstance().getScene() != NULL)	PhysX::getInstance().UpdateActors();

	GFXDevice::getInstance().renderElements(ModelArray);
	GFXDevice::getInstance().renderElements(GeometryArray);
}

int j = 1;
bool _switch = false;
void CubeScene::preRender()
{
	if(i >= 360) _switch = true;
	else if(i <= 0) _switch = false;

	if(!_switch) i += 0.1f;
	else i -= 0.1f;

	i >= 180 ? j = -1 : j = 1;

	for(unordered_map<string,Object3D*>::iterator iter = GeometryArray.begin(); iter != GeometryArray.end(); iter++)
	{
		if((iter->second)->getName().compare("Cutia1") == 0)
			(iter->second)->getTransform()->rotateEuler(vec3(0.3f*i, 0.6f*i,0));
		if((iter->second)->getName().compare("HelloText") == 0)
			(iter->second)->getTransform()->rotateEuler(vec3(0.6f*i,0.2f,0.4f*i));
		if((iter->second)->getName().compare("Bila") == 0)
			(iter->second)->getTransform()->translateY(j*i);
	}
	ModelArray["dwarf"]->getTransform()->rotate(vec3(0,1,0),i);
} 

void CubeScene::processInput()
{
	moveFB  = Engine::getInstance().moveFB;
	moveLR  = Engine::getInstance().moveLR;
	angleLR = Engine::getInstance().angleLR;
	angleUD = Engine::getInstance().angleUD;
	
	if(angleLR)	Camera::getInstance().RotateX(angleLR * Framerate::getInstance().getSpeedfactor());
	if(angleUD)	Camera::getInstance().RotateY(angleUD * Framerate::getInstance().getSpeedfactor());
	if(moveFB)	Camera::getInstance().PlayerMoveForward(moveFB * (Framerate::getInstance().getSpeedfactor()/5));
	if(moveLR)	Camera::getInstance().PlayerMoveStrafe(moveLR * (Framerate::getInstance().getSpeedfactor()/5));

}

bool CubeScene::load(const string& name)
{
	bool state = false;
	_terMgr->createTerrains(TerrainInfoArray);
	addDefaultLight();
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
	_sunVector = vec4(-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );
    i = 0;

	return true;
}
