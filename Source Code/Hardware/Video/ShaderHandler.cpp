#include "ShaderHandler.h"
#include "Hardware/Video/GFXDevice.h"
#include "Rendering/Frustum.h"

bool Shader::load(const std::string& name){
	//Apply global shader values valid throughout application runtime:
	this->bind();
		this->UniformTexture("texDepthMapFromLight0",7); //Shadow Maps always bound to 7 and 8
		this->UniformTexture("texDepthMapFromLight1",8);
	this->unbind();
	return true;
}

void Shader::bind(){
	//Apply global shader values valid throughout current render call:
	Frustum& frust = Frustum::getInstance();
    this->Uniform("modelViewInvMatrix",frust.getModelviewInvMatrix());
	this->Uniform("modelViewMatrix",frust.getModelviewMatrix());
	this->Uniform("modelViewProjectionMatrix",frust.getModelViewProjectionMatrix());
	this->Uniform("lightProjectionMatrix",GFXDevice::getInstance().getLightProjectionMatrix());
	this->Uniform("time", GETTIME());
}