#include "Water.h"
#include "Managers/ResourceManager.h"
#include "Managers/CameraManager.h"
#include "Managers/SceneManager.h"
#include "Rendering/common.h"
#include "Hardware/Video/GFXDevice.h"
using namespace std;

WaterPlane::WaterPlane() : SceneNode(), _maxViewDistance(0), _plane(NULL), _texture(NULL), _shader(NULL){
	for(U8 i = 0; i < 3; i++) 
		_fbo[i] = NULL;
}

bool WaterPlane::load(const std::string& name){
	ResourceDescriptor waterTexture("waterTexture");
	ResourceDescriptor waterShader("water");
	ResourceDescriptor waterPlane("waterPlane");
	waterTexture.setResourceLocation(ParamHandler::getInstance().getParam<string>("assetsLocation")+"/misc_images/terrain_water_NM.jpg");
	waterPlane.setFlag(true);

	_texture = ResourceManager::getInstance().LoadResource<Texture2D>(waterTexture);
	_shader = ResourceManager::getInstance().LoadResource<Shader>(waterShader);
	_plane = ResourceManager::getInstance().LoadResource<Quad3D>(waterPlane);
	if(!_texture && !_shader && !_plane) return false;
	_plane->setMaterial(NULL);
	_plane->useDefaultMaterial(false);
	_plane->setTransform(NULL);
	_plane->useDefaultTransform(false);
	_name = name; //optional
	_waterMaxDepth.set(-256,SceneManager::getInstance().getActiveScene()->getWaterLevel(),-256);
	_waterMinDepth.set(256, _waterMaxDepth.y, 256);
	_waterMaxDepth.y -= SceneManager::getInstance().getActiveScene()->getWaterDepth();

	Console::getInstance().printfn("Water plane height placement: %f", _waterMinDepth.y);
	Console::getInstance().printfn("Water plane depth level: %f", _waterMaxDepth.y);

	_shader->bind();
		_shader->Uniform("fog_color", vec3(0.7f, 0.7f, 0.9f));
		_shader->Uniform("water_shininess",50.0f );
		_shader->Uniform("noise_tile", 1.5f );
		_shader->Uniform("noise_factor", 0.1f);
		_shader->Uniform("transparency",0.5f);
	_shader->unbind();

	_maxViewDistance = 2.0f*ParamHandler::getInstance().getParam<D32>("zFar");
	const vec3& eye_pos = CameraManager::getInstance().getActiveCamera()->getEye();
	_plane->getCorner(Quad3D::TOP_LEFT)     = vec3(eye_pos.x - _maxViewDistance, _waterMinDepth.y, eye_pos.z - _maxViewDistance);
	_plane->getCorner(Quad3D::TOP_RIGHT)    = vec3(eye_pos.x + _maxViewDistance, _waterMinDepth.y, eye_pos.z - _maxViewDistance);
	_plane->getCorner(Quad3D::BOTTOM_LEFT)  = vec3(eye_pos.x - _maxViewDistance, _waterMinDepth.y, eye_pos.z + _maxViewDistance);
	_plane->getCorner(Quad3D::BOTTOM_RIGHT) = vec3(eye_pos.x + _maxViewDistance, _waterMinDepth.y, eye_pos.z + _maxViewDistance);
	return true;
}

bool WaterPlane::unload(){
	bool state = false;
	if(_plane)   ResourceManager::getInstance().remove(_plane);
	if(_shader)  ResourceManager::getInstance().remove(_shader);
	if(_texture) ResourceManager::getInstance().remove(_texture);
	for(U8 i = 0 ; i < 3; i++)
		if(_fbo[i] != NULL){
			delete _fbo[i];
			_fbo[i] = NULL;
		}
	if(!_plane && !_shader && !_texture && !_fbo[0] && !_fbo[1] && !_fbo[2]) 
		state = SceneNode::unload();

	if(state) 
		Console::getInstance().printfn("Water unloaded");
	else{
		Console::getInstance().errorfn("Water not unloaded");
		if(_plane)
			Console::getInstance().errorfn("Water plane not unloaded");
		if(_texture)
			Console::getInstance().errorfn("Water texture not unloaded");
		if(_shader)
			Console::getInstance().errorfn("Water shader not unloaded");
		if(_fbo[0])
			Console::getInstance().errorfn("Water fbo[0] not unloaded");
		if(_fbo[1])
			Console::getInstance().errorfn("Water fbo[1] not unloaded");
		if(_fbo[2])
			Console::getInstance().errorfn("Water fbo[2] not unloaded");

	}

	return state;
}

