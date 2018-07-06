#ifndef _SHADER_HANDLER_H_
#define _SHADER_HANDLER_H_

#include "Utility/Headers/MathClasses.h"
#include "Utility/Headers/BaseClasses.h"

class Shader : public Resource
{

public:
	
    virtual void init(const std::string &vsFile, const std::string &fsFile) = 0;
	bool unload(){unbind(); return true;}

    virtual bool loadVertOnly(const std::string& name) = 0;
	virtual bool loadFragOnly(const std::string& name) = 0;

	virtual void bind() = 0;
	virtual void unbind() = 0;
	
	virtual U32 getId() = 0;
	virtual std::string& getName() = 0;

	virtual void Uniform(const std::string& ext, int value) = 0;
	virtual void Uniform(const std::string& ext, F32 value) = 0 ;
	virtual void Uniform(const std::string& ext, const vec2& value) = 0;
	virtual void Uniform(const std::string& ext, const vec3& value) = 0;
	virtual void UniformTexture(const std::string& ext, int slot) = 0;

	virtual ~Shader(){};

protected:
	virtual char* shaderFileRead(const std::string &fn) = 0;
	virtual int   shaderFileWrite(char *fn, char *s) = 0;

protected:
	U32 _shaderId;
	U32 _shaderVP;
	U32 _shaderFP;
	
};


#endif
