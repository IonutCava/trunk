#include "Headers/Material.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/ShaderManager.h"
#include "Hardware/Video/Textures/Headers/Texture.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

Material::Material() : Resource(),
                       _dirty(false),
                       _doubleSided(false),
                       _shaderThreadedLoad(true),
                       _castsShadows(true),
                       _receiveShadows(true),
                       _hardwareSkinning(false),
                       _useStripInput(false),
                       _shadingMode(SHADING_PHONG), /// phong shading by default
                       _bumpMethod(BUMP_NONE)
{
   _materialMatrix.setCol(0,_shaderData._ambient);
   _materialMatrix.setCol(1,_shaderData._diffuse);
   _materialMatrix.setCol(2,_shaderData._specular);
   _materialMatrix.setCol(3,vec4<F32>(_shaderData._shininess,
                                      _shaderData._emissive.x,
                                      _shaderData._emissive.y,
                                      _shaderData._emissive.z));

   for(U8 i = 0; i < Config::MAX_TEXTURE_STORAGE; ++i)
       _textures[i] = NULL;
   for(U8 i = 0; i < Config::MAX_TEXTURE_STORAGE - TEXTURE_UNIT0; ++i)
       _operations[i] = TextureOperation_Replace;
   //std::fill(_textures, _textures + Config::MAX_TEXTURE_STORAGE, static_cast<Texture2D*>(NULL));
   //std::fill(_operations, _operations + (Config::MAX_TEXTURE_STORAGE - TEXTURE_UNIT0), TextureOperation_Replace);

   /// Normal state for final rendering
   RenderStateBlockDescriptor normalStateDescriptor;

   /// A descriptor used for rendering to depth map
   RenderStateBlockDescriptor depthDescriptor;
   depthDescriptor.setCullMode(CULL_MODE_CCW);
   /// set a polygon offset
   depthDescriptor._zBias = 1.1f;
   /// ignore colors
   depthDescriptor.setColorWrites(false,false,false,true);

   /// the reflection descriptor is the same as the normal descriptor, but with special culling
   RenderStateBlockDescriptor reflectionStateDescriptor;
   reflectionStateDescriptor.fromDescriptor(normalStateDescriptor);

   _defaultRenderStates.insert(std::make_pair(FINAL_STAGE, GFX_DEVICE.createStateBlock(normalStateDescriptor)));
   _defaultRenderStates.insert(std::make_pair(DEPTH_STAGE, GFX_DEVICE.createStateBlock(depthDescriptor)));
   _defaultRenderStates.insert(std::make_pair(REFLECTION_STAGE, GFX_DEVICE.createStateBlock(reflectionStateDescriptor)));

    assert(_defaultRenderStates[FINAL_STAGE] != NULL);
    assert(_defaultRenderStates[DEPTH_STAGE] != NULL);
    assert(_defaultRenderStates[REFLECTION_STAGE] != NULL);

    _shaderRef[FINAL_STAGE] = NULL;
    _shaderRef[DEPTH_STAGE] = NULL;

    //Create an immediate mode shader (one should exist already)
    ResourceDescriptor immediateModeShader("ImmediateModeEmulation");
    _imShader = CreateResource<ShaderProgram>(immediateModeShader);
    REGISTER_TRACKED_DEPENDENCY(_imShader);

    _computedShader[0] = false;
    _computedShader[1] = false;
    _computedShaderTextures = false;
    _matId[0].i = 0;
    _matId[1].i = 0;
}

Material::~Material(){
    SAFE_DELETE(_defaultRenderStates[FINAL_STAGE]);
    SAFE_DELETE(_defaultRenderStates[DEPTH_STAGE]);
    SAFE_DELETE(_defaultRenderStates[REFLECTION_STAGE]);
    _defaultRenderStates.clear();
}

RenderStateBlock* Material::setRenderStateBlock(const RenderStateBlockDescriptor& descriptor,const RenderStage& renderStage){
    if(descriptor.getGUID() == _defaultRenderStates[renderStage]->getDescriptor().getGUID()){
        return _defaultRenderStates[renderStage];
    }

    SAFE_DELETE(_defaultRenderStates[renderStage]);

    _defaultRenderStates[renderStage] = GFX_DEVICE.createStateBlock(descriptor);
    return _defaultRenderStates[renderStage];
}

