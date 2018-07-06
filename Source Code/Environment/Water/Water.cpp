#include "Headers/Water.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/RenderStateBlock.h"

using namespace std;

WaterPlane::WaterPlane() : SceneNode(TYPE_WATER), Reflector(TYPE_WATER_SURFACE,vec2<I32>(2048,2048)), 
						   _plane(NULL),_texture(NULL), _shader(NULL),_planeTransform(NULL),
						   _node(NULL),_planeSGN(NULL),_waterStateBlock(NULL),_waterLevel(0){}

bool WaterPlane::load(const std::string& name){
	ResourceDescriptor waterTexture("waterTexture");
	ResourceDescriptor waterShader("water");
	ResourceDescriptor waterPlane("waterPlane");
	waterTexture.setResourceLocation(ParamHandler::getInstance().getParam<string>("assetsLocation")+"/misc_images/terrain_water_NM.jpg");
	waterPlane.setFlag(true); //No default material
	//The material is responsible for the destruction of the textures and shaders it receives!!!!
	_texture = CreateResource<Texture2D>(waterTexture);
	_shader = CreateResource<ShaderProgram>(waterShader);
	_plane = CreateResource<Quad3D>(waterPlane);
	if(!_texture && !_shader && !_plane) return false;
	ResourceDescriptor waterMaterial("waterMaterial");
	setMaterial(CreateResource<Material>(waterMaterial));
	assert(getMaterial() != NULL);
	getMaterial()->setTexture(Material::TEXTURE_BASE, _texture);
	getMaterial()->setShaderProgram(_shader->getName());

	RenderStateBlockDescriptor waterMatDesc = getMaterial()->getRenderState(FINAL_STAGE)->getDescriptor();
	waterMatDesc.setCullMode(CULL_MODE_None);
	waterMatDesc.setBlend(true, BLEND_PROPERTY_SrcAlpha, BLEND_PROPERTY_InvSrcAlpha);
	_waterStateBlock = getMaterial()->setRenderStateBlock(waterMatDesc,FINAL_STAGE);

	_name = name; //optional

	_shader->bind();
		_shader->Uniform("water_shininess",50.0f );
		_shader->Uniform("noise_tile", 50.0f );
		_shader->Uniform("noise_factor", 0.1f);
	_shader->unbind();

	_farPlane = 2.0f * ParamHandler::getInstance().getParam<F32>("zFar");
	const vec3<F32>& eyePos = CameraManager::getInstance().getActiveCamera()->getEye();
	_plane->setCorner(Quad3D::TOP_LEFT, vec3<F32>(eyePos.x - _farPlane, 0, eyePos.z - _farPlane));
	_plane->setCorner(Quad3D::TOP_RIGHT, vec3<F32>(eyePos.x + _farPlane, 0, eyePos.z - _farPlane));
	_plane->setCorner(Quad3D::BOTTOM_LEFT, vec3<F32>(eyePos.x - _farPlane, 0, eyePos.z + _farPlane));
	_plane->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>(eyePos.x + _farPlane, 0, eyePos.z + _farPlane));
	_plane->setDrawState(false);
	
	return true;
}

void WaterPlane::postLoad(SceneGraphNode* const sgn){
	_node = sgn;
	_planeSGN = _node->addNode(_plane);
	_planeSGN->setActive(false);
	_planeTransform = _planeSGN->getTransform();
	///The water doesn't cast shadows, doesn't need ambient occlusion and doesn't have real "depth"
	addToDrawExclusionMask(SHADOW_STAGE | SSAO_STAGE | DEPTH_STAGE);
}

bool WaterPlane::computeBoundingBox(SceneGraphNode* const sgn){
	BoundingBox& bb = _node->getBoundingBox();
	if(bb.isComputed()) return true;
	_waterLevel = SceneManager::getInstance().getActiveScene()->getWaterLevel();
	F32 waterDepth = SceneManager::getInstance().getActiveScene()->getWaterDepth();
	bb.set(vec3<F32>(-_farPlane,_waterLevel - waterDepth, -_farPlane),vec3<F32>(_farPlane, _waterLevel, _farPlane));
	_planeSGN->getBoundingBox().Add(bb);
	PRINT_FN("Water plane height placement: %f", bb.getMax().y);
	PRINT_FN("Water plane depth level: %f", bb.getMin().y);
	bool state = SceneNode::computeBoundingBox(sgn);
	_shader->bind();
		_shader->Uniform("win_width",  (I32)Application::getInstance().getWindowDimensions().width);
		_shader->Uniform("win_height", (I32)Application::getInstance().getWindowDimensions().height);
		_shader->Uniform("water_bb_min",bb.getMin());
		_shader->Uniform("water_bb_max",bb.getMax());
	_shader->unbind();
	return state;
}

bool WaterPlane::unload(){
	bool state = false;
	state = SceneNode::unload();
	return state;
}


void WaterPlane::setParams(F32 shininess, F32 noiseTile, F32 noiseFactor, F32 transparency){
	_shader->bind();
		_shader->Uniform("water_shininess",shininess );
		_shader->Uniform("noise_tile", noiseTile );
		_shader->Uniform("noise_factor", noiseFactor);
	_shader->unbind();
}

void WaterPlane::onDraw(){
	const vec3<F32>& eyePos = CameraManager::getInstance().getActiveCamera()->getEye();
	BoundingBox& bb = _node->getBoundingBox();
	_planeTransform->setPosition(vec3<F32>(eyePos.x,bb.getMax().y,eyePos.z));
}

void WaterPlane::prepareMaterial(SceneGraphNode const* const sgn){
	
	SET_STATE_BLOCK(_waterStateBlock);

	GFX_DEVICE.setMaterial(getMaterial());
	
	_reflectedTexture->Bind(0);
	_texture->Bind(1);	
	_shader->bind();
	_shader->UniformTexture("texWaterReflection", 0);
	_shader->UniformTexture("texWaterNoiseNM", 1);
}

void WaterPlane::releaseMaterial(){

	_texture->Unbind(1);
	_reflectedTexture->Unbind(0);
}

void WaterPlane::render(SceneGraphNode* const sgn){
	
	GFX_DEVICE.setObjectState(_planeTransform);
	GFX_DEVICE.renderModel(_plane);
	GFX_DEVICE.releaseObjectState(_planeTransform);
}

bool WaterPlane::getDrawState(RENDER_STAGE currentStage)  const{
	/// Wait for the Reflector to update
	if(!_createdFBO){
		return false;
	}
	/// Make sure we are not drawing ourself
	if(_updateSelf){
		/// unless this is desired
		return false;
	}
	/// Else, process normal exclusion
	return SceneNode::getDrawState(currentStage);
}

/// Update water reflections
void WaterPlane::updateReflection(){
	/// Early out check for render callback
	if(_renderCallback.empty()) return;
    /// Set the correct stage
	RENDER_STAGE prev = GFX_DEVICE.getRenderStage();
	bool underwater = isCameraUnderWater();
	if(!underwater){
		GFX_DEVICE.setRenderStage(REFLECTION_STAGE);
	}
	/// bind the reflective texture
	_reflectedTexture->Begin();
		/// render to the reflective texture
		_renderCallback();
	_reflectedTexture->End();

	if(!underwater){
		GFX_DEVICE.setRenderStage(prev);
	}
}

bool WaterPlane::isCameraUnderWater(){
	Camera* cam = CameraManager::getInstance().getActiveCamera();
	return (cam->getEye().y < _waterLevel);
}