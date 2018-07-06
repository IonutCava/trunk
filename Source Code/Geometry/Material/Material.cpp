#include "Headers/Material.h"
#include "Managers/Headers/ResourceManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Hardware/Video/Texture.h"
#include "Hardware/Video/GFXDevice.h"
#include "Hardware/Video/RenderStateBlock.h"
#include "Utility/Headers/XMLParser.h"



Material::Material() : Resource(),
					   _ambient(vec4<F32>(vec3<F32>(1.0f,1.0f,1.0f)/1.5f,1)),
					   _diffuse(vec4<F32>(vec3<F32>(1.0f,1.0f,1.0f)/1.5f,1)),
					   _specular(1.0f,1.0f,1.0f,1.0f),
					   _emissive(0.6f,0.6f,0.6f),
					   _shininess(5),
					   _opacity(1.0f),
					   _materialMatrix(_ambient,_diffuse,_specular,vec4<F32>(_shininess,_emissive.x,_emissive.y,_emissive.z)),
					   _computedLightShaders(false),
					   _dirty(false),
					   _doubleSided(false),
					   _castsShadows(true),
					   _receiveShadows(true),
					   _shaderProgramChanged(false),
					   _hardwareSkinning(false),
					   _shadingMode(SHADING_PHONG), /// phong shading by default
					   _shaderRef(NULL)
{
	_textureOperationTable[TextureOperation_Replace]   = 0;
	_textureOperationTable[TextureOperation_Multiply]  = 1;
	_textureOperationTable[TextureOperation_Decal]     = 2;
	_textureOperationTable[TextureOperation_Blend]     = 3;
	_textureOperationTable[TextureOperation_Add]       = 4;
	_textureOperationTable[TextureOperation_SmoothAdd] = 5;
	_textureOperationTable[TextureOperation_SignedAdd] = 6;
	_textureOperationTable[TextureOperation_Divide]    = 7;
	_textureOperationTable[TextureOperation_Subtract]  = 8;
	_textureOperationTable[TextureOperation_Combine]   = 9;

   _textures[TEXTURE_BASE] = NULL;
   _textures[TEXTURE_BUMP] = NULL;
   _textures[TEXTURE_SECOND] = NULL;
   _textures[TEXTURE_OPACITY] = NULL;
   _textures[TEXTURE_SPECULAR] = NULL;
   _matId.i = 0;
   /// Normal state for final rendering
   RenderStateBlockDescriptor normalStateDescriptor;
   if(GFX_DEVICE.getDeferredRendering()){
	   normalStateDescriptor._fixedLighting = false;
   }

   /// A descriptor used for rendering to shadow depth map
   RenderStateBlockDescriptor shadowDescriptor;
   shadowDescriptor.setCullMode(CULL_MODE_CCW);
   /// do not used fixed lighting
   shadowDescriptor._fixedLighting = false;
   /// set a polygon offset
   //shadowDescriptor._zBias = -0.0002f;
   /// ignore colors
   shadowDescriptor.setColorWrites(false,false,false,false);

   /// the reflection descriptor is the same as the normal descriptor, but with special culling
   RenderStateBlockDescriptor reflectionStateDescriptor;
   reflectionStateDescriptor.fromDescriptor(normalStateDescriptor);

   _defaultRenderStates.insert(std::make_pair(FINAL_STAGE, GFX_DEVICE.createStateBlock(normalStateDescriptor)));
   _defaultRenderStates.insert(std::make_pair(SHADOW_STAGE, GFX_DEVICE.createStateBlock(shadowDescriptor)));
   _defaultRenderStates.insert(std::make_pair(REFLECTION_STAGE, GFX_DEVICE.createStateBlock(reflectionStateDescriptor)));

    assert(_defaultRenderStates[FINAL_STAGE] != NULL);
	assert(_defaultRenderStates[SHADOW_STAGE] != NULL);
	assert(_defaultRenderStates[REFLECTION_STAGE] != NULL);
}



Material::~Material(){

	SAFE_DELETE(_defaultRenderStates[FINAL_STAGE]);
	SAFE_DELETE(_defaultRenderStates[SHADOW_STAGE]);
	SAFE_DELETE(_defaultRenderStates[REFLECTION_STAGE]);
	_defaultRenderStates.clear();
}

RenderStateBlock* Material::setRenderStateBlock(const RenderStateBlockDescriptor& descriptor,RENDER_STAGE renderStage){
	if(descriptor.getHash() == _defaultRenderStates[renderStage]->getDescriptor().getHash()){
		return _defaultRenderStates[renderStage];
	}

	SAFE_DELETE(_defaultRenderStates[renderStage]);

	_defaultRenderStates[renderStage] = GFX_DEVICE.createStateBlock(descriptor);
	return _defaultRenderStates[renderStage];
}

void Material::removeCopy(){
	decRefCount();
	for_each(textureMap::value_type& iter , _textures){
		if(iter.second){
			PRINT_FN("Removing texture [ %s ] new ref count: %d",iter.second->getName().c_str(),iter.second->getRefCount());
			iter.second->removeCopy();
		}
	}
}
void Material::createCopy(){
	//increment all dependencies;
	incRefCount();
	for_each(textureMap::value_type& iter , _textures){
		if(iter.second){
			PRINT_FN("Adding texture [ %s ] new ref count: %d",iter.second->getName().c_str(),iter.second->getRefCount());
			iter.second->createCopy();
		}
	}
}

