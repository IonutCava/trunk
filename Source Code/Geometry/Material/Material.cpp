#include "Headers/Material.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/ShaderManager.h"
#include "Hardware/Video/Textures/Headers/Texture.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"


Material::Material() : Resource(),
					   _ambient(vec4<F32>(vec3<F32>(1.0f,1.0f,1.0f)/1.5f,1)),
					   _diffuse(vec4<F32>(vec3<F32>(1.0f,1.0f,1.0f)/1.5f,1)),
					   _specular(1.0f,1.0f,1.0f,1.0f),
					   _emissive(0.6f,0.6f,0.6f),
					   _shininess(5),
					   _opacity(1.0f),
					   _materialMatrix(_ambient,_diffuse,_specular,vec4<F32>(_shininess,_emissive.x,_emissive.y,_emissive.z)),
					   _dirty(false),
					   _doubleSided(false),
					   _castsShadows(true),
					   _receiveShadows(true),
					   _shaderProgramChanged(false),
					   _hardwareSkinning(false),
					   _shadingMode(SHADING_PHONG), /// phong shading by default
					   _bumpMethod(BUMP_NONE)
{
	_bumpMethodTable[BUMP_NONE] = 0;
	_bumpMethodTable[BUMP_NORMAL] = 1;
	_bumpMethodTable[BUMP_PARALLAX] = 2;
	_bumpMethodTable[BUMP_RELIEF] = 3;

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
   shadowDescriptor._zBias = 1.0f;
   /// ignore colors
   shadowDescriptor.setColorWrites(false,false,false,true);

   /// the reflection descriptor is the same as the normal descriptor, but with special culling
   RenderStateBlockDescriptor reflectionStateDescriptor;
   reflectionStateDescriptor.fromDescriptor(normalStateDescriptor);

   _defaultRenderStates.insert(std::make_pair(FINAL_STAGE, GFX_DEVICE.createStateBlock(normalStateDescriptor)));
   _defaultRenderStates.insert(std::make_pair(SHADOW_STAGE, GFX_DEVICE.createStateBlock(shadowDescriptor)));
   _defaultRenderStates.insert(std::make_pair(REFLECTION_STAGE, GFX_DEVICE.createStateBlock(reflectionStateDescriptor)));

    assert(_defaultRenderStates[FINAL_STAGE] != NULL);
	assert(_defaultRenderStates[SHADOW_STAGE] != NULL);
	assert(_defaultRenderStates[REFLECTION_STAGE] != NULL);

	_shaderRef[FINAL_STAGE] = NULL;
	_shaderRef[SHADOW_STAGE] = NULL;
	_computedShader[0] = false;
	_computedShader[1] = false;
	_matId[0].i = 0;
    _matId[1].i = 0;
}


Material::~Material(){

	SAFE_DELETE(_defaultRenderStates[FINAL_STAGE]);
	SAFE_DELETE(_defaultRenderStates[SHADOW_STAGE]);
	SAFE_DELETE(_defaultRenderStates[REFLECTION_STAGE]);
	_defaultRenderStates.clear();
}

RenderStateBlock* Material::setRenderStateBlock(const RenderStateBlockDescriptor& descriptor,RenderStage renderStage){
	if(descriptor.getHash() == _defaultRenderStates[renderStage]->getDescriptor().getHash()){
		return _defaultRenderStates[renderStage];
	}

	SAFE_DELETE(_defaultRenderStates[renderStage]);

	_defaultRenderStates[renderStage] = GFX_DEVICE.createStateBlock(descriptor);
	return _defaultRenderStates[renderStage];
}

