#ifndef GLSL_H_
#define GLSL_H_

#include "Utility/Headers/MathClasses.h"
#include "Utility/Headers/BaseClasses.h"
#include "Hardware/Video/ShaderHandler.h"

class glShader : public Shader
{
public:
	glShader(){};
	glShader(const char *vsFile, const char *fsFile);
	~glShader();
	
    void init(const string &vsFile, const string &fsFile);
	bool load(const string& name);
    bool loadVertOnly(const string& name);
	bool loadFragOnly(const string& name);
	void bind();
	void unbind();
	
	U32 getId();

	void Uniform(const string& ext, int value);
	void Uniform(const string& ext, F32 value);
	void Uniform(const string& ext, const vec2& value);
	void Uniform(const string& ext, const vec3& value);
	void UniformTexture(const string& ext, int slot);

private:
	char* shaderFileRead(const string &fn);
	int   shaderFileWrite(char *fn, char *s);
	void  validateShader(U32 shader, const string &file = 0);
	void  validateProgram(U32 program);
};


#endif