//base = base texture
//second = second texture used for multitexturing
//bump = bump map
void Material::setTexture(TextureUsage textureUsage, Texture2D* const texture, TextureOperation op) {
	boost::unique_lock< boost::mutex > lock_access_here(_materialMutex);
	if(_textures[textureUsage]){
		RemoveResource(_textures[textureUsage]);
	}else{
		//if we add a new type of texture
		_computedLightShaders = false; //recompute shaders on texture change
	}
	_textures[textureUsage] = texture;
	_operations[textureUsage] = op;

	_dirty = true;
}

Material::TextureOperation Material::getTextureOperation(U32 op){
	TextureOperation operation = TextureOperation_Replace;
	switch(op){
		default:
		case 0:
			operation = TextureOperation_Replace;
			break;
		case 1:
			operation = TextureOperation_Multiply;
			break;
		case 2:
			operation = TextureOperation_Decal;
			break;
		case 3:
			operation = TextureOperation_Blend;
			break;
		case 4:
			operation = TextureOperation_Add;
			break;
		case 5:
			operation = TextureOperation_SmoothAdd;
			break;
		case 6:
			operation = TextureOperation_SignedAdd;
			break;
		case 7:
			operation = TextureOperation_Divide;
			break;
		case 8:
			operation = TextureOperation_Subtract;
			break;
		case 9:
			operation = TextureOperation_Combine;
			break;
	};
	return operation;
}


//Here we set the shader's name
ShaderProgram* Material::setShaderProgram(const std::string& shader){
	//if we already had a shader assigned ...
	if(!_shader.empty()){
		//and we are trying to assing the same one again, return.
		if(_shader.compare(shader) == 0){
			_shaderRef = static_cast<ShaderProgram* >(FindResource(_shader));
			return _shaderRef;
		}else{
			PRINT_FN("Replacing shader [ %s ] with shader  [ %s ]",_shader.c_str(),shader.c_str());
		}
	}

	(!shader.empty()) ? _shader = shader : _shader = "NULL";

	_shaderRef = static_cast<ShaderProgram* >(FindResource(_shader));
	if(!_shaderRef){
		ResourceDescriptor shaderDescriptor(_shader);
		_shaderRef = CreateResource<ShaderProgram>(shaderDescriptor);
	}
	_dirty = true;
	_computedLightShaders = true;
	_shaderProgramChanged = true;
	return _shaderRef;
}

ShaderProgram* const Material::getShaderProgram(){
	return _shaderRef;
}

Texture2D* const  Material::getTexture(TextureUsage textureUsage)  {
	boost::mutex::scoped_lock(_materialMutex);
	return _textures[textureUsage];
}

///If the current material doesn't have a shader associated with it, then add the default ones.
///Manually setting a shader, overrides this function by setting _computedLightShaders to "true"
void Material::computeLightShaders(){
	if(_computedLightShaders) return;
	if(_shader.empty()){
		///the base shader is either for a Forward Renderer ...
		std::string shader = "lighting";
		if(GFX_DEVICE.getDeferredRendering()){
			///... or a Deferred one
			shader = "DeferredShadingPass1";
		}
		///What kind of effects do we need?
		if(_textures[TEXTURE_BASE]){
			///Bump mapping?
			if(_textures[TEXTURE_BUMP]){
				shader += ".Bump";
			}else{
				/// Or simple texture mapping?
				shader += ".Texture";
			}
		}else{
			/// Or just color mapping?
			shader += ".NoTexture";
		}

		if(_hardwareSkinning){
			///Add the GPU skinnig module to the vertex shader?
			shader += ",WithBones"; ///<"," as opposed to "." sets the primary vertex shader property
		}

		setShaderProgram(shader);
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
	//if(getName().compare("defaultMaterial") == 0) return;
	XML::dumpMaterial(this);
}

void Material::setDoubleSided(bool state) {
	if(_doubleSided == state) return;
	_doubleSided = state;
	/// Update all render states for this item
	if(_doubleSided){
		typedef unordered_map<RENDER_STAGE, RenderStateBlock* >::value_type stateValue;
		for_each(stateValue& it, _defaultRenderStates){
			RenderStateBlockDescriptor desc =  it.second->getDescriptor();
				if(desc._cullMode != CULL_MODE_None){
					desc.setCullMode(CULL_MODE_None);
					setRenderStateBlock(desc,it.first);
				}
		}
	}

	_dirty = true;
}

bool Material::isTranslucent(){
	bool state = false;
	/// base texture is translucent
	if(_textures[TEXTURE_BASE]){
		if(_textures[TEXTURE_BASE]->hasTransparency()) state = true;
	}
	/// opacity map
	if(_textures[TEXTURE_OPACITY]) state = true;
	/// diffuse channel alpha
	if(getMaterialMatrix().getCol(1).a < 1.0f) state = true;

	/// if we have a global opacity value
	if(getOpacityValue() < 1.0f){
		state = true;
	}
	/// Disable culling for translucent items
	if(state){
		typedef unordered_map<RENDER_STAGE, RenderStateBlock* >::value_type stateValue;
		for_each(stateValue& it, _defaultRenderStates){
			RenderStateBlockDescriptor desc =  it.second->getDescriptor();
			if(desc._cullMode != CULL_MODE_None){
				desc.setCullMode(CULL_MODE_None);
				setRenderStateBlock(desc,it.first);
			}
		}
	}
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