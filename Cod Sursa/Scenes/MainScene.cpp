#include "MainScene.h"


#include "Utility/Headers/ParamHandler.h"
#include "Managers/ResourceManager.h"
#include "Managers/TerrainManager.h"
#include "Rendering/Framerate.h"
#include "Rendering/Camera.h"
#include "Rendering/common.h"
#include "PhysX/PhysX.h"
#include "Terrain/Sky.h"
#include "GUI/GUI.h"
#include "Rendering/Frustum.h"

void MainScene::preRender()
{
	ResourceManager& res = ResourceManager::getInstance();
	Sky &sky = Sky::getInstance();
	Camera& cam = Camera::getInstance();
	
	ParamHandler &par = ParamHandler::getInstance();

	_skyFBO->Begin();

	_GFX.clearBuffers(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	vec3 zeros(0.0f, 0.0f, 0.0f);
	vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 black(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 orange(1.0f, 0.5f, 0.0f, 1.0f);
	vec4 yellow(1.0f, 1.0f, 0.8f, 1.0f);
	F32 sun_cosy = cosf(_sunAngle.y);


	glPushAttrib(GL_ENABLE_BIT);
	vec4 vSunColor = white.lerp(orange, yellow, 0.25f + sun_cosy * 0.75f);
	sky.setParams(Camera::getInstance().getEye(),vec3(_sunVector),true,true,true);
	
	

	_lights[0]->setLightProperties(string("spotDirection"),zeros);
	_lights[0]->setLightProperties(string("position"),_sunVector);
	_lights[0]->setLightProperties(string("ambient"),white);
	_lights[0]->setLightProperties(string("diffuse"),vSunColor);
	_lights[0]->setLightProperties(string("specular"),vSunColor);
	_lights[0]->update();
	
	
	vec4 vGroundAmbient = white.lerp(white*0.2f, black, sun_cosy);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, vGroundAmbient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0f);
	
	sky.draw();
	_terMgr->drawTerrains(true,true);
	glPopAttrib();

	_skyFBO->End();
}

void MainScene::render()
{
	Sky &sky = Sky::getInstance();
	GUI &gui = GUI::getInstance();
	Camera& cam = Camera::getInstance();

	glPushAttrib(GL_ENABLE_BIT);
	
	vec4 zeros(0.0f, 0.0f, 0.0f,0.0f);
	vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 black(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 orange(1.0f, 0.5f, 0.0f, 1.0f);
	vec4 yellow(1.0f, 1.0f, 0.8f, 1.0f);
	F32 sun_cosy = cosf(_sunAngle.y);

	vec4 vSunColor = white.lerp(orange, yellow, 0.25f + sun_cosy * 0.75f);
	sky.setParams(Camera::getInstance().getEye(),vec3(_sunVector),false,true,true);

	_lights[0]->setLightProperties(string("spotDirection"),zeros);
	_lights[0]->setLightProperties(string("position"),_sunVector);
	_lights[0]->setLightProperties(string("ambient"),white);
	_lights[0]->setLightProperties(string("diffuse"),vSunColor);
	_lights[0]->setLightProperties(string("specular"),vSunColor);
	_lights[0]->update();
	
	vec4 vGroundAmbient = white.lerp(white*0.2f, black, sun_cosy);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, vGroundAmbient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0f);

	sky.draw();
	_terMgr->drawTerrains(true,false);
	_terMgr->drawInfinitePlane(Camera::getInstance().getEye(), 15.0f*1500,*_skyFBO);

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
		{
			GFXDevice::getInstance().pushMatrix();
			if(_drawBB)(*ModelIterator)->DrawBBox();
			if(!(*ModelIterator)->IsInView()) continue;
			GFXDevice::getInstance().translate((*ModelIterator)->getPosition());
			GFXDevice::getInstance().rotate((*ModelIterator)->getOrientation());
			GFXDevice::getInstance().scale((*ModelIterator)->getScale());
			(*ModelIterator)->getShader()->bind();
				if(_drawObjects) (*ModelIterator)->Draw();
			(*ModelIterator)->getShader()->unbind();
			GFXDevice::getInstance().popMatrix();
		}
}

void MainScene::processInput()
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

F32 SunDisplay = 0.5f;
F32 FpsDisplay = 0.3f;
F32 TimeDisplay = 0.01f;
void MainScene::processEvents(F32 time)
{
	
	if (time - _eventTimers[0] >= SunDisplay)
	{
		_sunAngle.y += 0.0005f;
		_sunVector = vec4(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );
		_eventTimers[0] += SunDisplay;
	}

	if (time - _eventTimers[1] >= FpsDisplay)
	{
		
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		_eventTimers[1] += FpsDisplay;
	}
    
	
	if (time - _eventTimers[2] >= TimeDisplay)
	{
		GUI::getInstance().modifyText("timeDisplay", "Elapsed time: %5.0f", time);
		_eventTimers[2] += TimeDisplay;
	}
}

bool MainScene::load(const string& name)
{
	bool state = false;
	_terMgr->createTerrains(TerrainInfoArray);
	state = loadResources(true);	
	state = loadEvents(true);
	return state;
}

bool MainScene::unload()
{
	return true;
}

bool _switchFB = false;
void MainScene::test(boost::any a, CallbackParam b)
{
	boost::mutex::scoped_lock l(_mutex);
	vec3 pos = ModelArray[0]->getPosition();

	if(!_switchFB)
	{
		if(pos.x < 300 && pos.z == 0)		   pos.x++;
		if(pos.x == 300)
		{
			if(pos.y < 800 && pos.z == 0)      pos.y++;
			if(pos.y == 800)
			{
				if(pos.z > -500)   pos.z--;
				if(pos.z == -500)  _switchFB = true;
			}
			
		}
	}
	else 
	{
		if(pos.x > -300 && pos.z ==  -500)      pos.x--;
		if(pos.x == -300)
		{
			if(pos.y > 150 && pos.z == -500)    pos.y--;
			if(pos.y == 150)
			{
				if(pos.z < 0)    pos.z++;
				if(pos.z == 0)   _switchFB = false;
			}
		}
	}

	ModelArray[0]->setPosition(pos);
}

bool MainScene::loadResources(bool continueOnErrors)
{
	angleLR=0.0f,angleUD=0.0f,moveFB=0.0f;
	_eventTimers.push_back(0.0f); //Sun
	_eventTimers.push_back(0.0f); //Fps
	_eventTimers.push_back(0.0f); //Time
	_skyFBO = GFXDevice::getInstance().newFBO();
	_skyFBO->Create(FrameBufferObject::FBO_2D_COLOR, 1024, 1024);
	_geometryShader  = ResourceManager::getInstance().LoadResource<Shader>("terrain_tree.frag");
	_sunAngle = vec2(0.0f, RADIANS(45.0f));
	_sunVector = vec4(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );


	_events.push_back(new Event(30,true,false,boost::bind(&MainScene::test,this,string("test"),TYPE_STRING)));


	return true;
}