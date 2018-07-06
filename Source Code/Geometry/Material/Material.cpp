#include "Headers/Material.h"

#include "Rendering/Headers/Renderer.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"

#include "Core/Headers/Console.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

bool Material::_shaderQueueLocked = false;
bool Material::_serializeShaderLoad = false;

Material::Material() : Resource("temp_material"),
                       _parallaxFactor(1.0f),
                       _dirty(false),
                       _doubleSided(false),
                       _shaderThreadedLoad(true),
                       _hardwareSkinning(false),
                       _useAlphaTest(false),
                       _dumpToFile(true),
                       _translucencyCheck(false),
                       _shadingMode(ShadingMode_PLACEHOLDER),
                       _bumpMethod(BUMP_NONE)
{
    _textures.resize(ShaderProgram::TextureUsage_PLACEHOLDER, nullptr);

    _operation = TextureOperation_Replace;

    /// Normal state for final rendering
    RenderStateBlockDescriptor stateDescriptor;
    hashAlg::insert(_defaultRenderStates, 
                    hashAlg::makePair(FINAL_STAGE, GFX_DEVICE.getOrCreateStateBlock(stateDescriptor)));
    /// the reflection descriptor is the same as the normal descriptor
    RenderStateBlockDescriptor reflectorDescriptor(stateDescriptor);
    hashAlg::insert(_defaultRenderStates, 
                    hashAlg::makePair(REFLECTION_STAGE, GFX_DEVICE.getOrCreateStateBlock(reflectorDescriptor)));
    /// the z-pre-pass descriptor does not process colors
    RenderStateBlockDescriptor zPrePassDescriptor(stateDescriptor);
    zPrePassDescriptor.setColorWrites(false, false, false, false);
    hashAlg::insert(_defaultRenderStates, 
                    hashAlg::makePair(Z_PRE_PASS_STAGE, GFX_DEVICE.getOrCreateStateBlock(zPrePassDescriptor)));
    /// A descriptor used for rendering to depth map
    RenderStateBlockDescriptor shadowDescriptor(stateDescriptor);
    shadowDescriptor.setCullMode(CULL_MODE_CCW);
    /// set a polygon offset
    //shadowDescriptor.setZBias(1.0f, 2.0f);
    /// ignore colors - Some shadowing techniques require drawing to the a color buffer
    shadowDescriptor.setColorWrites(true, true, false, false);
    hashAlg::insert(_defaultRenderStates, 
                    hashAlg::makePair(SHADOW_STAGE, GFX_DEVICE.getOrCreateStateBlock(shadowDescriptor)));
   
    assert(_defaultRenderStates[FINAL_STAGE]      != 0);
    assert(_defaultRenderStates[Z_PRE_PASS_STAGE] != 0);
    assert(_defaultRenderStates[SHADOW_STAGE]     != 0);
    assert(_defaultRenderStates[REFLECTION_STAGE] != 0);

    _computedShaderTextures = false;
}

Material::~Material()
{
    _defaultRenderStates.clear();
}

Material* Material::clone(const stringImpl& nameSuffix) {
    DIVIDE_ASSERT(!nameSuffix.empty(),
                  "Material error: clone called without a valid name suffix!");

    const Material& base = *this;
    Material* cloneMat = CreateResource<Material>(ResourceDescriptor(getName() + nameSuffix));

    cloneMat->_translucencySource.clear();
    for (const shaderInfoMap::value_type& it : base._shaderInfo) {
        cloneMat->_shaderInfo[it.first] = it.second;
    }
    for (const TranslucencySource& trans : base._translucencySource){
        cloneMat->_translucencySource.push_back(trans);
    }
    for (const renderStateBlockMap::value_type& it : base._defaultRenderStates){
        cloneMat->_defaultRenderStates[it.first] = it.second;
    }
    for (U8 i = 0; i < static_cast<U8>(base._textures.size()); ++i) {
        Texture* const tex = base._textures[i];
        if (tex) {
            tex->AddRef();
            cloneMat->setTexture(static_cast<ShaderProgram::TextureUsage>(i), tex);
        }
    }
    for (const std::pair<Texture*, U32>& tex : base._customTextures) {
        if (tex.first) {
            tex.first->AddRef();
            cloneMat->addCustomTexture(tex.first, tex.second);
        }
    }
    cloneMat->_shadingMode = base._shadingMode;
    cloneMat->_shaderModifier = base._shaderModifier;
    cloneMat->_translucencyCheck = base._translucencyCheck;
    cloneMat->_dirty = base._dirty;
    cloneMat->_dumpToFile = base._dumpToFile;
    cloneMat->_useAlphaTest = base._useAlphaTest;
    cloneMat->_doubleSided = base._doubleSided;
    cloneMat->_hardwareSkinning = base._hardwareSkinning;
    cloneMat->_shaderThreadedLoad = base._shaderThreadedLoad;
    cloneMat->_computedShaderTextures = base._computedShaderTextures;
    cloneMat->_operation = base._operation;
    cloneMat->_bumpMethod = base._bumpMethod;
    cloneMat->_shaderData = base._shaderData;
    cloneMat->_parallaxFactor = base._parallaxFactor;

    return cloneMat;
}

