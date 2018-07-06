#include "Headers/Material.h"
#include "Hardware/Video/Texture.h"
#include "Managers/Headers/ResourceManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Hardware/Video/GFXDevice.h"
#include "Utility/Headers/XMLParser.h"

Material::Material() : Resource(),
					   _ambient(vec4(vec3(1.0f,1.0f,1.0f)/1.5f,1)),
					   _diffuse(vec4(vec3(1.0f,1.0f,1.0f)/1.5f,1)),
					   _specular(1.0f,1.0f,1.0f,1.0f),
					   _emissive(0.6f,0.6f,0.6f),
					   _shininess(5),
					   _materialMatrix(_ambient,_diffuse,_specular,vec4(_shininess,_emissive.x,_emissive.y,_emissive.z)),
					   _computedLightShaders(false),
					   _dirty(false),
					   _twoSided(false),
					   _castsShadows(true),
					   _receiveShadows(true),
					   _state(New RenderState()),
					   _shaderRef(NULL)
{
   _textures[TEXTURE_BASE] = NULL;
   _textures[TEXTURE_BUMP] = NULL;
   _textures[TEXTURE_SECOND] = NULL;
   _textures[TEXTURE_OPACITY] = NULL;
   _textures[TEXTURE_SPECULAR] = NULL;
   _matId.i = 0;
}



Material::~Material(){
	delete _state;
}

void Material::removeCopy(){
	decRefCount();
	for_each(textureMap::value_type& iter , _textures){
		if(iter.second){
			Console::getInstance().printfn("Removing texture [ %s ] new ref count: %d",iter.second->getName().c_str(),iter.second->getRefCount());
			iter.second->removeCopy();
		}
	}
}
void Material::createCopy(){
	//increment all dependencies;
	incRefCount();
	for_each(textureMap::value_type& iter , _textures){
		if(iter.second){
			Console::getInstance().printfn("Adding texture [ %s ] new ref count: %d",iter.second->getName().c_str(),iter.second->getRefCount());
			iter.second->createCopy();
		}
	}
}

//base = base texture
//second = second texture used for multitexturing
//bump = bump map
void Material::setTexture(TextureUsage textureUsage, Texture2D* const texture) {
	if(_textures[textureUsage]){
		RemoveResource(_textures[textureUsage]);
	}else{
		//if we add a new type of texture
		_computedLightShaders = false; //recompute shaders on texture change
	}
	_textures[textureUsage] = texture;

	_dirty = true;
}

//Here we set the shader's name
ShaderProgram* Material::setShaderProgram(const std::string& shader){
	//if we already had a shader assigned ...
	if(!_shader.empty()){
		//and we are trying to assing the same one again, return.
		if(_shader.compare(shader) == 0){
			_shaderRef = static_cast<ShaderProgram* >(ResourceManager::getInstance().find(_shader));
			return _shaderRef;
		}else{
			Console::getInstance().printfn("Replacing shader [ %s ] with shader  [ %s ]",_shader.c_str(),shader.c_str());
		}
	}

	(!shader.empty()) ? _shader = shader : _shader = "NULL";

	_shaderRef = static_cast<ShaderProgram* >(ResourceManager::getInstance().find(_shader));
	if(!_shaderRef){
		_shaderRef = ResourceManager::getInstance().loadResource<ShaderProgram>(ResourceDescriptor(_shader));
	}
	_dirty = true;
	_computedLightShaders = true;
	return _shaderRef;
}

ShaderProgram* const Material::getShaderProgram(){
	return _shaderRef;
}

Texture2D* const  Material::getTexture(TextureUsage textureUsage)  {
	return _textures[textureUsage];
}

void Material::computeLightShaders(){
	//If the current material doesn't have a shader associated with it, then add the default ones.
	//Manually setting a shader, overrides this function by setting _computedLightShaders to "true"
	if(_computedLightShaders) return;
	if(_shader.empty()){
		//if(GFXDevice::getInstance().getRenderStage() == DEFERRED_STAGE){
		if(GFXDevice::getInstance().getDeferredRendering()){
			if(_textures[TEXTURE_BASE]){
				setShaderProgram("DeferredShadingPass1.Texture");
			}else{
				setShaderProgram("DeferredShadingPass1.NoTexture");
			}
		}else{
			if(_textures[TEXTURE_BASE]){
				if(_textures[TEXTURE_BUMP]){
					setShaderProgram("lighting.Bump");
				}else{
					setShaderProgram("lighting.Texture");
				}
			}else{
				setShaderProgram("lighting.NoTexture");
			}
		}
	}
}

bool Material::unload(){
	for_each(textureMap::value_type& iter , _textures){
		if(iter.second){
			RemoveResource(iter.second);
		}
	}
	_textures.clear();
	return true;
}

void Material::dumpToXML(){
	if(getName().compare("defaultMaterial") == 0) return;
	XML::dumpMaterial(this);
}

void Material::setTwoSided(bool state) {
	state ? _state->cullingEnabled() = false : _state->cullingEnabled() = true;
	_twoSided = state;
	_dirty = true;
}

bool Material::isTranslucent(){
	bool state = false;
	if(_textures[TEXTURE_BASE]){
		if(_textures[TEXTURE_BASE]->hasTransparency()) state = true;
	}
	if(!_state->cullingEnabled() && _state->blendingEnabled()) state = true;
	if(_textures[TEXTURE_OPACITY]) state = true;
	return state;
}

P32 Material::getMaterialId(){
	if(_dirty){
		(_shaderRef != NULL) ? _matId.i = _shaderRef->getId() :  _matId.i = 0;
		dumpToXML();
		_dirty = false;
	}
	return _matId;
}