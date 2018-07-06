#include "Headers/Material.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/ShaderManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

Material::Material() : Resource("temp_material"),
                       _dirty(false),
                       _doubleSided(false),
                       _shaderThreadedLoad(true),
                       _hardwareSkinning(false),
                       _useAlphaTest(false),
                       _dumpToFile(true),
                       _translucencyCheck(false),
                       _shadingMode(SHADING_PHONG), /// phong shading by default
                       _bumpMethod(BUMP_NONE),
                       _translucencySource(TRANSLUCENT_NONE)
{
   _materialMatrix.resize(1, mat4<F32>());
   _shaderData.resize(1, ShaderData());

   _materialMatrix[0].setCol(0,_shaderData[0]._ambient);
   _materialMatrix[0].setCol(1,_shaderData[0]._diffuse);
   _materialMatrix[0].setCol(2,_shaderData[0]._specular);
   _materialMatrix[0].setCol(3,vec4<F32>(_shaderData[0]._shininess,
                                         _shaderData[0]._emissive.x,
                                         _shaderData[0]._emissive.y,
                                         _shaderData[0]._emissive.z));

   for(U8 i = 0; i < Config::MAX_TEXTURE_STORAGE; ++i)
       _textures[i] = nullptr;
   for(U8 i = 0; i < Config::MAX_TEXTURE_STORAGE - TEXTURE_UNIT0; ++i)
       _operations[i] = TextureOperation_Replace;
   //std::fill(_textures, _textures + Config::MAX_TEXTURE_STORAGE, static_cast<Texture*>(nullptr));
   //std::fill(_operations, _operations + (Config::MAX_TEXTURE_STORAGE - TEXTURE_UNIT0), TextureOperation_Replace);

   /// Normal state for final rendering
   RenderStateBlockDescriptor stateDescriptor;
   _defaultRenderStates.insert(std::make_pair(FINAL_STAGE, GFX_DEVICE.getOrCreateStateBlock(stateDescriptor)));
   /// the reflection descriptor is the same as the normal descriptor
   RenderStateBlockDescriptor reflectorDescriptor(stateDescriptor);
   _defaultRenderStates.insert(std::make_pair(REFLECTION_STAGE, GFX_DEVICE.getOrCreateStateBlock(reflectorDescriptor)));
   /// the z-pre-pass descriptor does not process colors
   RenderStateBlockDescriptor zPrePassDescriptor(stateDescriptor);
   zPrePassDescriptor.setColorWrites(false, false, false, false);
   _defaultRenderStates.insert(std::make_pair(Z_PRE_PASS_STAGE, GFX_DEVICE.getOrCreateStateBlock(zPrePassDescriptor)));
   /// A descriptor used for rendering to depth map
   RenderStateBlockDescriptor shadowDescriptor(stateDescriptor);
   shadowDescriptor.setCullMode(CULL_MODE_CCW);
   /// set a polygon offset
   //shadowDescriptor.setZBias(1.0f, 2.0f);
   /// ignore colors - Some shadowing techniques require drawing to the a color buffer
   shadowDescriptor.setColorWrites(true, true, false, false);
   _defaultRenderStates.insert(std::make_pair(SHADOW_STAGE, GFX_DEVICE.getOrCreateStateBlock(shadowDescriptor)));
   
   assert(_defaultRenderStates[FINAL_STAGE] != nullptr);
   assert(_defaultRenderStates[Z_PRE_PASS_STAGE] != nullptr);
   assert(_defaultRenderStates[SHADOW_STAGE] != nullptr);
   assert(_defaultRenderStates[REFLECTION_STAGE] != nullptr);

   _computedShaderTextures = false;
}

Material::~Material(){
    _defaultRenderStates.clear();
}

void Material::update(const U64 deltaTime){
    // build one shader per frame
    computeShaderInternal();
}

