#ifndef _SHADER_HANDLER_H_
#define _SHADER_HANDLER_H_

#include "Utility/Headers/BaseClasses.h"

class Shader : public Resource
{

public:
	
    virtual void init(const std::string &vsFile, const std::string &fsFile) = 0;

    virtual bool loadVertOnly(const std::string& name) = 0;
	virtual bool loadFragOnly(const std::string& name) = 0;

	virtual void bind() = 0;
	virtual void unbind() = 0;
	
	virtual U16 getId() = 0;

	virtual void Uniform(const std::string& ext, I32 value) = 0;
	virtual void Uniform(const std::string& ext, F32 value) = 0 ;
	virtual void Uniform(const std::string& ext, const vec2& value) = 0;
	virtual void Uniform(const std::string& ext, const vec3& value) = 0;
	virtual void Uniform(const std::string& ext, const vec4& value) = 0;
	virtual void Uniform(const std::string& ext, const mat3& value) = 0;
	virtual void Uniform(const std::string& ext, const mat4& value) = 0;
	virtual void UniformTexture(const std::string& ext, U16 slot) = 0;

	//Legacy
	virtual void Uniform(I32 location, const vec4& value) = 0;
	virtual ~Shader(){}
protected:
	virtual char* shaderFileRead(const std::string &fn) = 0;
	virtual I8   shaderFileWrite(char *fn, char *s) = 0;

protected:
	U32 _shaderId;
	U32 _shaderVP;
	U32 _shaderFP;
	
};


#endif
