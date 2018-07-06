#include "Headers/ShaderGenerator.h"

ShaderGenerator::ShaderGenerator(){
}

ShaderGenerator::~ShaderGenerator(){
}

Shader* ShaderGenerator::generateShader(const vectorImpl<ShaderStageDescriptor> & stages){
	for(U8 i = 0; i < stages.size(); i++){
		switch(stages[i]._stage){
			case ShaderStageDescriptor::SHADER_NUM_LIGHTS:
				break;
			case ShaderStageDescriptor::SHADER_PHONG:
				break;
			case ShaderStageDescriptor::SHADER_BLIN:
				break;
			case ShaderStageDescriptor::SHADER_BUMP:
				break;
			case ShaderStageDescriptor::SHADER_PARALLAX:
				break;
			case ShaderStageDescriptor::SHADER_SHADOW:
				break;
			case ShaderStageDescriptor::SHADER_SMOOTH_SHADOW:
				break;
			case ShaderStageDescriptor::SHADER_SPECULAR_MAP:
				break;
			case ShaderStageDescriptor::SHADER_OPACITY_MAP:
				break;
			case ShaderStageDescriptor::SHADER_FOG:
				break;
			default:
				break;
		}
	}
	return NULL;
}