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
	
    void init(const string& vsFile, const string &fsFile);
	bool load(const string& name);
	void bind();
	void unbind();
	
	U32 getId();
	string& getName() {return _name;}

	void Uniform(const string& ext, int value);
	void Uniform(const string& ext, F32 value);
	void Uniform(const string& ext, bool state);
	void Uniform(const string& ext, const vec2& value);
	void Uniform(const string& ext, const vec3& value);
	void Uniform(const string& ext, const vec4& value);
	void UniformTexture(const string& ext, int slot);

private:
	string _name;
	bool _loaded;
	char* shaderFileRead(const string &fn);
	int   shaderFileWrite(char *fn, char *s);
	void  validateShader(U32 shader, const string &file = 0);
	void  validateProgram(U32 program);

	bool loadVertOnly(const string& name);
	bool loadFragOnly(const string& name);
};


#endif