//base = base texture
//second = second texture used for multitexturing
//bump = bump map
void Material::setTexture(U32 textureUsageSlot, Texture2D* const texture, const TextureOperation& op) {
    if(_textures[textureUsageSlot]){
        UNREGISTER_TRACKED_DEPENDENCY(_textures[textureUsageSlot]);
        RemoveResource(_textures[textureUsageSlot]);
        if(textureUsageSlot == TEXTURE_OPACITY || textureUsageSlot == TEXTURE_SPECULAR){
            _computedShaderTextures = false;
        }
    }else{
        //if we add a new type of texture
        _computedShader[0] = false; //recompute shaders on texture change
    }
    _textures[textureUsageSlot] = texture;

    if(textureUsageSlot >= TEXTURE_UNIT0)
        _operations[textureUsageSlot - TEXTURE_UNIT0] = op;

    REGISTER_TRACKED_DEPENDENCY(_textures[textureUsageSlot]);

    if(textureUsageSlot >= TEXTURE_UNIT0){
        texture ? _shaderData._textureCount++ : _shaderData._textureCount--;
    }

    _dirty = true;
}

//Here we set the shader's name
ShaderProgram* Material::setShaderProgram(const std::string& shader, const RenderStage& renderStage){
    ShaderProgram* shaderReference = _shaderRef[renderStage];
    U8 id = (renderStage == FINAL_STAGE ? 0 : 1);
    //if we already had a shader assigned ...
    if(!_shader[id].empty()){
        //and we are trying to assing the same one again, return.
        if(_shader[id].compare(shader) == 0){
            shaderReference = FindResource<ShaderProgram>(_shader[id]);
            return shaderReference;
        }else{
            PRINT_FN(Locale::get("REPLACE_SHADER"),_shader[id].c_str(),shader.c_str());
        }
    }

    (!shader.empty()) ? _shader[id] = shader : _shader[id] = "NULL_SHADER";

    shaderReference = FindResource<ShaderProgram>(_shader[id]);
    if(!shaderReference){
        ResourceDescriptor shaderDescriptor(_shader[id]);
        std::stringstream ss;
        if(!_shaderDefines[id].empty()){
            for_each(std::string& shaderDefine, _shaderDefines[id]){
                ss << shaderDefine;
                ss << ",";
            }
        }
        ss << "DEFINE_PLACEHOLDER";
        shaderDescriptor.setPropertyList(ss.str());
        shaderDescriptor.setThreadedLoading(_shaderThreadedLoad);
        shaderReference = CreateResource<ShaderProgram>(shaderDescriptor);
        if(id == 1){//depth shader
            shaderReference->setMatrixMask(false,false,true,true);
        }
    }
    _dirty = true;
    _computedShader[id] = true;
    _computedShaderTextures = true;
    _shaderRef[renderStage] = shaderReference;

    return shaderReference;
}

void Material::clean() {
    if(_dirty){
        _matId[0].i = (_shaderRef[FINAL_STAGE] != NULL ?  _shaderRef[FINAL_STAGE]->getId() : 0);
        _matId[1].i = (_shaderRef[DEPTH_STAGE] != NULL ?  _shaderRef[DEPTH_STAGE]->getId() : 0);
        dumpToXML();
       _dirty = false;
    }
}

void Material::setReceivesShadows(const bool state) {
    _receiveShadows = state;
}

