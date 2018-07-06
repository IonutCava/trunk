#include "Headers/ShaderManager.h"
#include "Headers/ResourceManager.h"
#include "Hardware/Video/Shader.h"
#include "Hardware/Video/GFXDevice.h"

ShaderManager::ShaderManager() : _nullShader(NULL)
{
}

void ShaderManager::removeShader(Shader* s){
	std::string name = s->getName();
	if(_shaders.find(name) != _shaders.end()){
		s->decRefCount();
		if(s->getRefCount() == 0){
			delete _shaders[name];
			_shaders[name] = NULL;
			_shaders.erase(name);
		}
	}
}

Shader* ShaderManager::loadShader(const std::string& name, const std::string& location){
	if(_shaders.find(name) != _shaders.end()){
		_shaders[name]->incRefCount();
		Console::getInstance().printfn("ShaderManager: returning shader [ %s ]. New ref count [ %d ]",name.c_str(),_shaders[name]->getRefCount());
		return _shaders[name];
	}
	SHADER_TYPE type;
	std::string extension = name.substr(name.length()-4, name.length());
	if(extension.compare("frag") == 0){
		type = FRAGMENT_SHADER;
	}else if(extension.compare("vert") == 0){
		type = VERTEX_SHADER;
	}else if(extension.compare("geom") == 0){
		type = GEOMETRY_SHADER;
	}else{
		type = TESSELATION_SHADER;
	}
	Shader* s = GFXDevice::getInstance().newShader(name, type);
	if(!s->load(location + name)){
		delete s;
		s = NULL;
	}else{
		_shaders.insert(std::make_pair(name,s));
	}
	return s;
}

bool ShaderManager::unbind(){
	if(!_nullShader){
		_nullShader = static_cast<ShaderProgram* >(ResourceManager::getInstance().find("NULL"));
		if(!_nullShader){
			_nullShader = ResourceManager::getInstance().loadResource<ShaderProgram>(ResourceDescriptor("NULL"));
			assert(_nullShader != NULL); //LoL -Ionut
		}
	}
	//Should use program 0 and set previous shader ID to 0 as well
	_nullShader->bind();
	return true;
}