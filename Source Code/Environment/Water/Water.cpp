#include "Headers/Water.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Core/Headers/Application.h"
#include "Graphs/Headers/RenderQueue.h"
#include "Core/Headers/ParamHandler.h"
using namespace std;

WaterPlane::WaterPlane() : SceneNode(), _plane(NULL), _texture(NULL), _shader(NULL),
						   _planeTransform(NULL), _node(NULL), _planeSGN(NULL){
	_reflectionFBO = GFXDevice::getInstance().newFBO();
}

bool WaterPlane::load(const std::string& name){
	ResourceDescriptor waterTexture("waterTexture");
	ResourceDescriptor waterShader("water");
	ResourceDescriptor waterPlane("waterPlane");
	waterTexture.setResourceLocation(ParamHandler::getInstance().getParam<string>("assetsLocation")+"/misc_images/terrain_water_NM.jpg");
	waterPlane.setFlag(true); //No default material

	_texture = ResourceManager::getInstance().loadResource<Texture2D>(waterTexture);
	_shader = ResourceManager::getInstance().loadResource<Shader>(waterShader);
	_plane = ResourceManager::getInstance().loadResource<Quad3D>(waterPlane);
	if(!_texture && !_shader && !_plane) return false;
	_name = name; //optional

	_shader->bind();
		_shader->Uniform("water_shininess",50.0f );
		_shader->Uniform("noise_tile", 50.0f );
		_shader->Uniform("noise_factor", 0.1f);
	_shader->unbind();

	_farPlane = 2.0f * ParamHandler::getInstance().getParam<F32>("zFar");
	const vec3& eyePos = CameraManager::getInstance().getActiveCamera()->getEye();
	_plane->setCorner(Quad3D::TOP_LEFT, vec3(eyePos.x - _farPlane, 0, eyePos.z - _farPlane));
	_plane->setCorner(Quad3D::TOP_RIGHT, vec3(eyePos.x + _farPlane, 0, eyePos.z - _farPlane));
	_plane->setCorner(Quad3D::BOTTOM_LEFT, vec3(eyePos.x - _farPlane, 0, eyePos.z + _farPlane));
	_plane->setCorner(Quad3D::BOTTOM_RIGHT, vec3(eyePos.x + _farPlane, 0, eyePos.z + _farPlane));
	_plane->setRenderState(false);
	
	Console::getInstance().printfn("Creating FBO for water reflections in object [ %s ]",getName().c_str());
	_reflectionFBO->Create(FrameBufferObject::FBO_2D_COLOR, 2048, 2048);
	return true;
}

void WaterPlane::postLoad(SceneGraphNode* const node){
	_node = node;
	_planeSGN = node->addNode(_plane);
	_planeSGN->setActive(false);
	_planeTransform = _planeSGN->getTransform();
	//The sky doesn't cast shadows, doesn't need ambient occlusion and doesn't have real "depth"
	addToRenderExclusionMask(SHADOW_STAGE | SSAO_STAGE | DEPTH_STAGE);
}

bool WaterPlane::computeBoundingBox(SceneGraphNode* const node){
	BoundingBox& bb = _node->getBoundingBox();
	if(bb.isComputed()) return true;
	F32 waterLevel = SceneManager::getInstance().getActiveScene()->getWaterLevel();
	F32 waterDepth = SceneManager::getInstance().getActiveScene()->getWaterDepth();
	bb.set(vec3(-_farPlane,waterLevel - waterDepth, -_farPlane),vec3(_farPlane, waterLevel, _farPlane));
	Console::getInstance().printfn("Water plane height placement: %f", bb.getMax().y);
	Console::getInstance().printfn("Water plane depth level: %f", bb.getMin().y);
	bool state = SceneNode::computeBoundingBox(node);
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
	if(_shader)  RemoveResource(_shader);
	if(_texture) RemoveResource(_texture);
	if(_reflectionFBO) {
		delete _reflectionFBO;
		_reflectionFBO = NULL;
	}
	if(!_shader && !_texture && !_reflectionFBO) 
		state = SceneNode::unload();

	if(state) 
		Console::getInstance().printfn("Water unloaded");
	else{
		Console::getInstance().errorfn("Water not unloaded");
		if(_texture)
			Console::getInstance().errorfn("Water texture not unloaded");
		if(_shader)
			Console::getInstance().errorfn("Water shader not unloaded");
		if(_reflectionFBO)
			Console::getInstance().errorfn("Water reflection FBO not unloaded");
	}

	return state;
}




void WaterPlane::setParams(F32 shininess, F32 noiseTile, F32 noiseFactor, F32 transparency){
	_shader->bind();
		_shader->Uniform("water_shininess",shininess );
		_shader->Uniform("noise_tile", noiseTile );
		_shader->Uniform("noise_factor", noiseFactor);
	_shader->unbind();
}

void WaterPlane::prepareMaterial(SceneGraphNode* const sgn){
	GFXDevice::getInstance().ignoreStateChanges(true);
	RenderState s = GFXDevice::getInstance().getActiveRenderState();
	s.blendingEnabled() = true;
	s.cullingEnabled() = false;
	GFXDevice::getInstance().setRenderState(s);
}

void WaterPlane::releaseMaterial(){
	GFXDevice::getInstance().ignoreStateChanges(false);	
	GFXDevice::getInstance().restoreRenderState();
}

void WaterPlane::render(SceneGraphNode* const node){
	if(!getRenderState(GFXDevice::getInstance().getRenderStage())) return;
	const vec3& eyePos = CameraManager::getInstance().getActiveCamera()->getEye();
	BoundingBox& bb = node->getBoundingBox();

	_planeTransform->setPosition(vec3(eyePos.x,bb.getMax().y,eyePos.z));
	changeSortKey(RenderQueue::getInstance().setPriority(RenderingPriority::LAST));

	_reflectionFBO->Bind(0);
	_texture->Bind(1);	
	_shader->bind();

		_shader->UniformTexture("texWaterReflection", 0);
		_shader->UniformTexture("texWaterNoiseNM", 1);	

		GFXDevice::getInstance().setObjectState(_planeTransform);
		GFXDevice::getInstance().renderModel(_plane);
		GFXDevice::getInstance().releaseObjectState(_planeTransform);

	_shader->unbind();
	_texture->Unbind(1);
	_reflectionFBO->Unbind(0);
}