///If the current material doesn't have a shader associated with it, then add the default ones.
///Manually setting a shader, overrides this function by setting _computedShaders to "true"
void Material::computeShader(bool force, const RenderStage& renderStage){
    U8 id = (renderStage == FINAL_STAGE ? 0 : 1);
    if(_computedShader[id] && !force && (id == 0 && _computedShaderTextures)) return;
    if(_shader[id].empty() || (id == 0 && !_computedShaderTextures)){
        //the base shader is either for a Deferred Renderer or a Forward  one ...
        std::string shader = (GFX_DEVICE.getRenderer()->getType() != RENDERER_FORWARD ? "DeferredShadingPass1" :
                             (renderStage == DEPTH_STAGE ? "depthPass" : "lighting"));

        //What kind of effects do we need?
        if(_textures[TEXTURE_UNIT0]){
            //Bump mapping?
            if(_textures[TEXTURE_NORMALMAP] && _bumpMethod != BUMP_NONE){
                addShaderDefines(id, "COMPUTE_TBN");
                shader += ".Bump"; //Normal Mapping
                if(_bumpMethod == 2){ //Parallax Mapping
                    shader += ".Parallax";
                    addShaderDefines(id, "USE_PARALLAX_MAPPING");
                }else if(_bumpMethod == 3){ //Relief Mapping
                    shader += ".Relief";
                    addShaderDefines(id, "USE_RELIEF_MAPPING");
                }
            }else{
                // Or simple texture mapping?
                shader += ".Texture";
            }
        }else{
            // Or just color mapping? Use the simple fragment
        }

        if(_textures[TEXTURE_OPACITY]){
            shader += ".Opacity";
            addShaderDefines(id, "USE_OPACITY_MAP");
        }
        if(_textures[TEXTURE_SPECULAR]){
            shader += ".Specular";
            addShaderDefines(id, "USE_SPECULAR_MAP");
        }
        //if this is true, geometry shader will take a triangle strip as input, else it will use triangles
        if(_useStripInput){
            shader += ".Strip";
            addShaderDefines(id, "USE_GEOMETRY_STRIP_INPUT");
        }
        //Add the GPU skinnig module to the vertex shader?
        if(_hardwareSkinning){
            addShaderDefines(id, "USE_GPU_SKINNING");
            shader += ",Skinned"; //<Use "," instead of "." will add a Vertex only property
        }

        //Add any modifiers you wish
        if(!_shaderModifier.empty()){
            shader += ".";
            shader += _shaderModifier;
        }

        setShaderProgram(shader,renderStage);
    }
}

ShaderProgram* const Material::getShaderProgram(RenderStage renderStage) {
    ShaderProgram* shaderPr = _shaderRef[renderStage];

    if(shaderPr == NULL)
        shaderPr = _imShader;

    return shaderPr;
}

void Material::setCastsShadows(bool state) {
    if(_castsShadows == state)
        return;

    _castsShadows = state;
    _dirty = true;

    if(!state) _shader[1] = "NULL_SHADER";
    else _shader[1].clear();

    _computedShader[1] = false;
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

void Material::setBumpMethod(const BumpMethod& newBumpMethod,const bool force){
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

void Material::addShaderDefines(U8 shaderId, const std::string& shaderDefines, bool force) {
    _shaderDefines[shaderId].push_back(shaderDefines);
    if(force){
        _shader[FINAL_STAGE].clear();
        _shader[DEPTH_STAGE].clear();
        computeShader(true);
        computeShader(true,DEPTH_STAGE);
    }
}

bool Material::unload(){
    for(U8 i = 0; i < Config::MAX_TEXTURE_STORAGE; ++i){
        if(_textures[i] != NULL){
            UNREGISTER_TRACKED_DEPENDENCY(_textures[i]);
            RemoveResource(_textures[i]);
        }
    }
    UNREGISTER_TRACKED_DEPENDENCY(_imShader);
    RemoveResource(_imShader);
    return true;
}

void Material::setDoubleSided(bool state) {
    if(_doubleSided == state)
        return;

    _doubleSided = state;
    /// Update all render states for this item
    if(_doubleSided){
        typedef Unordered_map<RenderStage, RenderStateBlock* >::value_type stateValue;
        for_each(stateValue& it, _defaultRenderStates){
            RenderStateBlockDescriptor& desc =  it.second->getDescriptor();
            desc.setCullMode(CULL_MODE_NONE);
        }
    }

    _dirty = true;
}

bool Material::isTranslucent() {
    bool state = false;
    // base texture is translucent
    if(_textures[TEXTURE_UNIT0]){
        if(_textures[TEXTURE_UNIT0]->hasTransparency()) state = true;
    }
    // opacity map
    if(_textures[TEXTURE_OPACITY])
        state = true;
    // diffuse channel alpha
    if(_materialMatrix.getCol(1).a < 1.0f)
        state = true;

    // if we have a global opacity value
    if(getOpacityValue() < 1.0f)
        state = true;

    // Disable culling for translucent items
    if(state){
        typedef Unordered_map<RenderStage, RenderStateBlock* >::value_type stateValue;
        for_each(stateValue it, _defaultRenderStates){
            RenderStateBlockDescriptor& desc =  it.second->getDescriptor();
            desc.setCullMode(CULL_MODE_NONE);
        }
    }
    return state;
}

P32 Material::getMaterialId(const RenderStage& renderStage){
    return _matId[renderStage == FINAL_STAGE ? 0 : 1];
}