void Material::update(const U64 deltaTime){
    // build one shader per frame
    computeShaderInternal();
    clean();
}

size_t Material::getRenderStateBlock(RenderStage currentStage) {
    const renderStateBlockMap::const_iterator& it = _defaultRenderStates.find(currentStage);
    if (it == std::end(_defaultRenderStates)) {
        return _defaultRenderStates[FINAL_STAGE];
    }
    return it->second;
}

size_t Material::setRenderStateBlock(const RenderStateBlockDescriptor& descriptor, const RenderStage& renderStage){
    renderStateBlockMap::iterator it = _defaultRenderStates.find(renderStage);
    size_t stateBlockHash = GFX_DEVICE.getOrCreateStateBlock(descriptor);
    if(it == std::end(_defaultRenderStates)){
        hashAlg::insert(_defaultRenderStates, hashAlg::makePair(renderStage, stateBlockHash));
        return stateBlockHash;
    }

    it->second = stateBlockHash;
    return it->second;
}

//base = base texture
//second = second texture used for multitexturing
//bump = bump map
void Material::setTexture(ShaderProgram::TextureUsage textureUsageSlot, Texture* const texture, const TextureOperation& op) {
    bool computeShaders = false;

    if(_textures[textureUsageSlot]){
        UNREGISTER_TRACKED_DEPENDENCY(_textures[textureUsageSlot]);
        RemoveResource(_textures[textureUsageSlot]);
    }else{
        //if we add a new type of texture recompute shaders
        computeShaders = true;
    }

    _textures[textureUsageSlot] = texture;

    if ( texture ) {
        REGISTER_TRACKED_DEPENDENCY( _textures[textureUsageSlot] );
    }

    if ( textureUsageSlot == ShaderProgram::TEXTURE_UNIT1 ) {
        _operation = op;
    }
    if ( textureUsageSlot == ShaderProgram::TEXTURE_UNIT0 || textureUsageSlot == ShaderProgram::TEXTURE_UNIT1 ) {
        texture ? _shaderData._textureCount++ : _shaderData._textureCount--;
    }
    if ( textureUsageSlot == ShaderProgram::TEXTURE_UNIT0 || textureUsageSlot == ShaderProgram::TEXTURE_OPACITY ) {
        _translucencyCheck = false;
    }
   
    if ( computeShaders ) {
        recomputeShaders();
    }
    _dirty = true;
}

//Here we set the shader's name
void Material::setShaderProgram(const stringImpl& shader, 
                                const RenderStage& renderStage,
                                const bool computeOnAdd,
                                const DELEGATE_CBK<>& shaderCompileCallback) {
    _shaderInfo[renderStage]._isCustomShader = true;
    setShaderProgramInternal(shader, renderStage, computeOnAdd, shaderCompileCallback);
}