//base = base texture
//second = second texture used for multitexturing
//bump = bump map
void Material::setTexture(TextureUsage textureUsage, Texture2D* const texture, TextureOperation op) {
	boost::unique_lock< boost::mutex > lock_access_here(_materialMutex);
	if(_textures[textureUsage]){
		UNREGISTER_TRACKED_DEPENDENCY(_textures[textureUsage]);
		RemoveResource(_textures[textureUsage]);
	}else{
		//if we add a new type of texture
		_computedShader[0] = false; //recompute shaders on texture change
	}
	_textures[textureUsage] = texture;
	_operations[textureUsage] = op;
	REGISTER_TRACKED_DEPENDENCY(_textures[textureUsage]);
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
ShaderProgram* Material::setShaderProgram(const std::string& shader,RenderStage renderStage){
	ShaderProgram* shaderReference = _shaderRef[renderStage];
	U8 id = (renderStage == FINAL_STAGE ? 0 : 1);
	//if we already had a shader assigned ...
	if(!_shader[id].empty()){
		//and we are trying to assing the same one again, return.
		if(_shader[id].compare(shader) == 0){
			shaderReference = static_cast<ShaderProgram* >(FindResource(_shader[id]));
			return shaderReference;
		}else{
			PRINT_FN(Locale::get("REPLACE_SHADER"),_shader[id].c_str(),shader.c_str());
		}
	}

	(!shader.empty()) ? _shader[id] = shader : _shader[id] = "NULL_SHADER";

	shaderReference = static_cast<ShaderProgram* >(FindResource(_shader[id]));
	if(!shaderReference){
		ResourceDescriptor shaderDescriptor(_shader[id]);
		std::stringstream ss;
		ss << "USE_VBO_DATA,";
		if(!_shaderDefines.empty()){
			for_each(std::string& shaderDefine, _shaderDefines){
				ss << shaderDefine;
				ss << ",";
			}
		}
		ss << "DEFINE_PLACEHOLDER";
		shaderDescriptor.setPropertyList(ss.str());
		shaderReference = CreateResource<ShaderProgram>(shaderDescriptor);
	}
	_dirty = true;
	_computedShader[id] = true;
	if(id == 0) { _shaderProgramChanged = true; }
	_shaderRef[renderStage] = shaderReference; 
	return shaderReference;
}

///If the current material doesn't have a shader associated with it, then add the default ones.
///Manually setting a shader, overrides this function by setting _computedShaders to "true"
void Material::computeShader(bool force, RenderStage renderStage){
	U8 id = (renderStage == FINAL_STAGE ? 0 : 1);
	if(_computedShader[id] && !force) return;
	if(_shader[id].empty()){
		///the base shader is either for a Forward Renderer ...
		std::string shader;
		if(renderStage == SHADOW_STAGE){
			shader = "depthPass";
		}else{
			shader = "lighting";
		}
		if(GFX_DEVICE.getDeferredRendering()){
			///... or a Deferred one
			shader = "DeferredShadingPass1";
		}
		///What kind of effects do we need?
		if(_textures[TEXTURE_BASE]){
			///Bump mapping?
			if(_textures[TEXTURE_BUMP] && _bumpMethod != BUMP_NONE){
				shader += ".Bump"; //Normal Mapping
				if(_bumpMethod == 2){ //Parallax Mapping
					shader += ".Parallax";
				}else if(_bumpMethod == 3){ //Relief Mapping
					shader += ".Relief";
				}
			}else{
				/// Or simple texture mapping?
				shader += ".Texture";
			}
		}else{
			/// Or just color mapping? Use the simple fragment
		}

		if(_hardwareSkinning){
			///Add the GPU skinnig module to the vertex shader?
			shader += ",WithBones"; ///<"," as opposed to "." sets the primary vertex shader property
		}
		///Add any modifiers you wish
		if(!_shaderModifier.empty()){
			shader += ".";
			shader += _shaderModifier;
		}

		setShaderProgram(shader,renderStage);
	}
}

void Material::setCastsShadows(bool state) {
	if(_castsShadows != state){
		if(!state) _shader[1] = "NULL_SHADER";
		else _shader[1].clear();
		_castsShadows = state;
		_computedShader[1] = false;
		_dirty = true;
	}
}

void Material::setBumpMethod(U32 newBumpMethod,bool force){
	if(newBumpMethod == 0){
		_bumpMethod = BUMP_NONE;
	}else if(newBumpMethod == 1){
		_bumpMethod = BUMP_NORMAL;
	}else if(newBumpMethod == 2){
		_bumpMethod = BUMP_PARALLAX;
	}else{
		_bumpMethod = BUMP_RELIEF;
	}
	if(force){
		_shader[FINAL_STAGE].clear();
		computeShader(true);
	}
}

void Material::setBumpMethod(BumpMethod newBumpMethod,bool force){
	_bumpMethod = newBumpMethod;
	if(force){
		_shader[FINAL_STAGE].clear();
		computeShader(true);
	}
}

void Material::addShaderModifier(const std::string& shaderModifier,bool force) {
	_shaderModifier = shaderModifier;
	if(force){
		_shader[FINAL_STAGE].clear();
		computeShader(true);
	}
}

void Material::addShaderDefines(const std::string& shaderDefines,bool force) {
	_shaderDefines.push_back(shaderDefines);
	if(force){
		_shader[FINAL_STAGE].clear();
		_shader[SHADOW_STAGE].clear();
		computeShader(true);
		computeShader(true,SHADOW_STAGE);
	}
}

bool Material::unload(){
	for_each(textureMap::value_type& iter , _textures){
		if(iter.second){
			UNREGISTER_TRACKED_DEPENDENCY(iter.second);
			RemoveResource(iter.second);
		}
	}
	_textures.clear();
	return true;
}

void Material::setDoubleSided(bool state) {
	if(_doubleSided == state) return;
	_doubleSided = state;
	/// Update all render states for this item
	if(_doubleSided){
		typedef Unordered_map<RenderStage, RenderStateBlock* >::value_type stateValue;
		for_each(stateValue& it, _defaultRenderStates){
			RenderStateBlockDescriptor desc =  it.second->getDescriptor();
				if(desc._cullMode != CULL_MODE_NONE){
					desc.setCullMode(CULL_MODE_NONE);
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
		typedef Unordered_map<RenderStage, RenderStateBlock* >::value_type stateValue;
		for_each(stateValue& it, _defaultRenderStates){
			RenderStateBlockDescriptor desc =  it.second->getDescriptor();
			if(desc._cullMode != CULL_MODE_NONE){
				desc.setCullMode(CULL_MODE_NONE);
				setRenderStateBlock(desc,it.first);
			}
		}
	}
	return state;
}

P32 Material::getMaterialId(RenderStage renderStage){
	
	if(_dirty){
		_matId[0].i = (_shaderRef[FINAL_STAGE] != NULL ?  _shaderRef[FINAL_STAGE]->getId() : 0);
		_matId[1].i = (_shaderRef[SHADOW_STAGE] != NULL ?  _shaderRef[SHADOW_STAGE]->getId() : 0);
		dumpToXML();
		_dirty = false;
	}
	return _matId[renderStage == FINAL_STAGE ? 0 : 1];
}