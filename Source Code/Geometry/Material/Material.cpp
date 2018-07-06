#include "Headers/Material.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/ShaderManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

bool Material::_shaderQueueLocked = false;
bool Material::_serializeShaderLoad = false;

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

    I32 i = 0, j = 0;
    for (; i < TEXTURE_UNIT0; ++i)
        sprintf_s(_textureOperationUniformSlots[i], "textureOperation[%d]", TEXTURE_UNIT0 + i);

    for (i = TEXTURE_UNIT0; i < Config::MAX_TEXTURE_STORAGE; ++i)
        sprintf_s(_textureOperationUniformSlots[i], "textureOperation[%d]", j++);

   //std::fill(_textures, _textures + Config::MAX_TEXTURE_STORAGE, static_cast<Texture*>(nullptr));
   //std::fill(_operations, _operations + (Config::MAX_TEXTURE_STORAGE - TEXTURE_UNIT0), TextureOperation_Replace);

   /// Normal state for final rendering
   RenderStateBlockDescriptor stateDescriptor;
   _defaultRenderStates.insert(std::make_pair(FINAL_STAGE,      GFX_DEVICE.getOrCreateStateBlock(stateDescriptor)));
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
   
   assert(_defaultRenderStates[FINAL_STAGE]      != 0);
   assert(_defaultRenderStates[Z_PRE_PASS_STAGE] != 0);
   assert(_defaultRenderStates[SHADOW_STAGE]     != 0);
   assert(_defaultRenderStates[REFLECTION_STAGE] != 0);

   _computedShaderTextures = false;
}

Material::~Material(){
    _defaultRenderStates.clear();
}

void Material::update(const U64 deltaTime){
    // build one shader per frame
    computeShaderInternal();
}

I64 Material::getRenderStateBlock(RenderStage currentStage) {
    const renderStateBlockMap::const_iterator& it = _defaultRenderStates.find(currentStage);
    if (it == _defaultRenderStates.end())
        return _defaultRenderStates[FINAL_STAGE];
    
    return it->second;
}