void Material::setShaderProgramInternal(const stringImpl& shader, 
                                        const RenderStage& renderStage, 
                                        const bool computeOnAdd, 
                                        const DELEGATE_CBK<>& shaderCompileCallback) {
    ShaderProgram* shaderReference = _shaderInfo[renderStage]._shaderRef;
    //if we already had a shader assigned ...
    if (!_shaderInfo[renderStage]._shader.empty()){
        //and we are trying to assign the same one again, return.
        shaderReference = FindResourceImpl<ShaderProgram>(_shaderInfo[renderStage]._shader);
        if (_shaderInfo[renderStage]._shader.compare(shader) != 0){
            Console::printfn(Locale::get("REPLACE_SHADER"), _shaderInfo[renderStage]._shader.c_str(), shader.c_str());
            UNREGISTER_TRACKED_DEPENDENCY( shaderReference );
            RemoveResource(shaderReference);
        }
    }

    (!shader.empty()) ? _shaderInfo[renderStage]._shader = shader : _shaderInfo[renderStage]._shader = "NULL_SHADER";
    
    ResourceDescriptor shaderDescriptor(_shaderInfo[renderStage]._shader);
    std::stringstream ss;
    if (!_shaderInfo[renderStage]._shaderDefines.empty()){
        for(stringImpl& shaderDefine : _shaderInfo[renderStage]._shaderDefines){
            ss << stringAlg::fromBase(shaderDefine);
            ss << ",";
        }
    }
    ss << "DEFINE_PLACEHOLDER";
    shaderDescriptor.setPropertyList(stringAlg::toBase(ss.str()));
    shaderDescriptor.setThreadedLoading(_shaderThreadedLoad);

    _computedShaderTextures = true;
    if (computeOnAdd){
        _shaderInfo[renderStage]._shaderRef = CreateResource<ShaderProgram>(shaderDescriptor);
        _shaderInfo[renderStage]._shaderCompStage = ShaderInfo::SHADER_STAGE_COMPUTED;
        REGISTER_TRACKED_DEPENDENCY(_shaderInfo[renderStage]._shaderRef); 
        if (shaderCompileCallback){
            shaderCompileCallback();
        }
    }else{
        _shaderComputeQueue.push(std::make_tuple(renderStage, shaderDescriptor, shaderCompileCallback));
        _shaderInfo[renderStage]._shaderCompStage = ShaderInfo::SHADER_STAGE_QUEUED;
    }
}

void Material::clean() {
    if(_dirty && _dumpToFile){
        isTranslucent();
        XML::dumpMaterial(*this);
       _dirty = false;
    }
}

void Material::recomputeShaders() {
    for (shaderInfoMap::value_type& it : _shaderInfo) {
        if ( !it.second._isCustomShader ) {
            it.second._shaderCompStage = ShaderInfo::SHADER_STAGE_REQUESTED;
            computeShader( it.first, false, DELEGATE_CBK<>() );
        }
    }
}

