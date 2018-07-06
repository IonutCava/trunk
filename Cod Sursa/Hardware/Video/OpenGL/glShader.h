#ifndef GLSL_H_
#define GLSL_H_

#include "Utility/Headers/MathClasses.h"
#include "Utility/Headers/BaseClasses.h"
#include "Hardware/Video/ShaderHandler.h"

class glShader : public Shader
{
public:
	glShader() : _loaded(false) {};
	glShader(const char *vsFile, const char *fsFile);
	~glShader();
	
    void init(const std::string& vsFile, const std::string &fsFile);
	bool load(const std::string& name);
	void bind();
	void unbind();
	
	U32 getId();
	std::string& getName() {return _name;}

	void Uniform(const std::string& ext, int value);
	void Uniform(const std::string& ext, F32 value);
	void Uniform(const std::string& ext, bool state);
	void Uniform(const std::string& ext, const vec2& value);
	void Uniform(const std::string& ext, const vec3& value);
	void Uniform(const std::string& ext, const vec4& value);
	void UniformTexture(const std::string& ext, int slot);

private:
	std::string _name;
	bool _loaded;
	char* shaderFileRead(const std::string &fn);
	int   shaderFileWrite(char *fn, char *s);
	void  validateShader(U32 shader, const std::string &file = 0);
	void  validateProgram(U32 program);

	bool loadVertOnly(const std::string& name);
	bool loadFragOnly(const std::string& name);
};


#endif
