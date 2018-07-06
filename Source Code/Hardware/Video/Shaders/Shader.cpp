#include "Headers/Shader.h"
#include "Headers/ShaderProgram.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

Shader::Shader(const std::string& name, const ShaderType& type,const bool optimise) : _shader(std::numeric_limits<U32>::max()), 
																					  _name(name),
																				      _type(type),
																					  _compiled(false),
																					  _optimise(optimise)
{
}

Shader::~Shader(){
	D_PRINT_FN(Locale::get("SHADER_DELETE"),getName().c_str());
	for_each(ShaderProgram* shaderPtr, _parentShaderPrograms){
		shaderPtr->detachShader(this);
	}
	_parentShaderPrograms.clear();
}

void Shader::addParentProgram(ShaderProgram* const shaderProgram){
	_parentShaderPrograms.push_back(shaderProgram);
}

void Shader::removeParentProgram(ShaderProgram* const shaderProgram){
   vectorImpl<ShaderProgram* >::iterator it;

   for ( it = _parentShaderPrograms.begin(); it != _parentShaderPrograms.end(); ){
	    if( (*it)->getId() ==  shaderProgram->getId()){
			it = _parentShaderPrograms.erase(it);
		}else{
			++it;
		}
   }
}