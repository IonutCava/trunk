#include "Headers/ShaderProgram.h"
#include "Hardware/Video/Headers/GFXDevice.h"
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
	D_PRINT_FN(Locale::get("SHADER_PROGRAM_REMOVE"), getName().c_str());
	for_each(Shader* s, _shaders){
		ShaderManager::getInstance().removeShader(s);
	}
	_shaders.clear();
}

bool ShaderProgram::generateHWResource(const std::string& name){
	_name = name;
	_maxCombinedTextureUnits = ParamHandler::getInstance().getParam<I32>("GFX_DEVICE.maxTextureCombinedUnits",16);
	//Apply global shader values valid throughout application runtime:
	std::string shadowMaps("texDepthMapFromLight");
	this->bind();
		for(I32 i = 0; i < MAX_SHADOW_CASTING_LIGHTS_PER_NODE; i++){
			std::stringstream ss;
			ss << shadowMaps;
			ss << i;
			this->UniformTexture(ss.str(),10+i); //Shadow Maps always bound from 8 and above
		}
		this->UniformTexture("texDepthMapFromLightArray",_maxCombinedTextureUnits - 2); //Reserve first for directional shadows
		this->UniformTexture("texDepthMapFromLightCube",_maxCombinedTextureUnits - 1); //Reserve second for point shadows
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
	this->Uniform("projectionMatrix",frust.getProjectionMatrix());
	this->Uniform("zPlanes",frust.getZPlanes());
	this->Uniform("screenDimension", app.getResolution());
	this->Uniform("enableFog",par.getParam<bool>("rendering.enableFog"));
	this->Uniform("time", GETMSTIME());

}

void ShaderProgram::unbind(){
	_prevShaderProgramId = 0;
	_bound = false;
}

vectorImpl<Shader* > ShaderProgram::getShaders(ShaderType type){
	vectorImpl<Shader* > returnShaders;
	for_each(Shader* s, _shaders){
		if(s->getType() == type){
			returnShaders.push_back(s);
		}
	}
	return returnShaders;
}