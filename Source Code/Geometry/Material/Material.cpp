#include "Material.h"
#include "Hardware/Video/Texture.h"
#include "Managers/ResourceManager.h"
#include "Hardware/Video/ShaderHandler.h"

Material::Material() : _shader(NULL) {
	ambient = vec4(1.0f,1.0f,1.0f,1.0f)/6;
	diffuse = vec4(1.0f,1.0f,1.0f,1.0f);
	specular = vec4(1.0f,1.0f,1.0f,1.0f);
	shininess = 50.0f;
	_textures[TEXTURE_BASE] = NULL;
	_textures[TEXTURE_BUMP] = NULL;
	_textures[TEXTURE_SECOND] = NULL;
  }

Material::Material(const Material& old){
		for(textureMap::const_iterator iter = old._textures.begin(); iter != old._textures.end(); iter++){
			_textures[iter->first] = ResourceManager::getInstance().LoadResource<Texture2D>(iter->second->getName(),true);
		}
		_shader = ResourceManager::getInstance().LoadResource<Shader>(old._shader->getName());
		diffuse = old.diffuse;
		ambient = old.ambient;
		specular = old.specular;
		emmissive = old.emmissive;
		shininess = old.shininess;
  }

Material& Material::operator=(const Material& old){
	  if (this != &old){ // protect against invalid self-assignment
		  for(textureMap::iterator iter = _textures.begin(); iter != _textures.end(); iter++){
			  ResourceManager::getInstance().remove(iter->second);
		  }
		  _textures.clear();
		  if(_shader != NULL) ResourceManager::getInstance().remove(_shader);
  		  for(textureMap::const_iterator iter = old._textures.begin(); iter != old._textures.end(); iter++){
			_textures[iter->first] = ResourceManager::getInstance().LoadResource<Texture2D>(iter->second->getName(),true);
		  }
		  _shader = ResourceManager::getInstance().LoadResource<Shader>(old._shader->getName());
		  diffuse = old.diffuse;
		  ambient = old.ambient;
		  specular = old.specular;
		  emmissive = old.emmissive;
		  shininess = old.shininess;
	  }
	  return *this;
  }

Material::~Material(){
	 clear();
}

//base = base texture
//second = second texture used for multitexturing
//bump = bump map
void Material::setTexture(TextureUsage textureUsage, Texture2D* texture) {
	if(!_textures.empty()){
		textureMap::iterator result = _textures.find(textureUsage);
		if(result != _textures.end()){
			ResourceManager::getInstance().remove(result->second);
		}
	}
	_textures[textureUsage] = texture;
}

void Material::setShader(Shader* shader){
	if(_shader != NULL)
		ResourceManager::getInstance().remove(_shader);
	_shader = shader;
}

Texture2D* const  Material::getTexture(TextureUsage textureUsage)  {
	textureMap::iterator result = _textures.find(textureUsage); 
	if(result != _textures.end())
		return result->second;
	else return NULL;
}

void Material::clear(){
	ResourceManager::getInstance().remove(_shader);
	 for(textureMap::iterator iter = _textures.begin(); iter != _textures.end(); iter++){
		  ResourceManager::getInstance().remove(iter->second);
	  }
	 _textures.clear();
}