#ifndef GL_SHADER_H_
#define GL_SHADER_H_

#include "Hardware/Video/Shader.h"

class glShader : public Shader{
public:
	glShader(const std::string& name, SHADER_TYPE type);
	~glShader();

	bool load(const std::string& name);
	void validate();
};

#endif