I64 Material::setRenderStateBlock(const RenderStateBlockDescriptor& descriptor, const RenderStage& renderStage){
    renderStateBlockMap::iterator it = _defaultRenderStates.find(renderStage);
    I64 stateBlockHash = GFX_DEVICE.getOrCreateStateBlock(descriptor);
    if(it == _defaultRenderStates.end()){
        _defaultRenderStates.insert(std::make_pair(renderStage, stateBlockHash));
        return stateBlockHash;
    }

    it->second = stateBlockHash;
    return it->second;
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
    if (force || ((_shaderInfo[renderStage]._shader.empty() || (renderStage == FINAL_STAGE && !_computedShaderTextures)) || !_shaderInfo[renderStage]._computedShader)) {
        bool deferredPassShader = GFX_DEVICE.getRenderer()->getType() != RENDERER_FORWARD;
        bool depthPassShader = renderStage == SHADOW_STAGE || renderStage == Z_PRE_PASS_STAGE;

        //the base shader is either for a Deferred Renderer or a Forward  one ...
        std::string shader = (deferredPassShader ? "DeferredShadingPass1" : (depthPassShader ? "depthPass" : "lighting"));
        if(Config::Profile::DISABLE_SHADING){
            shader = "passThrough";
        }
        if (depthPassShader) renderStage == Z_PRE_PASS_STAGE ? shader += ".PrePass" :  shader += ".Shadow";
 
        //What kind of effects do we need?
        if (_textures[TEXTURE_UNIT0]){
            //Bump mapping?
            if (_textures[TEXTURE_NORMALMAP] && _bumpMethod != BUMP_NONE){
                addShaderDefines(renderStage, "COMPUTE_TBN");
                shader += ".Bump"; //Normal Mapping
                if (_bumpMethod == BUMP_PARALLAX){
                    shader += ".Parallax";
                    addShaderDefines(renderStage, "USE_PARALLAX_MAPPING");
                }else if (_bumpMethod == BUMP_RELIEF){
                    shader += ".Relief";
                    addShaderDefines(renderStage, "USE_RELIEF_MAPPING");
                }
            }else{
                // Or simple texture mapping?
                shader += ".Texture";
            }
        }else{
            addShaderDefines(renderStage, "SKIP_TEXTURES");
            shader += ".NoTexture";
        }

        if (_textures[TEXTURE_SPECULAR]){
            shader += ".Specular";
            addShaderDefines(renderStage, "USE_SPECULAR_MAP");
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
    if (_shaderComputeQueue.empty()/* || Material::isShaderQueueLocked()*/)
        return;

    //Material::lockShaderQueue();

    std::pair<RenderStage, ResourceDescriptor> currentItem = _shaderComputeQueue.front();

    _dirty = true;
    _shaderInfo[currentItem.first]._shaderRef = CreateResource<ShaderProgram>(currentItem.second);
    _shaderInfo[currentItem.first]._computedShader = true;

    _shaderComputeQueue.pop();
}

void Material::UploadToShader(Material::ShaderInfo& shaderInfo){
    ShaderProgram* shader = shaderInfo.getProgram();

    Texture* texture = nullptr;
    for (U16 i = 0; i < Config::MAX_TEXTURE_STORAGE; ++i){
        if ((texture = getTexture(i)) != nullptr){
            texture->Bind(i);
            if (i >= TEXTURE_UNIT0)
                shader->Uniform(_textureOperationUniformSlots[i], (I32)getTextureOperation(i));
        }
    }

    if (isTranslucent()){
        shader->Uniform("opacity",      getOpacityValue());
        shader->Uniform("useAlphaTest", useAlphaTest() || GFX_DEVICE.isCurrentRenderStage(SHADOW_STAGE));
    }

    shader->Uniform("material",     getMaterialMatrix());
    shader->Uniform("textureCount", getTextureCount());
    /*shader->setFunctionIndex(VERTEX_SHADER,   0, 0, shaderInfo._shadingFunction[VERTEX_SHADER][getBumpMethod()]);
    shader->setFunctionIndex(VERTEX_SHADER,   1, 0, shaderInfo._shadingFunction[VERTEX_SHADER][BUMP_NONE]);
    shader->setFunctionIndex(FRAGMENT_SHADER, 0, 0, shaderInfo._shadingFunction[FRAGMENT_SHADER][getBumpMethod()]);
    shader->setFunctionIndex(FRAGMENT_SHADER, 1, 0, shaderInfo._shadingFunction[FRAGMENT_SHADER][BUMP_NONE]);*/
}

ShaderProgram* const Material::ShaderInfo::getProgram(){
    return _shaderRef == nullptr ? ShaderManager::getInstance().getDefaultShader() : _shaderRef;
}

Material::ShaderInfo& Material::getShaderInfo(RenderStage renderStage) {
    Unordered_map<RenderStage, ShaderInfo >::iterator it = _shaderInfo.find(renderStage);
    if(it == _shaderInfo.end())
        return _shaderInfo[FINAL_STAGE];

    return it->second;
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

void Material::setDoubleSided(bool state, const bool useAlphaTest) {
    if(_doubleSided == state && _useAlphaTest == useAlphaTest)
        return;
    
    _doubleSided = state;
    _useAlphaTest = useAlphaTest;
    // Update all render states for this item
    if(_doubleSided){
        FOR_EACH(renderStateBlockMap::value_type& it, _defaultRenderStates){
            RenderStateBlockDescriptor descriptor(GFX_DEVICE.getStateBlockDescriptor(it.second));
            if (it.first != SHADOW_STAGE) descriptor.setCullMode(CULL_MODE_NONE);
            if (!_useAlphaTest) descriptor.setBlend(true);
            setRenderStateBlock(descriptor, it.first);
        }
    }

    _dirty = true;
}

bool Material::isTranslucent(U8 index) {
    if (!_translucencyCheck){
        _translucencySource.clear();
        bool useAlphaTest = false;
        // In order of importance (less to more)!
        // diffuse channel alpha
        if(_materialMatrix[index].getCol(1).a < 0.95f) {
            _translucencySource.push_back(TRANSLUCENT_DIFFUSE);
            useAlphaTest = (getOpacityValue() < 0.15f);
        }

        // base texture is translucent
        if (_textures[TEXTURE_UNIT0] && _textures[TEXTURE_UNIT0]->hasTransparency()){
            _translucencySource.push_back(TRANSLUCENT_DIFFUSE_MAP);
            useAlphaTest = true;
        }

        // opacity map
        if(_textures[TEXTURE_OPACITY]){
            _translucencySource.push_back(TRANSLUCENT_OPACITY_MAP);
            useAlphaTest = false;
        }

        // if we have a global opacity value
        if(getOpacityValue() < 0.95f){
            _translucencySource.push_back(TRANSLUCENT_OPACITY);
            useAlphaTest = (getOpacityValue() < 0.15f);
        }

        // Disable culling for translucent items
        if (!_translucencySource.empty()){
            setDoubleSided(true, useAlphaTest);
        }
        _translucencyCheck = true;
        _computedShaderTextures = false;
    }

    return !_translucencySource.empty();
}

void Material::getSortKeys(I32& shaderKey, I32& textureKey) const {
    Unordered_map<RenderStage, ShaderInfo >::const_iterator it = _shaderInfo.find(FINAL_STAGE);

    shaderKey  = (it != _shaderInfo.end() && it->second._shaderRef) ? it->second._shaderRef->getId() : GFXDevice::SORT_NO_VALUE;
    textureKey = _textures[TEXTURE_UNIT0] ? _textures[TEXTURE_UNIT0]->getHandle() : GFXDevice::SORT_NO_VALUE;
}