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
                       _hardwareSkinning(false),
                       _useAlphaTest(true),
                       _gsInputType(GS_TRIANGLES),
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
       _textures[i] = NULL;
   for(U8 i = 0; i < Config::MAX_TEXTURE_STORAGE - TEXTURE_UNIT0; ++i)
       _operations[i] = TextureOperation_Replace;
   //std::fill(_textures, _textures + Config::MAX_TEXTURE_STORAGE, static_cast<Texture2D*>(NULL));
   //std::fill(_operations, _operations + (Config::MAX_TEXTURE_STORAGE - TEXTURE_UNIT0), TextureOperation_Replace);

   /// Normal state for final rendering
   RenderStateBlockDescriptor stateDescriptor;
   _defaultRenderStates.insert(std::make_pair(FINAL_STAGE, GFX_DEVICE.createStateBlock(stateDescriptor)));
   /// the reflection descriptor is the same as the normal descriptor
   _defaultRenderStates.insert(std::make_pair(REFLECTION_STAGE, GFX_DEVICE.createStateBlock(stateDescriptor)));
   /// the z-pre-pass descriptor does not process colors
   stateDescriptor.setColorWrites(false,false,false,false);
   _defaultRenderStates.insert(std::make_pair(Z_PRE_PASS_STAGE, GFX_DEVICE.createStateBlock(stateDescriptor)));
   /// A descriptor used for rendering to depth map
   stateDescriptor.setCullMode(CULL_MODE_CCW);
   /// set a polygon offset
   stateDescriptor._zBias = 1.1f;
   /// ignore colors - Some shadowing techniques require drawing to the a color buffer
   stateDescriptor.setColorWrites(true,true,false,false);
   _defaultRenderStates.insert(std::make_pair(SHADOW_STAGE, GFX_DEVICE.createStateBlock(stateDescriptor)));
   

    assert(_defaultRenderStates[FINAL_STAGE] != NULL);
    assert(_defaultRenderStates[Z_PRE_PASS_STAGE] != NULL);
    assert(_defaultRenderStates[SHADOW_STAGE] != NULL);
    assert(_defaultRenderStates[REFLECTION_STAGE] != NULL);

    _shaderRef[FINAL_STAGE] = NULL;
    _shaderRef[Z_PRE_PASS_STAGE] = NULL;
    _shaderRef[SHADOW_STAGE] = NULL;

    _computedShader[0] = false;
    _computedShader[1] = false;
    _computedShader[2] = false;
    _computedShaderTextures = false;
    _matId[0].i = 0;
    _matId[1].i = 0;
    _matId[2].i = 0;
}

