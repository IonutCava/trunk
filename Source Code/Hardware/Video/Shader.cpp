#include "Shader.h"

Shader::Shader(const std::string& name, SHADER_TYPE type){
  _name = name;
  _type = type;
  _compiled = false;
  _refCount = 1;
}

Shader::~Shader(){
	Console::getInstance().d_printfn("Deleting Shader  [ %s ]",getName().c_str());
}
