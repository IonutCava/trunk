#include "Material.h"
#include "Hardware/Video/Texture.h"
#include "Managers/ResourceManager.h"

Material::Material() : bumpMap(NULL) {
	ambient = vec4(1.0f,1.0f,1.0f,1.0f)/6;
	diffuse = vec4(1.0f,1.0f,1.0f,1.0f);
	specular = vec4(1.0f,1.0f,1.0f,1.0f);
	shininess = 50.0f;
  }

Material::Material(const Material& old){
		textures.reserve(old.textures.size());
		for(U8 i = 0; i < old.textures.size(); i++){
			textures.push_back(ResourceManager::getInstance().LoadResource<Texture2D>(old.textures[i]->getName(),true));
		}
		bumpMap = ResourceManager::getInstance().LoadResource<Texture2D>(old.bumpMap->getName(),true);
		diffuse = old.diffuse;
		ambient = old.ambient;
		specular = old.specular;
		emmissive = old.emmissive;
		shininess = old.shininess;
  }

Material& Material::operator=(const Material& old){
	  if (this != &old){ // protect against invalid self-assignment
		  for(U8 i = 0; i < textures.size(); i++)
			  delete textures[i];
		  textures.clear();
		  textures.reserve(old.textures.size());
		  for(U8 i = 0; i < old.textures.size(); i++){
			textures.push_back(ResourceManager::getInstance().LoadResource<Texture2D>(old.textures[i]->getName(),true));
		  }
		  delete bumpMap;
		  bumpMap =  ResourceManager::getInstance().LoadResource<Texture2D>(old.bumpMap->getName(),true);
		  diffuse = old.diffuse;
		  ambient = old.ambient;
		  specular = old.specular;
		  emmissive = old.emmissive;
		  shininess = old.shininess;
	  }
	  return *this;
  }

Material::~Material(){
	  for(U8 i = 0; i < textures.size(); i++)
			ResourceManager::getInstance().remove(textures[i]);
	  if(bumpMap)
			ResourceManager::getInstance().remove(bumpMap);
  }