Material::~Material(){
    SAFE_DELETE(_defaultRenderStates[FINAL_STAGE]);
    SAFE_DELETE(_defaultRenderStates[Z_PRE_PASS_STAGE]);
    SAFE_DELETE(_defaultRenderStates[SHADOW_STAGE]);
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
void Material::setTexture(U32 textureUsageSlot, Texture2D* const texture, const TextureOperation& op, U8 index) {
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

    if(textureUsageSlot >= TEXTURE_UNIT0){
        texture ? _shaderData[index]._textureCount++ : _shaderData[index]._textureCount--;
    }

    if(texture){
        REGISTER_TRACKED_DEPENDENCY(_textures[textureUsageSlot]);
    }
    _dirty = true;
}

//Here we set the shader's name
ShaderProgram* Material::setShaderProgram(const std::string& shader, const RenderStage& renderStage){
    ShaderProgram* shaderReference = _shaderRef[renderStage];
    U8 id = (renderStage == FINAL_STAGE ? 0 : (renderStage == Z_PRE_PASS_STAGE ? 1 : 2));
    //if we already had a shader assigned ...
    if(!_shader[id].empty()){
        //and we are trying to assing the same one again, return.
        shaderReference = FindResourceImpl<ShaderProgram>(_shader[id]);
        if(_shader[id].compare(shader) == 0){
            return shaderReference;
        }else{
            PRINT_FN(Locale::get("REPLACE_SHADER"),_shader[id].c_str(),shader.c_str());
            RemoveResource(shaderReference);
        }
    }

    (!shader.empty()) ? _shader[id] = shader : _shader[id] = "NULL_SHADER";

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
    
    _dirty = true;
    _computedShader[id] = true;
    _computedShaderTextures = true;
    _shaderRef[renderStage] = shaderReference;

    return shaderReference;
}

void Material::clean() {
    if(_dirty){
        isTranslucent();
        _matId[0].i = (_shaderRef[FINAL_STAGE] != NULL ?  _shaderRef[FINAL_STAGE]->getId() : 0);
        _matId[1].i = (_shaderRef[Z_PRE_PASS_STAGE] != NULL ?  _shaderRef[Z_PRE_PASS_STAGE]->getId() : 0);
        _matId[2].i = (_shaderRef[SHADOW_STAGE] != NULL ?  _shaderRef[SHADOW_STAGE]->getId() : 0);
        dumpToXML();
       _dirty = false;
    }
}

///If the current material doesn't have a shader associated with it, then add the default ones.
///Manually setting a shader, overrides this function by setting _computedShaders to "true"
void Material::computeShader(bool force, const RenderStage& renderStage){
    bool deferredPassShader = GFX_DEVICE.getRenderer()->getType() != RENDERER_FORWARD;
    bool depthPassShader = renderStage == SHADOW_STAGE || renderStage == Z_PRE_PASS_STAGE;
    //bool forwardPassShader = !deferredPassShader && !depthPassShader;

    U8 id = (renderStage == FINAL_STAGE ? 0 : (renderStage == Z_PRE_PASS_STAGE ? 1 : 2));
    if(_computedShader[id] && !force && (id == 0 && _computedShaderTextures)) return;
    if(_shader[id].empty() || (id == 0 && !_computedShaderTextures)){
        //the base shader is either for a Deferred Renderer or a Forward  one ...
        std::string shader = (deferredPassShader ? "DeferredShadingPass1" : (depthPassShader ? "depthPass" : "lighting"));

        if(renderStage == Z_PRE_PASS_STAGE)
            shader += ".PrePass";

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
        }

        if(isTranslucent()){
            switch(_translucencySource){
                case TRANSLUCENT_DIFFUSE :{
                    shader += ".OpacityDiffuse";
                    addShaderDefines(id, "USE_OPACITY_DIFFUSE");
                }break;
                case TRANSLUCENT_OPACITY :{
                    shader += ".Opacity";
                    addShaderDefines(id, "USE_OPACITY");
                }break;
                case TRANSLUCENT_OPACITY_MAP :{
                    shader += ".OpacityMap";
                    addShaderDefines(id, "USE_OPACITY_MAP");
                }break;
                case TRANSLUCENT_DIFFUSE_MAP :{
                    shader += ".OpacityDiffuseMap";
                    addShaderDefines(id, "USE_OPACITY_DIFFUSE_MAP");
                }break;
            }
        }

        if(_textures[TEXTURE_SPECULAR]){
            shader += ".Specular";
            addShaderDefines(id, "USE_SPECULAR_MAP");
        }
        //if this is true, geometry shader will take a triangle strip as input, else it will use triangles
        if(_gsInputType == GS_TRIANGLES){
            shader += ".Triangles";
            addShaderDefines(id, "USE_GEOMETRY_TRIANGLE_INPUT");
        }else if(_gsInputType == GS_LINES){
            shader += ".Lines";
            addShaderDefines(id, "USE_GEOMETRY_LINE_INPUT");
        }else{
            shader += ".Points";
            addShaderDefines(id, "USE_GEOMETRY_POINT_INPUT");
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
    ShaderProgram* shaderPtr = _shaderRef[renderStage];
    return shaderPtr == NULL ? ShaderManager::getInstance().getDefaultShader() : shaderPtr;
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
        _shader[SHADOW_STAGE].clear();
        _shader[Z_PRE_PASS_STAGE].clear();
        computeShader(true);
        computeShader(true,SHADOW_STAGE);
        computeShader(true,Z_PRE_PASS_STAGE);
    }
}

bool Material::unload(){
    for(U8 i = 0; i < Config::MAX_TEXTURE_STORAGE; ++i){
        if(_textures[i] != NULL){
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

bool Material::isTranslucent(U8 index) {
    bool state = false;

    // In order of importance (less to more)!
    // diffuse channel alpha
    if(_materialMatrix[index].getCol(1).a < 0.98f) {
        state = true;
        _translucencySource = TRANSLUCENT_DIFFUSE;
        _useAlphaTest = (getOpacityValue() < 0.15f);
    }

    // base texture is translucent
    if(_textures[TEXTURE_UNIT0]){
        if(_textures[TEXTURE_UNIT0]->hasTransparency()) state = true;
        _translucencySource = TRANSLUCENT_DIFFUSE_MAP;
    }

    // opacity map
    if(_textures[TEXTURE_OPACITY]){
        state = true;
        _translucencySource = TRANSLUCENT_OPACITY_MAP;
    }

    // if we have a global opacity value
    if(getOpacityValue() < 0.98f){
        state = true;
        _translucencySource = TRANSLUCENT_OPACITY;
        _useAlphaTest = (getOpacityValue() < 0.15f);
    }

    // Disable culling for translucent items
    if(state && !_translucencyCheck){
        typedef Unordered_map<RenderStage, RenderStateBlock* >::value_type stateValue;
        for_each(stateValue& it, _defaultRenderStates){
            it.second->getDescriptor().setCullMode(CULL_MODE_NONE);
            if(!_useAlphaTest) it.second->getDescriptor().setBlend(true);
        }
        _translucencyCheck = true;
    }
    return state;
}

P32 Material::getMaterialId(const RenderStage& renderStage){
    return _matId[renderStage == FINAL_STAGE ? 0 : 1];
}