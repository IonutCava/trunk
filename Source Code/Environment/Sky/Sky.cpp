#include "Headers/Sky.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/LightManager.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
using namespace std;

Sky::Sky() : _skyShader(NULL), _skybox(NULL), _init(false){}

Sky::~Sky(){
	RemoveResource(_skyShader);
	RemoveResource(_skybox);
}

bool Sky::load() {
	if(_init) return false;
   	string location = ParamHandler::getInstance().getParam<string>("assetsLocation")+"/misc_images/";
	ResourceDescriptor skybox("SkyBox");
	skybox.setFlag(true); //no default material;
	_sky = ResourceManager::getInstance().loadResource<Sphere3D>(skybox);
	_sky->setResolution(4);
	_skyNode = SceneManager::getInstance().getActiveScene()->getSceneGraph()->getRoot()->addNode(_sky);
	_sky->setRenderState(false);
	ResourceDescriptor sun("Sun");
	sun.setFlag(true);
	_sun = ResourceManager::getInstance().loadResource<Sphere3D>(sun);
	_sun->setResolution(16);
	_sun->setSize(0.1f);
	_sunNode = SceneManager::getInstance().getActiveScene()->getSceneGraph()->getRoot()->addNode(_sun);
	_sun->setRenderState(false);
	ResourceDescriptor skyboxTextures("SkyboxTextures");
	skyboxTextures.setResourceLocation(location+"skybox_2.jpg "+ location+"skybox_1.jpg "+
								       location+"skybox_5.jpg "+ location+"skybox_6.jpg "+ 
									   location+"skybox_3.jpg "+ location+"skybox_4.jpg");

	_skybox =  ResourceManager::getInstance().loadResource<TextureCubemap>(skyboxTextures);
	ResourceDescriptor skyShaderDescriptor("sky");
	_skyShader = ResourceManager::getInstance().loadResource<Shader>(skyShaderDescriptor);
	assert(_skyShader);
	Console::getInstance().printfn("Generated sky cubemap and sun OK!");
	_init = true;
	return true;
}

void Sky::draw() const{
	if(!_init) return;
	_sky->onDraw();
	_sun->onDraw();
	if (_drawSky && _drawSun) drawSkyAndSun();
	if (_drawSky && !_drawSun) drawSky();
	if (!_drawSky && _drawSun) drawSun();
	GFXDevice::getInstance().clearBuffers(GFXDevice::DEPTH_BUFFER);
}

void Sky::setParams(const vec3& eyePos, const vec3& sunVect, bool invert, bool drawSun, bool drawSky) {
	if(!_init) load();
	_eyePos = eyePos;	_sunVect = sunVect;
	_invert = invert;	_drawSun = drawSun;
	_drawSky = drawSky;
}

void Sky::drawSkyAndSun() const {
	_skyNode->getTransform()->setPosition(_eyePos);
	_skyNode->getTransform()->scaleY(_invert ? -1.0f : 1.0f);
	GFXDevice::getInstance().ignoreStateChanges(true);

	RenderState s(false,false,false,true);
	GFXDevice::getInstance().setRenderState(s,true);

	_skybox->Bind(0);
	_skyShader->bind();
	
		_skyShader->UniformTexture("texSky", 0);
		_skyShader->Uniform("enable_sun", true);
		_skyShader->Uniform("sun_vector", _sunVect);
		GFXDevice::getInstance().setObjectState(_skyNode->getTransform());
		GFXDevice::getInstance().renderModel(_sky);
		GFXDevice::getInstance().releaseObjectState(_skyNode->getTransform());
	
	_skyShader->unbind();
	_skybox->Unbind(0);
	GFXDevice::getInstance().ignoreStateChanges(false);
	
}

void Sky::drawSky() const {
	_skyNode->getTransform()->setPosition(_eyePos);
	_skyNode->getTransform()->scaleY(_invert ? -1.0f : 1.0f);
	GFXDevice::getInstance().ignoreStateChanges(true);

	RenderState s(false,false,false,true);
	GFXDevice::getInstance().setRenderState(s,true);

	_skybox->Bind(0);
	_skyShader->bind();
	
		_skyShader->UniformTexture("texSky", 0);
		_skyShader->Uniform("enable_sun", false);

		GFXDevice::getInstance().setObjectState(_skyNode->getTransform());
		GFXDevice::getInstance().renderModel(_sky);
		GFXDevice::getInstance().releaseObjectState(_skyNode->getTransform());
	
	_skyShader->unbind();
	_skybox->Unbind(0);
	
	GFXDevice::getInstance().ignoreStateChanges(false);

}

void Sky::drawSun() const {
	Light* l = LightManager::getInstance().getLight(0);
	if(l){
		_sun->getMaterial()->setDiffuse(l->getDiffuseColor());
	}
	_sunNode->getTransform()->setPosition(vec3(_eyePos.x-_sunVect.x,_eyePos.y-_sunVect.y,_eyePos.z-_sunVect.z));
	
	GFXDevice::getInstance().ignoreStateChanges(true);
	RenderState s(false,false,false,true);
	GFXDevice::getInstance().setRenderState(s);

	GFXDevice::getInstance().setObjectState(_sunNode->getTransform());
	GFXDevice::getInstance().renderModel(_sun);
	GFXDevice::getInstance().releaseObjectState(_sunNode->getTransform());

	GFXDevice::getInstance().ignoreStateChanges(false);
}


