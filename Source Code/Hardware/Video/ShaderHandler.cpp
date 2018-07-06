#include "ShaderHandler.h"

bool Shader::load(const std::string& name){
	//Apply global shader values:
	this->bind();
		this->UniformTexture("texDepthMapFromLight0",7); //Shadow Maps always bound to 7 and 8
		this->UniformTexture("texDepthMapFromLight1",8);
	this->unbind();
	return true;
}