void WaterPlane::setFBO(FrameBufferObject* fbo[3]){
	_fbo[0] = fbo[0];
	_fbo[1] = fbo[1];
	_fbo[2] = fbo[2];
}

void WaterPlane::setParams(F32 shininess, F32 noiseTile, F32 noiseFactor, F32 transparency){
	_shader->bind();
		_shader->Uniform("water_shininess",shininess );
		_shader->Uniform("noise_tile", noiseTile );
		_shader->Uniform("noise_factor", noiseFactor);
		_shader->Uniform("transparency",transparency);
	_shader->unbind();
}

void WaterPlane::render(){

	if(!_fbo[0] || !_fbo[1] || !_fbo[2]) return;
	RenderState old = GFXDevice::getInstance().getActiveRenderState();
	RenderState s(false,true,true,true);
	GFXDevice::getInstance().setRenderState(s);

	GFXDevice::getInstance().setTextureMatrix(0,_projectionMatrix); //Ce pizda masii am incercat sa fac aici? ~X( -Ionut
	_fbo[0]->Bind(0);
	_fbo[1]->Bind(1);
	_fbo[2]->Bind(2);
	_texture->Bind(3);	

	_shader->bind();

		_shader->UniformTexture("texWaterReflection", 0);
		_shader->UniformTexture("texDepthMapFromLight0", 1);
		_shader->UniformTexture("texDepthMapFromLight1", 2);
		_shader->UniformTexture("texWaterNoiseNM", 3);		
	
		_shader->Uniform("time", GETTIME());
		_shader->Uniform("win_width",  (I32)Engine::getInstance().getWindowDimensions().width);
		_shader->Uniform("win_height", (I32)Engine::getInstance().getWindowDimensions().height);
		const vec3& eye_pos = CameraManager::getInstance().getActiveCamera()->getEye();
		_maxViewDistance = 2.0f*ParamHandler::getInstance().getParam<D32>("zFar");
		_plane->getCorner(Quad3D::TOP_LEFT)     = vec3(eye_pos.x - _maxViewDistance, _waterMinDepth.y, eye_pos.z - _maxViewDistance);
		_plane->getCorner(Quad3D::TOP_RIGHT)    = vec3(eye_pos.x + _maxViewDistance, _waterMinDepth.y, eye_pos.z - _maxViewDistance);
		_plane->getCorner(Quad3D::BOTTOM_LEFT)  = vec3(eye_pos.x - _maxViewDistance, _waterMinDepth.y, eye_pos.z + _maxViewDistance);
		_plane->getCorner(Quad3D::BOTTOM_RIGHT) = vec3(eye_pos.x + _maxViewDistance, _waterMinDepth.y, eye_pos.z + _maxViewDistance);
		_plane->computeBoundingBox();
		_waterMinDepth = _plane->getBoundingBox().getMin();
		_waterMaxDepth = _plane->getBoundingBox().getMax();
		_waterMaxDepth.y -= SceneManager::getInstance().getActiveScene()->getWaterDepth();
		_shader->Uniform("water_min_depth",_waterMinDepth);
		_shader->Uniform("water_max_depth",_waterMaxDepth);
		GFXDevice::getInstance().drawQuad3D(_plane);

	_shader->unbind();

	_texture->Unbind(3);
	_fbo[2]->Unbind(2);
	_fbo[1]->Unbind(1);
	_fbo[0]->Unbind(0);

	GFXDevice::getInstance().restoreTextureMatrix(0);

	GFXDevice::getInstance().setRenderState(old);
}