const RenderStateBlock& Material::getRenderStateBlock(RenderStage currentStage) {
    if (_defaultRenderStates.find(currentStage) == _defaultRenderStates.end())
        return *_defaultRenderStates[FINAL_STAGE];
    
    return *_defaultRenderStates[currentStage];
}

RenderStateBlock* Material::setRenderStateBlock(RenderStateBlockDescriptor& descriptor,const RenderStage& renderStage){
    if(descriptor.getHash() == _defaultRenderStates[renderStage]->getDescriptor().getHash()){
        return _defaultRenderStates[renderStage];
    }

    _defaultRenderStates[renderStage] = GFX_DEVICE.getOrCreateStateBlock(descriptor);
    return _defaultRenderStates[renderStage];
}

//base = base texture
//second = second texture used for multitexturing
//bump = bump map
void Material::setTexture(U32 textureUsageSlot, Texture* const texture, const TextureOperation& op, U8 index) {
    if(_textures[textureUsageSlot]){
        UNREGISTER_TRACKED_DEPENDENCY(_textures[textureUsageSlot]);
        RemoveResource(_textures[textureUsageSlot]);
        if(textureUsageSlot == TEXTURE_OPACITY || textureUsageSlot == TEXTURE_SPECULAR){
            _computedShaderTextures = false;
        }
    }else{
        //if we add a new type of texture
        _shaderInfo[FINAL_STAGE]._computedShader = false; //recompute shaders on texture change
    }
    _textures[textureUsageSlot] = texture;

    if(textureUsageSlot >= TEXTURE_UNIT0)
        _operations[textureUsageSlot - TEXTURE_UNIT0] = op;

    if(textureUsageSlot >= TEXTURE_UNIT0){
        texture ? _shaderData[index]._textureCount++ : _shaderData[index]._textureCount--;
    }

    if (textureUsageSlot == TEXTURE_OPACITY || textureUsageSlot == TEXTURE_UNIT0){
        _translucencyCheck = false;
    }
    if(texture){
        REGISTER_TRACKED_DEPENDENCY(_textures[textureUsageSlot]);
    }
    _dirty = true;
}

//Here we set the shader's name
void Material::setShaderProgram(const std::string& shader, const RenderStage& renderStage, const bool computeOnAdd){
    ShaderProgram* shaderReference = _shaderInfo[renderStage]._shaderRef;
    //if we already had a shader assigned ...
    if (!_shaderInfo[renderStage]._shader.empty()){
        //and we are trying to assing the same one again, return.
        shaderReference = FindResourceImpl<ShaderProgram>(_shaderInfo[renderStage]._shader);
        if (_shaderInfo[renderStage]._shader.compare(shader) != 0){
            PRINT_FN(Locale::get("REPLACE_SHADER"), _shaderInfo[renderStage]._shader.c_str(), shader.c_str());
            RemoveResource(shaderReference);
        }
    }

    (!shader.empty()) ? _shaderInfo[renderStage]._shader = shader : _shaderInfo[renderStage]._shader = "NULL_SHADER";
    
    ResourceDescriptor shaderDescriptor(_shaderInfo[renderStage]._shader);
    std::stringstream ss;
    if (!_shaderInfo[renderStage]._shaderDefines.empty()){
        for(std::string& shaderDefine : _shaderInfo[renderStage]._shaderDefines){
            ss << shaderDefine;
            ss << ",";
        }
    }
    ss << "DEFINE_PLACEHOLDER";
    shaderDescriptor.setPropertyList(ss.str());
    shaderDescriptor.setThreadedLoading(_shaderThreadedLoad);

    _computedShaderTextures = true;
    if (computeOnAdd){
        _shaderInfo[renderStage]._shaderRef = CreateResource<ShaderProgram>(shaderDescriptor);
        _shaderInfo[renderStage]._computedShader = true;
    }else{
        _shaderComputeQueue.push(std::make_pair(renderStage, shaderDescriptor));
    }
}

void Material::clean() {
    if(_dirty && _dumpToFile){
        isTranslucent();
        XML::dumpMaterial(*this);
       _dirty = false;
    }
}

