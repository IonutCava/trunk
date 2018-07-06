#include "ShaderProgram.h"
#include "Hardware/Video/GFXDevice.h"
#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Core/Headers/ParamHandler.h" 
#include "Core/Headers/Application.h" 

U32 ShaderProgram::_prevShaderProgramId = 0;
U32 ShaderProgram::_newShaderProgramId = 0;

bool ShaderProgram::checkBinding(U32 newShaderProgramId){
	if(_prevShaderProgramId != newShaderProgramId){
		_newShaderProgramId = newShaderProgramId;
		return true;
	}
	return false;
}

ShaderProgram::~ShaderProgram(){
	D_PRINT_FN("Removing shader program [ %s ]", getName().c_str());
	for_each(Shader* s, _shaders){
		ShaderManager::getInstance().removeShader(s);
	}
	_shaders.clear();
}

bool ShaderProgram::generateHWResource(const std::string& name){
	_name = name;
	//Apply global shader values valid throughout application runtime:
	this->bind();
		this->UniformTexture("texDepthMapFromLight0",8); //Shadow Maps always bound to 8, 9 and 10
		this->UniformTexture("texDepthMapFromLight1",9);
		this->UniformTexture("texDepthMapFromLight2",10);
	this->unbind();
	return HardwareResource::generateHWResource(name);
}

void ShaderProgram::bind(){
	_bound = true;
	_prevShaderProgramId = _newShaderProgramId;
	//Apply global shader values valid throughout current render call:
	Frustum& frust = Frustum::getInstance();
	ParamHandler& par = ParamHandler::getInstance();
	Application& app = Application::getInstance();

	this->Attribute("cameraPosition",frust.getEyePos());
    this->Uniform("modelViewInvMatrix",frust.getModelviewInvMatrix());
	this->Uniform("modelViewMatrix",frust.getModelviewMatrix());
	this->Uniform("modelViewProjectionMatrix",frust.getModelViewProjectionMatrix());
	this->Uniform("lightProjectionMatrix",GFX_DEVICE.getLightProjectionMatrix());
	this->Uniform("zPlanes",vec2<F32>(par.getParam<F32>("zNear"),par.getParam<F32>("zFar")));
	this->Uniform("screenDimension", app.getWindowDimensions());
	this->Uniform("resolutionFactor", par.getParam<U8>("shadowDetailLevel"));
	this->Uniform("time", GETTIME());
	this->Uniform("enableFog",true);
}

void ShaderProgram::unbind(){
	_prevShaderProgramId = 0;
	_bound = false;
}

std::vector<Shader* > ShaderProgram::getShaders(SHADER_TYPE type){
	std::vector<Shader* > returnShaders;
	for_each(Shader* s, _shaders){
		if(s->getType() == type){
			returnShaders.push_back(s);
		}
	}
	return returnShaders;
}