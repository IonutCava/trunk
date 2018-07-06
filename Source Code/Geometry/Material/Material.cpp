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
       _textures[i] = nullptr;
   for(U8 i = 0; i < Config::MAX_TEXTURE_STORAGE - TEXTURE_UNIT0; ++i)
       _operations[i] = TextureOperation_Replace;
   //std::fill(_textures, _textures + Config::MAX_TEXTURE_STORAGE, static_cast<Texture2D*>(nullptr));
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
   
    assert(_defaultRenderStates[FINAL_STAGE] != nullptr);
    assert(_defaultRenderStates[Z_PRE_PASS_STAGE] != nullptr);
    assert(_defaultRenderStates[SHADOW_STAGE] != nullptr);
    assert(_defaultRenderStates[REFLECTION_STAGE] != nullptr);

    _computedShaderTextures = false;
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
        _shaderInfo[FINAL_STAGE]._computedShader = false; //recompute shaders on texture change
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
    ShaderProgram* shaderReference = _shaderInfo[renderStage]._shaderRef;
    //if we already had a shader assigned ...
    if (!_shaderInfo[renderStage]._shader.empty()){
        //and we are trying to assing the same one again, return.
        shaderReference = FindResourceImpl<ShaderProgram>(_shaderInfo[renderStage]._shader);
        if (_shaderInfo[renderStage]._shader.compare(shader) == 0){
            return shaderReference;
        }else{
            PRINT_FN(Locale::get("REPLACE_SHADER"), _shaderInfo[renderStage]._shader.c_str(), shader.c_str());
            RemoveResource(shaderReference);
        }
    }

    (!shader.empty()) ? _shaderInfo[renderStage]._shader = shader : _shaderInfo[renderStage]._shader = "NULL_SHADER";

    ResourceDescriptor shaderDescriptor(_shaderInfo[renderStage]._shader);
    std::stringstream ss;
    if (!_shaderInfo[renderStage]._shaderDefines.empty()){
        FOR_EACH(std::string& shaderDefine, _shaderInfo[renderStage]._shaderDefines){
            ss << shaderDefine;
            ss << ",";
        }
    }
    ss << "DEFINE_PLACEHOLDER";
    shaderDescriptor.setPropertyList(ss.str());
    shaderDescriptor.setThreadedLoading(_shaderThreadedLoad);
    shaderReference = CreateResource<ShaderProgram>(shaderDescriptor);
    
    _dirty = true;
    _shaderInfo[renderStage]._computedShader = true;
    _computedShaderTextures = true;
    _shaderInfo[renderStage]._shaderRef = shaderReference;

    return shaderReference;
}

void Material::clean() {
    if(_dirty){
        isTranslucent();
        _shaderInfo[FINAL_STAGE]._matId.i = (_shaderInfo[FINAL_STAGE]._shaderRef != nullptr ? _shaderInfo[FINAL_STAGE]._shaderRef->getId() : 0);
        _shaderInfo[Z_PRE_PASS_STAGE]._matId.i = (_shaderInfo[Z_PRE_PASS_STAGE]._shaderRef != nullptr ? _shaderInfo[Z_PRE_PASS_STAGE]._shaderRef->getId() : 0);
        _shaderInfo[SHADOW_STAGE]._matId.i = (_shaderInfo[SHADOW_STAGE]._shaderRef != nullptr ? _shaderInfo[SHADOW_STAGE]._shaderRef->getId() : 0);
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

    if(_shaderInfo[renderStage]._computedShader && !force && (renderStage == FINAL_STAGE && _computedShaderTextures)) return;
    if(_shaderInfo[renderStage]._shader.empty() || (renderStage == FINAL_STAGE && !_computedShaderTextures)){
        //the base shader is either for a Deferred Renderer or a Forward  one ...
        std::string shader = (deferredPassShader ? "DeferredShadingPass1" : (depthPassShader ? "depthPass" : "lighting"));

        if(renderStage == Z_PRE_PASS_STAGE)
            shader += ".PrePass";

        //What kind of effects do we need?
        if(_textures[TEXTURE_UNIT0]){
            //Bump mapping?
            if(_textures[TEXTURE_NORMALMAP] && _bumpMethod != BUMP_NONE){
                addShaderDefines(renderStage, "COMPUTE_TBN");
                shader += ".Bump"; //Normal Mapping
                if(_bumpMethod == 2){ //Parallax Mapping
                    shader += ".Parallax";
                    addShaderDefines(renderStage, "USE_PARALLAX_MAPPING");
                }else if(_bumpMethod == 3){ //Relief Mapping
                    shader += ".Relief";
                    addShaderDefines(renderStage, "USE_RELIEF_MAPPING");
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
                    addShaderDefines(renderStage, "USE_OPACITY_DIFFUSE");
                }break;
                case TRANSLUCENT_OPACITY :{
                    shader += ".Opacity";
                    addShaderDefines(renderStage, "USE_OPACITY");
                }break;
                case TRANSLUCENT_OPACITY_MAP :{
                    shader += ".OpacityMap";
                    addShaderDefines(renderStage, "USE_OPACITY_MAP");
                }break;
                case TRANSLUCENT_DIFFUSE_MAP :{
                    shader += ".OpacityDiffuseMap";
                    addShaderDefines(renderStage, "USE_OPACITY_DIFFUSE_MAP");
                }break;
            }
        }

        if(_textures[TEXTURE_SPECULAR]){
            shader += ".Specular";
            addShaderDefines(renderStage, "USE_SPECULAR_MAP");
        }
        //if this is true, geometry shader will take a triangle strip as input, else it will use triangles
        if(_gsInputType == GS_TRIANGLES){
            shader += ".Triangles";
            addShaderDefines(renderStage, "USE_GEOMETRY_TRIANGLE_INPUT");
        }else if(_gsInputType == GS_LINES){
            shader += ".Lines";
            addShaderDefines(renderStage, "USE_GEOMETRY_LINE_INPUT");
        }else{
            shader += ".Points";
            addShaderDefines(renderStage, "USE_GEOMETRY_POINT_INPUT");
        }

        //Add the GPU skinnig module to the vertex shader?
        if(_hardwareSkinning){
            addShaderDefines(renderStage, "USE_GPU_SKINNING");
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
    /// Update all render states for this item
    if(_doubleSided){
        typedef Unordered_map<RenderStage, RenderStateBlock* >::value_type stateValue;
        FOR_EACH(stateValue& it, _defaultRenderStates){
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
        FOR_EACH(stateValue& it, _defaultRenderStates){
            it.second->getDescriptor().setCullMode(CULL_MODE_NONE);
            if(!_useAlphaTest) it.second->getDescriptor().setBlend(true);
        }
        _translucencyCheck = true;
    }
    return state;
}