///If the current material doesn't have a shader associated with it, then add the default ones.
///Manually setting a shader, overrides this function by setting _computedShaders to "true"
bool Material::computeShader(bool force, const RenderStage& renderStage){
    bool deferredPassShader = GFX_DEVICE.getRenderer()->getType() != RENDERER_FORWARD;
    bool depthPassShader = renderStage == SHADOW_STAGE || renderStage == Z_PRE_PASS_STAGE;
    //bool forwardPassShader = !deferredPassShader && !depthPassShader;

    if (force || ((_shaderInfo[renderStage]._shader.empty() || (renderStage == FINAL_STAGE && !_computedShaderTextures)) || !_shaderInfo[renderStage]._computedShader)) {
        //the base shader is either for a Deferred Renderer or a Forward  one ...
        std::string shader = (deferredPassShader ? "DeferredShadingPass1" : (depthPassShader ? "depthPass" : "lighting"));
  
        if (depthPassShader) renderStage == Z_PRE_PASS_STAGE ? shader += ".PrePass" :  shader += ".Shadow";
 
        //What kind of effects do we need?
        if (_textures[TEXTURE_UNIT0]){
            //Bump mapping?
            if (_textures[TEXTURE_NORMALMAP] && _bumpMethod != BUMP_NONE){
                addShaderDefines(renderStage, "COMPUTE_TBN");
                shader += ".Bump"; //Normal Mapping
                if (_bumpMethod == 2){ //Parallax Mapping
                    shader += ".Parallax";
                    addShaderDefines(renderStage, "USE_PARALLAX_MAPPING");
                }else if (_bumpMethod == 3){ //Relief Mapping
                    shader += ".Relief";
                    addShaderDefines(renderStage, "USE_RELIEF_MAPPING");
                }
            }
            else{
                // Or simple texture mapping?
                shader += ".Texture";
            }
        }

        if (isTranslucent()){
            for (Material::TranslucencySource source : _translucencySource){
                if(source == TRANSLUCENT_DIFFUSE ){
                    shader += ".DiffuseAlpha";
                    addShaderDefines(renderStage, "USE_OPACITY_DIFFUSE");
                }
                if (source == TRANSLUCENT_OPACITY){
                    shader += ".OpacityValue";
                    addShaderDefines(renderStage, "USE_OPACITY");
                }
                if (source == TRANSLUCENT_OPACITY_MAP){
                    shader += ".OpacityMap";
                    addShaderDefines(renderStage, "USE_OPACITY_MAP");
                }
                if (source == TRANSLUCENT_DIFFUSE_MAP){
                    shader += ".TextureAlpha";
                    addShaderDefines(renderStage, "USE_OPACITY_DIFFUSE_MAP");
                }
            }
        }

        if (_textures[TEXTURE_SPECULAR]){
            shader += ".Specular";
            addShaderDefines(renderStage, "USE_SPECULAR_MAP");
        }

        //Add the GPU skinning module to the vertex shader?
        if (_hardwareSkinning){
            addShaderDefines(renderStage, "USE_GPU_SKINNING");
            shader += ",Skinned"; //<Use "," instead of "." will add a Vertex only property
        }

        //Add any modifiers you wish
        if (!_shaderModifier.empty()){
            shader += ".";
            shader += _shaderModifier;
        }

        setShaderProgram(shader, renderStage);

        return true;
    }

    return false;
}

void Material::computeShaderInternal(){
    if (_shaderComputeQueue.empty())
        return;

    std::pair<RenderStage, ResourceDescriptor> currentItem = _shaderComputeQueue.front();

    _dirty = true;
    _shaderInfo[currentItem.first]._shaderRef = CreateResource<ShaderProgram>(currentItem.second);
    _shaderInfo[currentItem.first]._computedShader = true;

    _shaderComputeQueue.pop();
}

