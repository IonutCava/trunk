#include "Sky.h"

#include "Utility/Headers/ParamHandler.h"
#include "Managers/ResourceManager.h"
#include "Managers/SceneManager.h"
#include "Managers/CameraManager.h"
#include "Hardware/Video/GFXDevice.h"
#include "Hardware/Video/OpenGL/glResources.h"
#include "Hardware/Video/ShaderHandler.h"
#include "Geometry/Predefined/Sphere3D.h"
using namespace std;

Sky::Sky()
{
	_init = false;
	
   	string location = ParamHandler::getInstance().getParam<string>("assetsLocation")+"/misc_images/";
	_sky = ResourceManager::getInstance().LoadResource<Sphere3D>("Skybox",true);
	_sky->getResolution() = 4;
	_sun = ResourceManager::getInstance().LoadResource<Sphere3D>("Sun",true);
	_sun->getResolution() = 16;
	_sun->getSize() = 0.1f;
	_skybox =  ResourceManager::getInstance().LoadResource<TextureCubemap>(
				location+"skybox_2.jpg "+
				location+"skybox_1.jpg "+
				location+"skybox_5.jpg "+
				location+"skybox_6.jpg "+ 
				location+"skybox_3.jpg "+
				location+"skybox_4.jpg");

	_skyShader = ResourceManager::getInstance().LoadResource<Shader>("sky");
	assert(_skyShader);
	Con::getInstance().printfn("Generated sky cubemap and sun OK!");
	_init = true;
}

Sky::~Sky()
{
	ResourceManager::getInstance().remove(_sky);
	ResourceManager::getInstance().remove(_sun);
	ResourceManager::getInstance().remove(_skyShader);
	ResourceManager::getInstance().remove(_skybox);
}

void Sky::draw() const
{
	if(!_init) return;
	if (_drawSky && _drawSun) drawSkyAndSun();
	if (_drawSky && !_drawSun) drawSky();
	if (!_drawSky && _drawSun) drawSun();
	GFXDevice::getInstance().clearBuffers(GFXDevice::DEPTH_BUFFER);
}

void Sky::setParams(const vec3& eyePos, const vec3& sunVect, bool invert, bool drawSun, bool drawSky) 
{
	_eyePos = eyePos;	_sunVect = sunVect;
	_invert = invert;	_drawSun = drawSun;
	_drawSky = drawSky;
}

void Sky::drawSkyAndSun() const
{
	_sky->getTransform()->setPosition(vec3(_eyePos.x,_eyePos.y,_eyePos.z));
	_sky->getTransform()->scale(vec3(1.0f, _invert ? -1.0f : 1.0f, 1.0f));

	RenderState old = GFXDevice::getInstance().getActiveRenderState();
	RenderState s(false,false,false,true);
	_skybox->Bind(0);
	_skyShader->bind();
	{
		_skyShader->UniformTexture("texSky", 0);
		_skyShader->Uniform("enable_sun", true);
		_skyShader->Uniform("sun_vector", _sunVect);

		GFXDevice::getInstance().setRenderState(s);
		GFXDevice::getInstance().drawSphere3D(_sky);
		GFXDevice::getInstance().setRenderState(old);
	}
	_skyShader->unbind();
	_skybox->Unbind(0);
	
}

void Sky::drawSky() const
{
	_sky->getTransform()->setPosition(vec3(_eyePos.x,_eyePos.y,_eyePos.z));
	_sky->getTransform()->scale(vec3(1.0f, _invert ? -1.0f : 1.0f, 1.0f));
	RenderState old = GFXDevice::getInstance().getActiveRenderState();
	RenderState s(false,false,false,true);
	
	_skybox->Bind(0);
	_skyShader->bind();
	{
		_skyShader->UniformTexture("texSky", 0);
		_skyShader->Uniform("enable_sun", false);

		GFXDevice::getInstance().setRenderState(s);
		GFXDevice::getInstance().drawSphere3D(_sky);
		GFXDevice::getInstance().setRenderState(old);
	}
	_skyShader->unbind();
	_skybox->Unbind(0);

}

void Sky::drawSun() const
{
	_sun->getMaterial().diffuse = SceneManager::getInstance().getActiveScene()->getLights()[0]->getDiffuseColor();
	_sun->getTransform()->setPosition(vec3(_eyePos.x-_sunVect.x,_eyePos.y-_sunVect.y,_eyePos.z-_sunVect.z));
	RenderState old = GFXDevice::getInstance().getActiveRenderState();
	RenderState s(false,false,false,false);
	GFXDevice::getInstance().setRenderState(s);
	GFXDevice::getInstance().drawSphere3D(_sun);
	GFXDevice::getInstance().setRenderState(old);
}


