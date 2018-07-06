#include "Headers/Sky.h"

#include "Rendering/Headers/Frustum.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/LightManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

Sky::Sky(const std::string& name) : SceneNode(TYPE_SKY),
                                    _skyShader(NULL),
									_skybox(NULL),
									_skyGeom(NULL),
									_init(false),
									_exclusionMask(0)
{
	///The sky doesn't cast shadows, doesn't need ambient occlusion and doesn't have real "depth"
	addToDrawExclusionMask(DEPTH_STAGE);

	///Generate a render state
	RenderStateBlockDescriptor skyboxDesc;
	skyboxDesc.setCullMode(CULL_MODE_NONE);
	skyboxDesc.setZReadWrite(false,false);
    _skyboxRenderState = GFX_DEVICE.createStateBlock(skyboxDesc);
}

Sky::~Sky(){
	SAFE_DELETE(_skyboxRenderState);
	RemoveResource(_skyShader);
	RemoveResource(_skybox);
}

bool Sky::load() {
	if(_init) return false;
   	std::string location = ParamHandler::getInstance().getParam<std::string>("assetsLocation")+"/misc_images/";

	SamplerDescriptor skyboxSampler;
	skyboxSampler.toggleMipMaps(false);
	skyboxSampler.setAnisotrophy(16);
	skyboxSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);

	ResourceDescriptor skyboxTextures("SkyboxTextures");
	skyboxTextures.setResourceLocation(location+"skybox_2.jpg "+ location+"skybox_1.jpg "+
								       location+"skybox_5.jpg "+ location+"skybox_6.jpg "+
									   location+"skybox_3.jpg "+ location+"skybox_4.jpg");
    skyboxTextures.setEnumValue(TEXTURE_CUBE_MAP);
	skyboxTextures.setPropertyDescriptor<SamplerDescriptor>(skyboxSampler);
	_skybox =  CreateResource<TextureCubemap>(skyboxTextures);

	ResourceDescriptor skybox("SkyBox");
	skybox.setFlag(true); //no default material;
	_sky = CreateResource<Sphere3D>(skybox);

	ResourceDescriptor sun("Sun");
	sun.setFlag(true);
	_sun = CreateResource<Sphere3D>(sun);

	ResourceDescriptor skyShaderDescriptor("sky");
	_skyShader = CreateResource<ShaderProgram>(skyShaderDescriptor);

	_skyShader->setMatrixMask(false,false,false,true);
	assert(_skyShader);
	_sky->setResolution(4);
	_sun->setResolution(16);
	_sun->setRadius(0.1f);
	//_sky->getGeometryVBO()->setShaderProgram(_skyShader);
	setRenderingOptions(true,true);
	PRINT_FN(Locale::get("CREATE_SKY_RES_OK"));
	_init = true;
	return true;
}

void Sky::postLoad(SceneGraphNode* const sgn){
	if(!_init) load();
	_sunNode = sgn->addNode(_sun);
	_skyGeom = sgn->addNode(_sky);
	_sky->getSceneNodeRenderState().setDrawState(false);
	_sun->getSceneNodeRenderState().setDrawState(false);
    _sky->setCustomShader(_skyShader);
    _sun->setCustomShader(_skyShader);
	//getSceneNodeRenderState().setDrawState(false);
}

void Sky::prepareMaterial(SceneGraphNode* const sgn) {
	SET_STATE_BLOCK(_skyboxRenderState);
}

void Sky::releaseMaterial(){
}

void Sky::onDraw(const RenderStage& currentStage){
	_sky->onDraw(currentStage);
	_sun->onDraw(currentStage);
}

void Sky::render(SceneGraphNode* const sgn){
	vec3<F32> eyeTemp(Frustum::getInstance().getEyePos());

	sgn->getTransform()->setPosition(eyeTemp);

	if (_drawSky){
		_skyShader->bind();
		_skybox->Bind(0);
		_skyShader->UniformTexture("texSky", 0);
		_sky->renderInstance()->transform(sgn->getTransform());

		GFX_DEVICE.renderInstance(_sky->renderInstance());

		_skybox->Unbind(0);
	}
	if (!_drawSky && _drawSun) {
		Light* l = LightManager::getInstance().getLight(0);
		if(l){
			_sun->getMaterial()->setDiffuse(l->getDiffuseColor());
		}
		_sunNode->getTransform()->setPosition(eyeTemp - _sunVect);
		_sun->renderInstance()->transform(_sunNode->getTransform());

		GFX_DEVICE.renderInstance(_sun->renderInstance());
	}
}

void Sky::setRenderingOptions(bool drawSun, bool drawSky) {
	_drawSun = drawSun;
	_drawSky = drawSky;
	_skyShader->Uniform("enable_sun", _drawSun);
}

void Sky::setSunVector(const vec3<F32>& sunVect) {
	_sunVect = sunVect;
	_skyShader->Uniform("sun_vector", _sunVect);
}

void Sky::removeFromDrawExclusionMask(I32 stageMask) {
	assert((stageMask & ~(INVALID_STAGE-1)) == 0);
	_exclusionMask &= ~stageMask;
}

void Sky::addToDrawExclusionMask(I32 stageMask) {
	assert((stageMask & ~(INVALID_STAGE-1)) == 0);
	_exclusionMask |= static_cast<RenderStage>(stageMask);
}

bool Sky::getDrawState(const RenderStage& currentStage)  const{
	if(!_init) return false;
	return !bitCompare(_exclusionMask,currentStage);
}