ShaderProgram* const Material::getShaderProgram(RenderStage renderStage) {
    ShaderProgram* shaderPtr = _shaderInfo[renderStage]._shaderRef;
    return shaderPtr == nullptr ? ShaderManager::getInstance().getDefaultShader() : shaderPtr;
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
        _shaderInfo[FINAL_STAGE]._shader.clear();
        computeShader(true);
    }
}

void Material::setBumpMethod(const BumpMethod& newBumpMethod,const bool force){
    _bumpMethod = newBumpMethod;
    if(force){
        _shaderInfo[FINAL_STAGE]._shader.clear();
        computeShader(true);
    }
}

bool Material::unload(){
    for(U8 i = 0; i < Config::MAX_TEXTURE_STORAGE; ++i){
        if(_textures[i] != nullptr){
            UNREGISTER_TRACKED_DEPENDENCY(_textures[i]);
            RemoveResource(_textures[i]);
        }
    }
    return true;
}

void Material::setDoubleSided(bool state) {
    if(_doubleSided == state)
        return;
    
    _doubleSided = state;
    // Update all render states for this item
    if(_doubleSided){
        typedef Unordered_map<RenderStage, RenderStateBlock* >::value_type stateValue;
        FOR_EACH(stateValue& it, _defaultRenderStates){
            RenderStateBlockDescriptor descriptor(it.second->getDescriptor());
            if (it.first != SHADOW_STAGE) descriptor.setCullMode(CULL_MODE_NONE);
            setRenderStateBlock(descriptor, it.first);
        }
    }

    _dirty = true;
}

bool Material::isTranslucent(U8 index) {
    if (!_translucencyCheck){
        _translucencySource.clear();
        // In order of importance (less to more)!
        // diffuse channel alpha
        if(_materialMatrix[index].getCol(1).a < 0.95f) {
            _translucencySource.push_back(TRANSLUCENT_DIFFUSE);
            _useAlphaTest = (getOpacityValue() < 0.15f);
        }

        // base texture is translucent
        if (_textures[TEXTURE_UNIT0] && _textures[TEXTURE_UNIT0]->hasTransparency()){
            _translucencySource.push_back(TRANSLUCENT_DIFFUSE_MAP);
            _useAlphaTest = true;
        }

        // opacity map
        if(_textures[TEXTURE_OPACITY]){
            _translucencySource.push_back(TRANSLUCENT_OPACITY_MAP);
            _useAlphaTest = false;
        }

        // if we have a global opacity value
        if(getOpacityValue() < 0.95f){
            _translucencySource.push_back(TRANSLUCENT_OPACITY);
            _useAlphaTest = (getOpacityValue() < 0.15f);
        }

        // Disable culling for translucent items
        if (!_translucencySource.empty()){
            typedef Unordered_map<RenderStage, RenderStateBlock* >::value_type stateValue;
            FOR_EACH(stateValue& it, _defaultRenderStates){
                RenderStateBlockDescriptor descriptor(it.second->getDescriptor());
                if (it.first != SHADOW_STAGE) descriptor.setCullMode(CULL_MODE_NONE);
                if (!_useAlphaTest) descriptor.setBlend(true);
                setRenderStateBlock(descriptor, it.first);
            }
        }
        _translucencyCheck = true;
        _computedShaderTextures = false;
    }

    return !_translucencySource.empty();
}

void Material::getSortKeys(I32& shaderKey, I32& textureKey) const {
    Unordered_map<RenderStage, ShaderInfo >::const_iterator it = _shaderInfo.find(FINAL_STAGE);
    if (it != _shaderInfo.end()){
        ShaderProgram* shader = it->second._shaderRef;
        shaderKey = shader ? shader->getId() : -1;
    }else{
        shaderKey = GFXDevice::SORT_NO_VALUE;
    }

    textureKey = _textures[TEXTURE_UNIT0] ? _textures[TEXTURE_UNIT0]->getHandle() : GFXDevice::SORT_NO_VALUE;
}