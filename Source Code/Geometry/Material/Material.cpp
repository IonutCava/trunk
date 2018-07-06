#include "Material.h"
#include "Hardware/Video/Texture.h"
#include "Managers/ResourceManager.h"
#include "Hardware/Video/GFXDevice.h"
#include "Utility/Headers/XMLParser.h"

Material::Material() : Resource(),
					   _ambient(vec4(vec3(1.0f,1.0f,1.0f)/1.5f,1)),
					   _diffuse(vec4(vec3(1.0f,1.0f,1.0f)/1.5f,1)),
					   _specular(1.0f,1.0f,1.0f,1.0f),
					   _emissive(0.0f,0.0f,0.0f),
					   _shininess(50.0f),
					   _materialMatrix(_ambient,_diffuse,_specular,vec4(_shininess,_emissive.x,_emissive.y,_emissive.z)),
					   _shader(NULL),
					   _computedLightShaders(false),
					   _dirty(false)
{
   _textures[TEXTURE_BASE] = NULL;
   _textures[TEXTURE_BUMP] = NULL;
   _textures[TEXTURE_SECOND] = NULL;
}



Material::~Material(){
}

void Material::removeCopy(){
	decRefCount();
	if(_shader){
		_shader->removeCopy();
		Console::getInstance().printfn("Shader [ %s ] new ref count: %d",_shader->getName().c_str(),_shader->getRefCount());
	}
	foreach(textureMap::value_type iter , _textures){
		if(iter.second){
			iter.second->removeCopy();
		}
	}
}
void Material::createCopy(){
	//increment all dependencies;
	incRefCount();
	if(_shader){
		_shader->createCopy();
		Console::getInstance().printfn("Shader [ %s ] new ref count: %d",_shader->getName().c_str(),_shader->getRefCount());
	}

	foreach(textureMap::value_type iter , _textures){
		if(iter.second){
			iter.second->createCopy();
		}
	}
}

//base = base texture
//second = second texture used for multitexturing
//bump = bump map
void Material::setTexture(TextureUsage textureUsage, Texture2D* texture) {
	if(_textures[textureUsage])
			RemoveResource(_textures[textureUsage]);
	_textures[textureUsage] = texture;
	_dirty = true;
}

void Material::setShader(const std::string& shader){
	if(!shader.empty()){
		if(_shader){
			if(_shader->getName().compare(shader) == 0) return;
			RemoveResource(_shader);
		}
		ResourceDescriptor shaderDescriptor(shader);
		_shader = ResourceManager::getInstance().loadResource<Shader>(shaderDescriptor);
	}
	_dirty = true;
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
				setShader("DeferredShadingPass1");
			}else{
				setShader("DeferredShadingPass1_color.frag,DeferredShadingPass1.vert");
			}
		}else{
			setShader("lighting");
		}
	}
	dumpToXML();
	_computedLightShaders = true;
}

bool Material::unload(){
	if(_shader){
		RemoveResource(_shader);
	}
	
	foreach(textureMap::value_type iter , _textures){
		if(iter.second){
			RemoveResource(iter.second);
		}
	}
	_textures.clear();
	return true;
}

void Material::dumpToXML(){
	XML::dumpMaterial(this);
	_dirty = false;
}