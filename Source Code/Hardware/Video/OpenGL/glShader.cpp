#include "glShader.h"
#include "glResources.h"

glShader::glShader(const std::string& name, SHADER_TYPE type) : Shader(name, type){
  switch (type) {
    case VERTEX_SHADER : { 
		_shader = glCreateShader(GL_VERTEX_SHADER);
		break;
	}
    case FRAGMENT_SHADER : {
		_shader = glCreateShader(GL_FRAGMENT_SHADER);
		break;
	}

    case GEOMETRY_SHADER : {
		_shader = glCreateShader(GL_GEOMETRY_SHADER);
		break;
	}
	case TESSELATION_SHADER : {
		_shader = NULL; 
		Console::getInstance().errorfn("GLSL: Tesselation not yet implemented"); 
		break;
	}
	default:
		Console::getInstance().errorfn("GLSL: Unknown shader type received: %d",type);
		break;
  }
}

glShader::~glShader(){
	glDeleteShader(_shader);
}

bool glShader::load(const std::string& name){
	const char* shaderText = shaderFileRead(name);
	if(shaderText == NULL){
		Console::getInstance().errorfn("GLSL Manager: Shader [ %s ] not found!",name.c_str());
		return false;
	}
	glShaderSource(_shader, 1, &shaderText, 0);
	glCompileShader(_shader);
	_compiled = true;
	validate();
	delete shaderText;
	shaderText = NULL;
	return true;
}

void glShader::validate() {
	const U16 BUFFER_SIZE = 512;
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	I32 length = 0;
    I32 status = 0;
	glGetShaderInfoLog(_shader, BUFFER_SIZE, &length, buffer);
	glGetShaderiv(_shader, GL_COMPILE_STATUS, &status);
	if(status == GL_FALSE){
		Console::getInstance().errorfn("[GLSL Manager] Validating shader [ %s ]: %s", _name.c_str(),buffer);
	}else{
		Console::getInstance().d_printfn("[GLSL Manager] Validating shader [ %s ]: %s", _name.c_str(),buffer);
	}
}