///If the current material doesn't have a shader associated with it, then add the default ones.
///Manually setting a shader, overrides this function by setting _computedShaders to "true"
bool Material::computeShader(const RenderStage& renderStage, 
                             const bool computeOnAdd, 
                             const DELEGATE_CBK<>& shaderCompileCallback) {
    if ( _shaderInfo[renderStage]._shaderCompStage == ShaderInfo::SHADER_STAGE_COMPUTED ) {
        return true;
    }

    if ( ( _textures[ShaderProgram::TEXTURE_UNIT0] && _textures[ShaderProgram::TEXTURE_UNIT0]->getState() != RES_LOADED ) ||
        ( _textures[ShaderProgram::TEXTURE_OPACITY] && _textures[ShaderProgram::TEXTURE_OPACITY]->getState() != RES_LOADED ) ) {
        return false;
    }

    DIVIDE_ASSERT(_shadingMode != ShadingMode_PLACEHOLDER, "Material computeShader error: Invalid shading mode specified!");

    bool deferredPassShader = GFX_DEVICE.getRenderer().getType() != RENDERER_FORWARD_PLUS;
    bool depthPassShader = renderStage == SHADOW_STAGE || renderStage == Z_PRE_PASS_STAGE;

    //the base shader is either for a Deferred Renderer or a Forward  one ...
    stringImpl shader = (deferredPassShader ? "DeferredShadingPass1" : (depthPassShader ? "depthPass" : "lighting"));

    if ( Config::Profile::DISABLE_SHADING ) {
        shader = "passThrough";
    }
    if ( depthPassShader ) {
        renderStage == Z_PRE_PASS_STAGE ? shader += ".PrePass" : shader += ".Shadow";
    }
    //What kind of effects do we need?
    if (_textures[ShaderProgram::TEXTURE_UNIT0]){
        //Bump mapping?
        if (_textures[ShaderProgram::TEXTURE_NORMALMAP] && _bumpMethod != BUMP_NONE){
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

    if (_textures[ShaderProgram::TEXTURE_SPECULAR]){
        shader += ".Specular";
        addShaderDefines(renderStage, "USE_SPECULAR_MAP");
    }

    if (isTranslucent()){
        for (Material::TranslucencySource source : _translucencySource){
            if(source == TRANSLUCENT_DIFFUSE ){
                shader += ".DiffuseAlpha";
                addShaderDefines(renderStage, "USE_OPACITY_DIFFUSE");
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
    if (_doubleSided){
        shader += ".DoubleSided";
        addShaderDefines(renderStage, "USE_DOUBLE_SIDED");
    }
    //Add the GPU skinning module to the vertex shader?
    if (_hardwareSkinning){
        addShaderDefines(renderStage, "USE_GPU_SKINNING");
        shader += ",Skinned"; //<Use "," instead of "." will add a Vertex only property
    }

    switch (_shadingMode) {
        default:
        case SHADING_FLAT: {
            addShaderDefines(renderStage, "USE_SHADING_FLAT");
            shader += ".Flat";
        }break;
        case SHADING_PHONG: {
            addShaderDefines(renderStage, "USE_SHADING_PHONG");
            shader += ".Phong";
        }break;
        case SHADING_BLINN_PHONG: {
            addShaderDefines(renderStage, "USE_SHADING_BLINN_PHONG");
            shader += ".BlinnPhong";
        }break;
        case SHADING_TOON: {
            addShaderDefines(renderStage, "USE_SHADING_TOON");
            shader += ".Toon";
        }break;
        case SHADING_OREN_NAYAR: {
            addShaderDefines(renderStage, "USE_SHADING_OREN_NAYAR");
            shader += ".OrenNayar";
        }break;
        case SHADING_COOK_TORRANCE: {
            addShaderDefines(renderStage, "USE_SHADING_COOK_TORRANCE");
            shader += ".CookTorrance";
        }break;
    }
    //Add any modifiers you wish
    if (!_shaderModifier.empty()){
        shader += ".";
        shader += _shaderModifier;
    }

    setShaderProgramInternal(shader, renderStage, computeOnAdd, shaderCompileCallback);

    return true;
}

void Material::computeShaderInternal(){
    if ( _shaderComputeQueue.empty()/* || Material::isShaderQueueLocked()*/ ) {
        return;
    }
    //Material::lockShaderQueue();

    const std::tuple<RenderStage, ResourceDescriptor, DELEGATE_CBK<>>& currentItem = _shaderComputeQueue.front();
    const RenderStage& renderStage = std::get<0>(currentItem);
    const ResourceDescriptor& descriptor = std::get<1>(currentItem);
    const DELEGATE_CBK<>& callback = std::get<2>(currentItem);
    _dirty = true;
    
    _shaderInfo[renderStage]._shaderRef = CreateResource<ShaderProgram>(descriptor);
    _shaderInfo[renderStage]._shaderCompStage = ShaderInfo::SHADER_STAGE_COMPUTED;
    if (callback){
        callback();
    }
    REGISTER_TRACKED_DEPENDENCY(_shaderInfo[renderStage]._shaderRef);
    _shaderComputeQueue.pop();
}

void Material::bindTextures(){
    if (_textures[ShaderProgram::TEXTURE_OPACITY]) {
        _textures[ShaderProgram::TEXTURE_OPACITY]->Bind(ShaderProgram::TEXTURE_OPACITY);
    }

    if (_textures[ShaderProgram::TEXTURE_UNIT0]) {
        _textures[ShaderProgram::TEXTURE_UNIT0]->Bind(ShaderProgram::TEXTURE_UNIT0);
    }

    if (!GFX_DEVICE.isCurrentRenderStage(DEPTH_STAGE)) {
        if (_textures[ShaderProgram::TEXTURE_NORMALMAP]) {
            _textures[ShaderProgram::TEXTURE_NORMALMAP]->Bind(ShaderProgram::TEXTURE_NORMALMAP);
        }

        if (_textures[ShaderProgram::TEXTURE_SPECULAR]) {
            _textures[ShaderProgram::TEXTURE_SPECULAR]->Bind(ShaderProgram::TEXTURE_SPECULAR);
        }

        if (_textures[ShaderProgram::TEXTURE_UNIT1]) {
            _textures[ShaderProgram::TEXTURE_UNIT1]->Bind(ShaderProgram::TEXTURE_UNIT1);
        }

        for (std::pair<Texture*, U32>& tex : _customTextures) {
            tex.first->Bind(tex.second);
        }
    }
}

ShaderProgram* const Material::ShaderInfo::getProgram() const {
    return _shaderRef == nullptr ? ShaderManager::getInstance().getDefaultShader() : _shaderRef;
}

Material::ShaderInfo& Material::getShaderInfo(RenderStage renderStage) {
    shaderInfoMap::iterator it = _shaderInfo.find(renderStage);

    return (it == std::end(_shaderInfo) ? _shaderInfo[FINAL_STAGE] : it->second);
}

void Material::setBumpMethod(const BumpMethod& newBumpMethod){
    _bumpMethod = newBumpMethod;
    recomputeShaders();
}

bool Material::unload(){
    for(Texture*& tex : _textures){
        if(tex != nullptr){
            UNREGISTER_TRACKED_DEPENDENCY(tex);
            RemoveResource(tex);
        }
    }
    _customTextures.clear();
    for (shaderInfoMap::value_type& it : _shaderInfo) {
        ShaderProgram* shader = FindResourceImpl<ShaderProgram>( it.second._shader );
        if ( shader != nullptr ) {
            UNREGISTER_TRACKED_DEPENDENCY( shader );
            RemoveResource( shader );
        }
    }
    _shaderInfo.clear();
    return true;
}

void Material::setDoubleSided(bool state, const bool useAlphaTest) {
    if ( _doubleSided == state && _useAlphaTest == useAlphaTest ) {
        return;
    }
    _doubleSided = state;
    _useAlphaTest = useAlphaTest;
    // Update all render states for this item
    if ( _doubleSided ) {
        for (renderStateBlockMap::value_type& it : _defaultRenderStates) {
            RenderStateBlockDescriptor descriptor( GFX_DEVICE.getStateBlockDescriptor( it.second ) );
            descriptor.setCullMode( CULL_MODE_NONE );
            if ( !_translucencySource.empty() ) {
                descriptor.setBlend( true );
            }
            setRenderStateBlock( descriptor, it.first );
        }
    }

    _dirty = true;
    recomputeShaders();
}

bool Material::isTranslucent() {
    if (!_translucencyCheck){
        _translucencySource.clear();
        bool useAlphaTest = false;
        // In order of importance (less to more)!
        // diffuse channel alpha
        if(_shaderData._diffuse.a < 0.95f) {
            _translucencySource.push_back(TRANSLUCENT_DIFFUSE);
            useAlphaTest = (_shaderData._diffuse.a < 0.15f);
        }

        // base texture is translucent
        if (_textures[ShaderProgram::TEXTURE_UNIT0] && _textures[ShaderProgram::TEXTURE_UNIT0]->hasTransparency()){
            _translucencySource.push_back(TRANSLUCENT_DIFFUSE_MAP);
            useAlphaTest = true;
        }

        // opacity map
        if(_textures[ShaderProgram::TEXTURE_OPACITY]){
            _translucencySource.push_back(TRANSLUCENT_OPACITY_MAP);
            useAlphaTest = false;
        }

         // Disable culling for translucent items
        if (!_translucencySource.empty()){
            setDoubleSided(true, useAlphaTest);
        }
        _translucencyCheck = true;
        recomputeShaders();
    }

    return !_translucencySource.empty();
}

void Material::getSortKeys(I32& shaderKey, I32& textureKey) const {
    shaderInfoMap::const_iterator it = _shaderInfo.find(FINAL_STAGE);

    shaderKey  = (it != std::end(_shaderInfo) && it->second._shaderRef) ? it->second._shaderRef->getId() :
                                                                          -std::numeric_limits<I8>::max();
    textureKey = _textures[ShaderProgram::TEXTURE_UNIT0] ? _textures[ShaderProgram::TEXTURE_UNIT0]->getHandle() : 
                                                           -std::numeric_limits<I8>::max();
}

};