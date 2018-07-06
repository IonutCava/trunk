#include "Headers/ShaderManager.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Video/Shaders/Headers/Shader.h"
#include "Hardware/Video/Headers/GFXDevice.h"

ShaderManager::ShaderManager() : _nullShader(NULL)
{
}

char* ShaderManager::shaderFileRead(const std::string &atomName, const std::string& location){
	if(_atoms.find(atomName) != _atoms.end()){
		return (char*)_atoms[atomName];
	}
	if(location.empty()){
		return NULL;
	}

	std::string file = location+"/"+atomName;
	FILE *fp = NULL;
	fopen_s(&fp,file.c_str(),"r");

	char *content = NULL;


	if (fp != NULL) {
      
      fseek(fp, 0, SEEK_END);
      I32 count = ftell(fp);
      rewind(fp);

			if (count > 0) {
				content = New char[count+1];
				count = fread(content,sizeof(char),count,fp);
				content[count] = '\0';
			}
			fclose(fp);
		_atoms.insert(std::make_pair(atomName,content));
	}
	
	return content;
}

I8  ShaderManager::shaderFileWrite(char *atomName, char *s){
	FILE *fp = NULL;
	I8 status = 0;

	if (atomName != NULL) {
		fopen_s(&fp,atomName,"w");

		if (fp != NULL) {
			
			if (fwrite(s,sizeof(char),strlen(s),fp) == strlen(s))
				status = 1;
			fclose(fp);
		}
	}
	return(status);
}

void ShaderManager::removeShader(Shader* s){
	std::string name = s->getName();
	if(_shaders.find(name) != _shaders.end()){
		if(s->SubRef()){
			SAFE_DELETE(_shaders[name]);
			_shaders.erase(name);
		}
	}
}

Shader* ShaderManager::findShader(const std::string& name){
	if(_shaders.find(name) != _shaders.end()){
		_shaders[name]->AddRef();
		D_PRINT_FN(Locale::get("SHADER_MANAGER_GET_SHADER_INC"),name.c_str(),_shaders[name]->getRefCount());
		return _shaders[name];
	}
	return NULL;
}

Shader* ShaderManager::loadShader(const std::string& name, const std::string& source, ShaderType type){
	Shader* s = findShader(name);
	if(s != NULL){
		return s;
	}
	s = GFX_DEVICE.newShader(name, type);
	if(!s->load(source)){
		SAFE_DELETE(s);
	}else{
		_shaders.insert(std::make_pair(name,s));
	}
	return s;
}

bool ShaderManager::unbind(){
	if(!_nullShader){
		_nullShader = static_cast<ShaderProgram* >(FindResource("NULL_SHADER"));
		if(!_nullShader){
			_nullShader = CreateResource<ShaderProgram>(ResourceDescriptor("NULL_SHADER"));
			///the null shader should never be NULL!!!!
			assert(_nullShader != NULL); //LoL -Ionut
		}
	}
	//Should use program 0 and set previous shader ID to 0 as well
	_nullShader->bind();
	return true;
}