#ifndef GL_SHADER_H_
#define GL_SHADER_H_

#include "Hardware/Video/Shader.h"

class glShader : public Shader{
public:
	glShader(const std::string& name, SHADER_TYPE type);
	~glShader();

	bool load(const std::string& source);
	void validate();

private:
	std::string preprocessIncludes(const std::string& source, const std::string& filename, I32 level /*= 0 */ );
};

#endif