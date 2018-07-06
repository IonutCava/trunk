#include "Headers/Sky.h"

#include "Rendering/Headers/Frustum.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/LightManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

Sky::Sky(const std::string& name) : SceneNode(TYPE_SKY),
                                    _skyShader(NULL),
									_skybox(NULL),
									_skyGeom(NULL),
                                    _drawSky(true),
                                    _drawSun(true),
									_init(false),
                                    _invert(false),
                                    _invertPlaneY(0),
									_exclusionMask(0)
{
	setInitialData(name);
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
	ResourceDescriptor skybox("SkyBox");
	skybox.setFlag(true); //no default material;
	ResourceDescriptor sun("Sun");
	sun.setFlag(true);
	ResourceDescriptor skyShaderDescriptor("sky");
	ResourceDescriptor skyboxTextures("SkyboxTextures");
	skyboxTextures.setResourceLocation(location+"skybox_2.jpg "+ location+"skybox_1.jpg "+
								       location+"skybox_5.jpg "+ location+"skybox_6.jpg "+ 
									   location+"skybox_3.jpg "+ location+"skybox_4.jpg");
	_sky = CreateResource<Sphere3D>(skybox);
	_sun = CreateResource<Sphere3D>(sun);
	_skybox =  CreateResource<TextureCubemap>(skyboxTextures);
	_skyShader = CreateResource<ShaderProgram>(skyShaderDescriptor);
	assert(_skyShader);
	_sky->setResolution(4);
	_sun->setResolution(16);
	_sun->setRadius(0.1f);
	//_sky->getGeometryVBO()->setShaderProgram(_skyShader);
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
	//getSceneNodeRenderState().setDrawState(false);
}

void Sky::prepareMaterial(SceneGraphNode* const sgn) {
	SET_STATE_BLOCK(_skyboxRenderState);
}

void Sky::releaseMaterial(){
}

void Sky::onDraw(){
	_sky->onDraw();
	_sun->onDraw();
}

void Sky::render(SceneGraphNode* const sgn){
    if(!_invert){
        const vec3<F32>& eyePos = Frustum::getInstance().getEyePos();
        sgn->getTransform()->setPosition(eyePos);
        _sunNode->getTransform()->setPosition(eyePos - _sunVect);
    }else{
        vec3<F32> eyeTemp(Frustum::getInstance().getEyePos());
        eyeTemp.y = _invertPlaneY - eyeTemp.y;
        sgn->getTransform()->setPosition(eyeTemp);
        _sunNode->getTransform()->setPosition(eyeTemp - _sunVect);
    }

	if (_drawSky){
		_skybox->Bind(0);
		_skyShader->bind();
	
		_skyShader->UniformTexture("texSky", 0);
		_skyShader->Uniform("enable_sun", _drawSun);
		_skyShader->Uniform("sun_vector", _sunVect);

		GFX_DEVICE.setObjectState(sgn->getTransform());
		GFX_DEVICE.renderModel(_sky);
		GFX_DEVICE.releaseObjectState(sgn->getTransform());
	
		_skybox->Unbind(0);
	}
	if (!_drawSky && _drawSun) {
		Light* l = LightManager::getInstance().getLight(0);
		if(l){
			_sun->getMaterial()->setDiffuse(l->getDiffuseColor());
		}

		GFX_DEVICE.setObjectState(_sunNode->getTransform());
		GFX_DEVICE.renderModel(_sun);
		GFX_DEVICE.releaseObjectState(_sunNode->getTransform());
	}
}

void Sky::setRenderingOptions(bool drawSun, bool drawSky) {
	_drawSun = drawSun;
	_drawSky = drawSky;
}

void Sky::setInvertPlane(F32 invertPlaneY){
    _invertPlaneY = invertPlaneY;
}

void Sky::removeFromDrawExclusionMask(I32 stageMask) {
	assert((stageMask & ~(INVALID_STAGE-1)) == 0);
	_exclusionMask &= ~stageMask;
}

void Sky::addToDrawExclusionMask(I32 stageMask) {
	assert((stageMask & ~(INVALID_STAGE-1)) == 0);
	_exclusionMask |= static_cast<RenderStage>(stageMask);
}

bool Sky::getDrawState(RenderStage currentStage)  const{
	if(!_init) return false;
	return !bitCompare(_exclusionMask,currentStage);
}
