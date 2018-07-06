#include "ShaderHandler.h"
#include "Hardware/Video/GFXDevice.h"
#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/LightManager.h"
#include "Core/Headers/ParamHandler.h" 

bool Shader::load(const std::string& name){
	//Apply global shader values valid throughout application runtime:
	this->bind();
		this->UniformTexture("texDepthMapFromLight0",8); //Shadow Maps always bound to 8, 9 and 10
		this->UniformTexture("texDepthMapFromLight1",9);
		this->UniformTexture("texDepthMapFromLight2",10);
	this->unbind();
	return true;
}

void Shader::bind(){
	//Apply global shader values valid throughout current render call:
	Frustum& frust = Frustum::getInstance();
	ParamHandler& par = ParamHandler::getInstance();
    this->Uniform("modelViewInvMatrix",frust.getModelviewInvMatrix());
	this->Uniform("modelViewMatrix",frust.getModelviewMatrix());
	this->Uniform("modelViewProjectionMatrix",frust.getModelViewProjectionMatrix());
	this->Uniform("lightProjectionMatrix",GFXDevice::getInstance().getLightProjectionMatrix());
	this->Uniform("enable_shadow_mapping",LightManager::getInstance().shadowMappingEnabled());
	this->Uniform("zNear",par.getParam<F32>("zNear"));
	this->Uniform("zFar",par.getParam<F32>("zFar"));
	this->Uniform("time", GETTIME());
}