#include "Headers/Sky.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/LightManager.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/GFXDevice.h"
#include "Hardware/Video/RenderStateBlock.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

Sky::Sky() : _skyShader(NULL), _skybox(NULL), _init(false), _exclusionMask(0){}

Sky::~Sky(){
	SAFE_DELETE(_skyboxRenderState);
	RemoveResource(_skyShader);
	RemoveResource(_skybox);
}

bool Sky::load() {
	if(_init) return false;
   	std::string location = ParamHandler::getInstance().getParam<std::string>("assetsLocation")+"/misc_images/";
	ResourceDescriptor skybox("SkyBox");
	skybox.setFlag(true); //no default material;
	_sky = CreateResource<Sphere3D>(skybox);
	_sky->setResolution(4);
	_skyNode = GET_ACTIVE_SCENE()->getSceneGraph()->getRoot()->addNode(_sky);
	_sky->setDrawState(false);
	ResourceDescriptor sun("Sun");
	sun.setFlag(true);
	_sun = CreateResource<Sphere3D>(sun);
	_sun->setResolution(16);
	_sun->setRadius(0.1f);
	_sunNode = GET_ACTIVE_SCENE()->getSceneGraph()->getRoot()->addNode(_sun);
	_sun->setDrawState(false);
	ResourceDescriptor skyboxTextures("SkyboxTextures");
	skyboxTextures.setResourceLocation(location+"skybox_2.jpg "+ location+"skybox_1.jpg "+
								       location+"skybox_5.jpg "+ location+"skybox_6.jpg "+ 
									   location+"skybox_3.jpg "+ location+"skybox_4.jpg");

	_skybox =  CreateResource<TextureCubemap>(skyboxTextures);
	ResourceDescriptor skyShaderDescriptor("sky");
	_skyShader = CreateResource<ShaderProgram>(skyShaderDescriptor);
	assert(_skyShader);
	//_sky->getGeometryVBO()->setShaderProgram(_skyShader);
	PRINT_FN(Locale::get("CREATE_SKY_RES_OK"));
	///The sky doesn't cast shadows, doesn't need ambient occlusion and doesn't have real "depth"
	addToDrawExclusionMask(SHADOW_STAGE | SSAO_STAGE | DEPTH_STAGE);

	///Generate a render state
	RenderStateBlockDescriptor skyboxDesc;
	skyboxDesc.setCullMode(CULL_MODE_None);
	skyboxDesc.setZReadWrite(false,false);
	skyboxDesc._fixedLighting = false;
    _skyboxRenderState = GFX_DEVICE.createStateBlock(skyboxDesc);
	_init = true;
	return true;
}

void Sky::draw() const{
	RENDER_STAGE t = GFX_DEVICE.getRenderStage();
	//If we shouldn't draw in the current render stage, return
	if(!getDrawState(GFX_DEVICE.getRenderStage())) return;
	_sky->onDraw();
	_sun->onDraw();
	SET_STATE_BLOCK(_skyboxRenderState);
	if (_drawSky && _drawSun) drawSkyAndSun();
	if (_drawSky && !_drawSun) drawSky();
	if (!_drawSky && _drawSun) drawSun();

	//Do not process depth test results!
	GFX_DEVICE.clearBuffers(GFXDevice::DEPTH_BUFFER);
}

void Sky::setParams(const vec3<F32>& eyePos, const vec3<F32>& sunVect, bool invert, bool drawSun, bool drawSky) {
	if(!_init) load();
	_eyePos = eyePos;	_sunVect = sunVect;
	_invert = invert;	_drawSun = drawSun;
	_drawSky = drawSky;
}

void Sky::drawSkyAndSun() const {
	_skyNode->getTransform()->setPosition(_eyePos);
	_skyNode->getTransform()->scaleY(_invert ? -1.0f : 1.0f);

	_skybox->Bind(0);
	_skyShader->bind();
	
		_skyShader->UniformTexture("texSky", 0);
		_skyShader->Uniform("enable_sun", true);
		_skyShader->Uniform("sun_vector", _sunVect);

		GFX_DEVICE.setObjectState(_skyNode->getTransform());
		GFX_DEVICE.renderModel(_sky);
		GFX_DEVICE.releaseObjectState(_skyNode->getTransform());
	
	_skybox->Unbind(0);
	
}

void Sky::drawSky() const {
	_skyNode->getTransform()->setPosition(_eyePos);
	_skyNode->getTransform()->scaleY(_invert ? -1.0f : 1.0f);

	_skybox->Bind(0);
	_skyShader->bind();
	
		_skyShader->UniformTexture("texSky", 0);
		_skyShader->Uniform("enable_sun", false);

		GFX_DEVICE.setObjectState(_skyNode->getTransform());
		GFX_DEVICE.renderModel(_sky);
		GFX_DEVICE.releaseObjectState(_skyNode->getTransform());
	
	_skybox->Unbind(0);
	
}

void Sky::drawSun() const {
	Light* l = LightManager::getInstance().getLight(0);
	if(l){
		_sun->getMaterial()->setDiffuse(l->getDiffuseColor());
	}
	_sunNode->getTransform()->setPosition(vec3<F32>(_eyePos.x-_sunVect.x,_eyePos.y-_sunVect.y,_eyePos.z-_sunVect.z));

	GFX_DEVICE.setMaterial(_sun->getMaterial());
	GFX_DEVICE.setObjectState(_sunNode->getTransform());
	GFX_DEVICE.renderModel(_sun);
	GFX_DEVICE.releaseObjectState(_sunNode->getTransform());

}


void Sky::removeFromDrawExclusionMask(I32 stageMask) {
	assert((stageMask & ~(INVALID_STAGE-1)) == 0);
	_exclusionMask &= ~stageMask;
}

void Sky::addToDrawExclusionMask(I32 stageMask) {
	assert((stageMask & ~(INVALID_STAGE-1)) == 0);
	_exclusionMask |= static_cast<RENDER_STAGE>(stageMask);
}

bool Sky::getDrawState(RENDER_STAGE currentStage)  const{
	if(!_init) return false;
	return (_exclusionMask & currentStage) == currentStage ? false : true;
}
