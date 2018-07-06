#include "Material.h"
#include "Hardware/Video/Texture.h"
#include "Managers/ResourceManager.h"
#include "Hardware/Video/GFXDevice.h"

Material::Material() : Resource(),
						_ambient(vec4(1.0f,1.0f,1.0f,1.0f)/1.5f),
					   _diffuse(vec4(1.0f,1.0f,1.0f,1.0f)/1.5f),
					   _specular(1.0f,1.0f,1.0f,1.0f),
					   _emmissive(0.0f,0.0f,0.0f,0.0f),
					   _shininess(50.0f),
					   _materialMatrix(_ambient,_diffuse,_specular,vec4(_shininess,_emmissive.x,_emmissive.y,_emmissive.z)),
					   _shader(NULL),
					   _computedLightShaders(false)
{
   _textures[TEXTURE_BASE] = NULL;
   _textures[TEXTURE_BUMP] = NULL;
   _textures[TEXTURE_SECOND] = NULL;
}


Material::Material(const Material& old) : Resource(old){
		for(textureMap::const_iterator iter = old._textures.begin(); iter != old._textures.end(); iter++){
			ResourceDescriptor curentTexture(iter->second->getName());
			curentTexture.setResourceLocation(iter->second->getResourceLocation());
			curentTexture.setFlag(true);
			_textures[iter->first] = ResourceManager::getInstance().LoadResource<Texture2D>(curentTexture);
		}
		ResourceDescriptor shader(old._shader->getName());
		_shader = ResourceManager::getInstance().LoadResource<Shader>(shader);
		_diffuse = old._diffuse;
		_ambient = old._ambient;
		_specular = old._specular;
		_emmissive = old._emmissive;
		_shininess = old._shininess;
		_materialMatrix = old._materialMatrix;
		_computedLightShaders = old._computedLightShaders;
  }

Material& Material::operator=(const Material& old){
	  if (this != &old){ // protect against invalid self-assignment
		  for(textureMap::iterator iter = _textures.begin(); iter != _textures.end(); iter++){
			  ResourceManager::getInstance().remove(iter->second);
		  }
		  _textures.clear();
		  if(_shader != NULL) ResourceManager::getInstance().remove(_shader);
  		  for(textureMap::const_iterator iter = old._textures.begin(); iter != old._textures.end(); iter++){
			ResourceDescriptor curentTexture(iter->second->getName());
			curentTexture.setResourceLocation(iter->second->getResourceLocation());
			curentTexture.setFlag(true);
			_textures[iter->first] = ResourceManager::getInstance().LoadResource<Texture2D>(curentTexture);
		  }
		  ResourceDescriptor shader(old._shader->getName());
		  _shader = ResourceManager::getInstance().LoadResource<Shader>(shader);
		  _diffuse = old._diffuse;
		  _ambient = old._ambient;
		  _specular = old._specular;
		  _emmissive = old._emmissive;
		  _shininess = old._shininess;
		  _materialMatrix = old._materialMatrix;
		  _computedLightShaders = old._computedLightShaders;
	 }
	  return *this;
  }

Material::~Material(){
}

//base = base texture
//second = second texture used for multitexturing
//bump = bump map
void Material::setTexture(TextureUsage textureUsage, Texture2D* texture) {
	if(_textures[textureUsage])
			ResourceManager::getInstance().remove(_textures[textureUsage]);
	_textures[textureUsage] = texture;
}

void Material::setShader(const std::string& shader){
	if(_shader){
		if(_shader->getName().compare(shader) == 0) return;
		ResourceManager::getInstance().remove(_shader);
	}
	ResourceDescriptor shaderDescriptor(shader);
	_shader = ResourceManager::getInstance().LoadResource<Shader>(shaderDescriptor);
	_computedLightShaders = true;
}

Texture2D* const  Material::getTexture(TextureUsage textureUsage)  {
	return _textures[textureUsage];
}

void Material::computeLightShaders(){
	//If the current material doesn't have a shader associated with it, then add the default ones.
	//Manually setting a shader, overrides this function by setting _computedLightShaders to "true"
	if(_computedLightShaders) return;
	if(_shader == NULL){
		if(GFXDevice::getInstance().getDeferredShading()){
			if(_textures[TEXTURE_BASE]){
				ResourceDescriptor deferred("DeferredShadingPass1");
				_shader = ResourceManager::getInstance().LoadResource<Shader>(deferred);
			}else{
				ResourceDescriptor deferred("DeferredShadingPass1_color.frag,DeferredShadingPass1.vert");
				_shader = ResourceManager::getInstance().LoadResource<Shader>(deferred);
			}
		}else{
			ResourceDescriptor lighting("lighting");
			//vector<Light_ptr>& lights = SceneManager::getInstance().getActiveScene()->getLights();
			//for(U8 i = 0; i < lights.size(); i++)
			Console::getInstance().printfn("Adding default lighting shader to material [ %s ]", getName().c_str());
			_shader = ResourceManager::getInstance().LoadResource<Shader>(lighting);
		}
	}
	_computedLightShaders = true;
}

void Material::skipComputeLightShaders(){
	_computedLightShaders = true;
}

bool Material::unload(){
	if(_shader){
		ResourceManager::getInstance().remove(_shader);
	}
	for(textureMap::iterator iter = _textures.begin(); iter != _textures.end(); iter++){
		if(iter->second){
			ResourceManager::getInstance().remove(iter->second);
		}
	}
	_textures.clear();

	return true;
}