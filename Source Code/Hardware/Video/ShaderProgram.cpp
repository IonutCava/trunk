#include "ShaderProgram.h"
#include "Hardware/Video/GFXDevice.h"
#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Core/Headers/ParamHandler.h" 
#include "Core/Headers/Application.h" 

U32 ShaderProgram::_prevShaderProgramId = 0;

bool ShaderProgram::checkBinding(U32 newShaderProgramId){
	if(_prevShaderProgramId != newShaderProgramId){
		_prevShaderProgramId = newShaderProgramId;
		return true;
	}
	return false;
}

ShaderProgram::~ShaderProgram(){
	Console::getInstance().d_printfn("Removing shader program [ %s ]", getName().c_str());
	for_each(Shader* s, _shaders){
		ShaderManager::getInstance().removeShader(s);
	}
	_shaders.clear();
}

bool ShaderProgram::load(const std::string& name){
	_name = name;
	//Apply global shader values valid throughout application runtime:
	this->bind();
		this->UniformTexture("texDepthMapFromLight0",8); //Shadow Maps always bound to 8, 9 and 10
		this->UniformTexture("texDepthMapFromLight1",9);
		this->UniformTexture("texDepthMapFromLight2",10);
	this->unbind();
	return true;
}

void ShaderProgram::bind(){
	//Apply global shader values valid throughout current render call:
	Frustum& frust = Frustum::getInstance();
	ParamHandler& par = ParamHandler::getInstance();
	Application& app = Application::getInstance();
	F32 resolutionFactor = 1;
	switch(par.getParam<U8>("shadowDetailLevel")){
		case LOW: {
			 //1/4 the shadowmap resolution
			resolutionFactor = 4;
		}break;
		case MEDIUM: {
			//1/2 the shadowmap resolution
			resolutionFactor = 2;
		}break;
		default:
		case HIGH: {
			//Full shadowmap resolution
			resolutionFactor = 1;
		}break;
	};
	this->Attribute("cameraPosition",frust.getEyePos());
    this->Uniform("modelViewInvMatrix",frust.getModelviewInvMatrix());
	this->Uniform("modelViewMatrix",frust.getModelviewMatrix());
	this->Uniform("modelViewProjectionMatrix",frust.getModelViewProjectionMatrix());
	this->Uniform("lightProjectionMatrix",GFXDevice::getInstance().getLightProjectionMatrix());
	this->Uniform("zNear",par.getParam<F32>("zNear"));
	this->Uniform("zFar",par.getParam<F32>("zFar"));
	this->Uniform("screenWidth", app.getWindowDimensions().width);
	this->Uniform("screenHeight", app.getWindowDimensions().height);
	this->Uniform("resolutionFactor", resolutionFactor);
	this->Uniform("time", GETTIME());
	this->Uniform("enableFog",true);
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