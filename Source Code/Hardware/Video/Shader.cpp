#include "Shader.h"

Shader::Shader(const std::string& name, SHADER_TYPE type){
  _name = name;
  _type = type;
  _compiled = false;
}

Shader::~Shader(){
	D_PRINT_FN("Deleting Shader  [ %s ]",getName().c_str());
}
