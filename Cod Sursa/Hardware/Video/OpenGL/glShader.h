#ifndef GLSL_H_
#define GLSL_H_

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
	bool unload(){unbind(); return true;}
	void bind();
	void unbind();
	
	U16 getId();

	void Uniform(const std::string& ext, I32 value);
	void Uniform(const std::string& ext, F32 value);
	void Uniform(const std::string& ext, bool state);
	void Uniform(const std::string& ext, const vec2& value);
	void Uniform(const std::string& ext, const vec3& value);
	void Uniform(const std::string& ext, const vec4& value);
	void Uniform(const std::string& ext, const mat3& value);
    void Uniform(const std::string& ext, const mat4& value);
	void UniformTexture(const std::string& ext, U16 slot);

	//Legacy
	void Uniform(I32 location, const vec4& value);

private:

	bool _loaded;
	char* shaderFileRead(const std::string &fn);
	I8   shaderFileWrite(char *fn, char *s);
	void  validateShader(U16 shader, const std::string &file = 0);
	void  validateProgram(U16 program);

	bool loadVertOnly(const std::string& name);
	bool loadFragOnly(const std::string& name);
};


#endif
