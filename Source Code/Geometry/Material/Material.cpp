#include "Headers/Material.h"

#include "Rendering/Headers/Renderer.h"
#include "Utility/Headers/Localization.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"

#include "Core/Headers/Console.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

namespace {
#if defined(_DEBUG)
    const U32 g_MaxShadersComputedPerFrame = 2;
#else
    const U32 g_MaxShadersComputedPerFrame = 3;
#endif
    const U32 g_ReflectionResolution = 256;
};

bool Material::_shadersComputedThisFrame = false;
U32 Material::_totalShaderComputeCountThisFrame = 0;
U32 Material::_totalShaderComputeCount = 0;

Material::Material()
    : Resource("temp_material"),
      FrameListener(),
      _parallaxFactor(1.0f),
      _dirty(false),
      _doubleSided(false),
      _shaderThreadedLoad(true),
      _hardwareSkinning(false),
      _useAlphaTest(false),
      _dumpToFile(true),
      _translucencyCheck(true),
      _highPriority(false),
      _shadingMode(ShadingMode::COUNT),
      _bumpMethod(BumpMethod::NONE)
{
    REGISTER_FRAME_LISTENER(this, 9999);

    SamplerDescriptor screenSampler;
    screenSampler.setFilters(TextureFilter::NEAREST);
    screenSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    screenSampler.toggleMipMaps(false);
    TextureDescriptor environmentDescriptor(TextureType::TEXTURE_CUBE_MAP,
                                            GFXImageFormat::RGBA16F,
                                            GFXDataFormat::FLOAT_16);
    environmentDescriptor.setSampler(screenSampler);
    _reflectionTarget.reset(GFX_DEVICE.newFB(false));
    _reflectionTarget->addAttachment(environmentDescriptor, TextureDescriptor::AttachmentType::Color0);
    _reflectionTarget->useAutoDepthBuffer(true);
    _reflectionTarget->create(g_ReflectionResolution, g_ReflectionResolution);
    _reflectionTarget->setClearColor(DefaultColors::WHITE());

    _textures.resize(to_const_uint(ShaderProgram::TextureUsage::COUNT), nullptr);

    _operation = TextureOperation::REPLACE;

    /// Normal state for final rendering
    RenderStateBlock stateDescriptor;
    stateDescriptor.setZFunc(ComparisonFunction::LEQUAL);
    setRenderStateBlock(stateDescriptor.getHash(), RenderStage::DISPLAY);
    /// the reflection descriptor is the same as the normal descriptor
    RenderStateBlock reflectorDescriptor(stateDescriptor);
    setRenderStateBlock(reflectorDescriptor.getHash(), RenderStage::REFLECTION);
    /// the z-pre-pass descriptor does not process colors
    RenderStateBlock zPrePassDescriptor(stateDescriptor);
    zPrePassDescriptor.setColorWrites(true, true, true, false);
    zPrePassDescriptor.setZFunc(ComparisonFunction::LESS);
    setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStage::Z_PRE_PASS);
    /// A descriptor used for rendering to depth map
    RenderStateBlock shadowDescriptor(stateDescriptor);
    shadowDescriptor.setCullMode(CullMode::CCW);
    /// set a polygon offset
    shadowDescriptor.setZBias(1.0f, 2.0f);
    /// ignore half of the colors 
    /// Some shadowing techniques require drawing to the a color buffer
    shadowDescriptor.setColorWrites(true, true, false, false);
    setRenderStateBlock(shadowDescriptor.getHash(), RenderStage::SHADOW, 0);
    zPrePassDescriptor.setColorWrites(false, false, false, false);
    setRenderStateBlock(shadowDescriptor.getHash(), RenderStage::SHADOW, 1);
    setRenderStateBlock(shadowDescriptor.getHash(), RenderStage::SHADOW, 2);

    setTexture(ShaderProgram::TextureUsage::REFLECTION, _reflectionTarget->getAttachment());
}

Material::~Material()
{
    UNREGISTER_FRAME_LISTENER(this);
}

bool Material::frameStarted(const FrameEvent& evt) {
    _shadersComputedThisFrame = false;
    _totalShaderComputeCountThisFrame = 0;

    return true;
}

bool Material::frameEnded(const FrameEvent& evt) {
    if (_shadersComputedThisFrame) {
        _totalShaderComputeCount += _totalShaderComputeCountThisFrame;
    }

    return true;
}

Material* Material::clone(const stringImpl& nameSuffix) {
    DIVIDE_ASSERT(!nameSuffix.empty(),
                  "Material error: clone called without a valid name suffix!");

    const Material& base = *this;
    Material* cloneMat =
        CreateResource<Material>(ResourceDescriptor(getName() + nameSuffix));

    cloneMat->_shadingMode = base._shadingMode;
    cloneMat->_translucencyCheck = base._translucencyCheck;
    cloneMat->_dumpToFile = base._dumpToFile;
    cloneMat->_useAlphaTest = base._useAlphaTest;
    cloneMat->_doubleSided = base._doubleSided;
    cloneMat->_hardwareSkinning = base._hardwareSkinning;
    cloneMat->_shaderThreadedLoad = base._shaderThreadedLoad;
    cloneMat->_operation = base._operation;
    cloneMat->_bumpMethod = base._bumpMethod;
    cloneMat->_parallaxFactor = base._parallaxFactor;

    cloneMat->_translucencySource.clear();

    for (U8 i = 0; i < to_const_uint(RenderStage::COUNT); i++) {
        cloneMat->_shaderModifier[i] = base._shaderModifier[i];
        cloneMat->_shaderInfo[i] = _shaderInfo[i];
        for (U8 j = 0; j < _defaultRenderStates[i].size(); ++j) {
            cloneMat->_defaultRenderStates[i][j] = _defaultRenderStates[i][j];
        }
    }

    for (const TranslucencySource& trans : base._translucencySource) {
        cloneMat->_translucencySource.push_back(trans);
    }

    for (U8 i = 0; i < to_ubyte(base._textures.size()); ++i) {
        Texture* const tex = base._textures[i];
        if (tex) {
            tex->AddRef();
            cloneMat->setTexture(static_cast<ShaderProgram::TextureUsage>(i), tex);
        }
    }
    for (const std::pair<Texture*, U8>& tex : base._customTextures) {
        if (tex.first) {
            tex.first->AddRef();
            cloneMat->addCustomTexture(tex.first, tex.second);
        }
    }

    cloneMat->_shaderData = base._shaderData;

    return cloneMat;
}

void Material::update(const U64 deltaTime) {

    for (ShaderInfo& info : _shaderInfo) {
        if (info._shaderCompStage ==
            ShaderInfo::ShaderCompilationStage::PENDING) {
            if (info._shaderRef->getState() == ResourceState::RES_LOADED) {
                info._shaderCompStage =
                    ShaderInfo::ShaderCompilationStage::COMPUTED;
            }
        }
    }

    // build one shader per frame
    computeShaderInternal();

    for (ShaderInfo& info : _shaderInfo) {
        if (info._shaderCompStage ==
                ShaderInfo::ShaderCompilationStage::QUEUED ||
            info._shaderCompStage ==
                ShaderInfo::ShaderCompilationStage::REQUESTED ||
            info._shaderCompStage ==
                ShaderInfo::ShaderCompilationStage::PENDING) {
            return;
        }
    }

    clean();
}

U32 Material::getRenderStateBlock(RenderStage currentStage, I32 variant) {
    assert(variant >= 0 && variant < _defaultRenderStates[to_uint(currentStage)].size());
    return _defaultRenderStates[to_uint(currentStage)][variant];
}

// base = base texture
// second = second texture used for multitexturing
// bump = bump map
bool Material::setTexture(ShaderProgram::TextureUsage textureUsageSlot,
                          Texture* texture,
                          const TextureOperation& op) {
    bool computeShaders = false;
    U32 slot = to_uint(textureUsageSlot);

    if (textureUsageSlot == ShaderProgram::TextureUsage::UNIT1) {
        _operation = op;
    }

    if (texture && textureUsageSlot == ShaderProgram::TextureUsage::OPACITY) {
        Texture* diffuseMap = _textures[to_const_uint(ShaderProgram::TextureUsage::UNIT0)];
        if (diffuseMap && texture->getGUID() == diffuseMap->getGUID()) {
            /// If the opacity and diffuse map use the same texture, remove one reference
            RemoveResource(texture);
            return false;
        }

    }

    if (!_translucencyCheck) {
        _translucencyCheck =
            (textureUsageSlot == ShaderProgram::TextureUsage::UNIT0 ||
             textureUsageSlot == ShaderProgram::TextureUsage::OPACITY);
    }

    if (_textures[slot]) {
        UNREGISTER_TRACKED_DEPENDENCY(_textures[slot]);
        RemoveResource(_textures[slot]);
    } else {
        // if we add a new type of texture recompute shaders
        computeShaders = true;
    }

    _textures[slot] = texture;

    if (texture) {
        REGISTER_TRACKED_DEPENDENCY(_textures[slot]);
        // Environment maps ARE NOT OWNED by the material directly, but by the FBO
        if (slot == to_const_uint(ShaderProgram::TextureUsage::REFLECTION)) {
            texture->AddRef();
        }
    }

    if (computeShaders) {
        recomputeShaders();
    }
    _dirty = true;

    return true;
}

// Here we set the shader's name
void Material::setShaderProgram(const stringImpl& shader,
                                RenderStage renderStage,
                                const bool computeOnAdd) {
    _shaderInfo[to_uint(renderStage)]._customShader = true;
    setShaderProgramInternal(shader, renderStage, computeOnAdd);
}

void Material::setShaderProgramInternal(const stringImpl& shader,
                                        RenderStage renderStage,
                                        const bool computeOnAdd) {

    U32 stageIndex = to_uint(renderStage);
    ShaderInfo& info = _shaderInfo[stageIndex];
    // if we already had a shader assigned ...
    if (!info._shader.empty()) {
        // and we are trying to assign the same one again, return.
        info._shaderRef = FindResourceImpl<ShaderProgram>(info._shader);
        if (info._shader.compare(shader) != 0) {
            Console::printfn(Locale::get(_ID("REPLACE_SHADER")),
                             info._shader.c_str(), shader.c_str());
            UNREGISTER_TRACKED_DEPENDENCY(info._shaderRef);
            RemoveResource(info._shaderRef);
        }
    }

    (!shader.empty()) ? info._shader = shader : info._shader = "NULL";

    ResourceDescriptor shaderDescriptor(info._shader);
    std::stringstream ss;
    if (!info._shaderDefines.empty()) {
        for (stringImpl& shaderDefine : info._shaderDefines) {
            ss << shaderDefine;
            ss << ",";
        }
    }
    ss << "DEFINE_PLACEHOLDER";
    shaderDescriptor.setPropertyList(ss.str());
    shaderDescriptor.setThreadedLoading(_shaderThreadedLoad);

    ShaderQueueElement queueElement(stageIndex, shaderDescriptor);

    if (computeOnAdd) {
        _shaderComputeQueue.push_front(queueElement);
    } else {
        _shaderComputeQueue.push_back(queueElement);
    }
    
    info._shaderCompStage = ShaderInfo::ShaderCompilationStage::QUEUED;

    if (computeOnAdd) {
        computeShaderInternal();
    }
}

void Material::clean() {
    if (_dirty && _dumpToFile) {
        isTranslucent();
#if !defined(DEBUG)
        XML::dumpMaterial(*this);
#endif
        _dirty = false;
    }
}

void Material::recomputeShaders() {
    for (ShaderInfo& info : _shaderInfo) {
        if (!info._customShader) {
            info._shaderCompStage = ShaderInfo::ShaderCompilationStage::REQUESTED;
        }
    }
}

bool Material::canDraw(RenderStage renderStage) {
    for (U32 i = 0; i < to_const_uint(RenderStage::COUNT); ++i) {
        ShaderInfo& info = _shaderInfo[i];
        if (info._shaderCompStage != ShaderInfo::ShaderCompilationStage::COMPUTED) {
            computeShader(static_cast<RenderStage>(i), _highPriority);
            return false;
        }
    }

    return true;
}

/// If the current material doesn't have a shader associated with it, then add
/// the default ones.
bool Material::computeShader(RenderStage renderStage,
                             const bool computeOnAdd){

    ShaderInfo& info = _shaderInfo[to_uint(renderStage)];
    if (info._shaderCompStage ==
        ShaderInfo::ShaderCompilationStage::UNHANDLED) {
        info._shaderCompStage = ShaderInfo::ShaderCompilationStage::REQUESTED;
        return false;
    }

    if (info._shaderCompStage != ShaderInfo::ShaderCompilationStage::REQUESTED) {
        return info._shaderCompStage == ShaderInfo::ShaderCompilationStage::COMPUTED;
    }

    U32 slot0 = to_const_uint(ShaderProgram::TextureUsage::UNIT0);
    U32 slot1 = to_const_uint(ShaderProgram::TextureUsage::UNIT1);
    U32 slotOpacity = to_const_uint(ShaderProgram::TextureUsage::OPACITY);

    if ((_textures[slot0] &&
         _textures[slot0]->getState() != ResourceState::RES_LOADED) ||
        (_textures[slotOpacity] &&
         _textures[slotOpacity]->getState() != ResourceState::RES_LOADED)) {
        return false;
    }

    DIVIDE_ASSERT(
        _shadingMode != ShadingMode::COUNT,
        "Material computeShader error: Invalid shading mode specified!");

    info._shaderDefines.clear();

    if (_textures[slot0]) {
        _shaderData._textureCount = 1;
    }

    if (_textures[slot1]) {
        if (!_textures[slot0]) {
            std::swap(_textures[slot0], _textures[slot1]);
            _shaderData._textureCount = 1;
            _translucencyCheck = true;
        } else {
            _shaderData._textureCount = 2;
        }
    }

    bool deferredPassShader = SceneManager::instance().getRenderer().getType() !=
                              RendererType::RENDERER_FORWARD_PLUS;
    bool depthPassShader = renderStage == RenderStage::SHADOW ||
                           renderStage == RenderStage::Z_PRE_PASS;

    // the base shader is either for a Deferred Renderer or a Forward  one ...
    stringImpl shader =
        (deferredPassShader ? "DeferredShadingPass1"
                            : (depthPassShader ? "depthPass" : "lighting"));

    if (Config::Profile::DISABLE_SHADING) {
        shader = "passThrough";
        setShaderProgramInternal(shader, renderStage, computeOnAdd);
        return false;;
    }

    if (depthPassShader && renderStage == RenderStage::SHADOW) {
        setShaderDefines(renderStage, "SHADOW_PASS");
        shader += ".Shadow";
    }

    // What kind of effects do we need?
    if (_textures[slot0]) {
        // Bump mapping?
        if (_textures[to_const_uint(ShaderProgram::TextureUsage::NORMALMAP)] &&
            _bumpMethod != BumpMethod::NONE) {
            setShaderDefines(renderStage, "COMPUTE_TBN");
            shader += ".Bump";  // Normal Mapping
            if (_bumpMethod == BumpMethod::PARALLAX) {
                shader += ".Parallax";
                setShaderDefines(renderStage, "USE_PARALLAX_MAPPING");
            } else if (_bumpMethod == BumpMethod::RELIEF) {
                shader += ".Relief";
                setShaderDefines(renderStage, "USE_RELIEF_MAPPING");
            }
        } else {
            // Or simple texture mapping?
            shader += ".Texture";
        }
    } else {
        setShaderDefines(renderStage, "SKIP_TEXTURES");
        shader += ".NoTexture";
    }

    if (_textures[to_const_uint(ShaderProgram::TextureUsage::SPECULAR)]) {
        shader += ".Specular";
        setShaderDefines(renderStage, "USE_SPECULAR_MAP");
    }

    if (isTranslucent()) {
        bool diffuseOpacityDefined = false;
        for (Material::TranslucencySource source : _translucencySource) {
            if (source == TranslucencySource::OPACITY_MAP) {
                shader += ".OpacityMap";
                setShaderDefines(renderStage, "USE_OPACITY_MAP");
            }
            if (source == TranslucencySource::DIFFUSE ||
                source == TranslucencySource::DIFFUSE_MAP) {
                if (!diffuseOpacityDefined) {
                    shader += ".DiffuseAlpha";
                    setShaderDefines(renderStage, "USE_OPACITY_DIFFUSE");
                    diffuseOpacityDefined = true;
                }
            }
        }
    }

    if (_doubleSided) {
        shader += ".DoubleSided";
        setShaderDefines(renderStage, "USE_DOUBLE_SIDED");
    }

    // Add the GPU skinning module to the vertex shader?
    if (_hardwareSkinning) {
        setShaderDefines(renderStage, "USE_GPU_SKINNING");
        shader += ",Skinned";  //<Use "," instead of "." will add a Vertex only property
    }

    switch (_shadingMode) {
        default:
        case ShadingMode::FLAT: {
            setShaderDefines(renderStage, "USE_SHADING_FLAT");
            shader += ".Flat";
        } break;
        case ShadingMode::PHONG: {
            setShaderDefines(renderStage, "USE_SHADING_PHONG");
            shader += ".Phong";
        } break;
        case ShadingMode::BLINN_PHONG: {
            setShaderDefines(renderStage, "USE_SHADING_BLINN_PHONG");
            shader += ".BlinnPhong";
        } break;
        case ShadingMode::TOON: {
            setShaderDefines(renderStage, "USE_SHADING_TOON");
            shader += ".Toon";
        } break;
        case ShadingMode::OREN_NAYAR: {
            setShaderDefines(renderStage, "USE_SHADING_OREN_NAYAR");
            shader += ".OrenNayar";
        } break;
        case ShadingMode::COOK_TORRANCE: {
            setShaderDefines(renderStage, "USE_SHADING_COOK_TORRANCE");
            shader += ".CookTorrance";
        } break;
    }
    // Add any modifiers you wish
    if (!_shaderModifier[to_uint(renderStage)].empty()) {
        shader += ".";
        shader += _shaderModifier[to_uint(renderStage)];
    }

    setShaderProgramInternal(shader, renderStage, computeOnAdd);

    return false;
}

void Material::computeShaderInternal() {
    if (_shaderComputeQueue.empty() || _shadersComputedThisFrame) {
        return;
    }
    
    if (g_MaxShadersComputedPerFrame == ++_totalShaderComputeCountThisFrame) {
        _shadersComputedThisFrame = true;
    }

    const ShaderQueueElement& currentItem = _shaderComputeQueue.front();
    _dirty = true;

    ShaderInfo& info = _shaderInfo[std::get<0>(currentItem)];
    info._shaderRef = CreateResource<ShaderProgram>(std::get<1>(currentItem));
    bool shaderAvailable = info._shaderRef->getState() == ResourceState::RES_LOADED;
    info._shaderCompStage = shaderAvailable
                                ? ShaderInfo::ShaderCompilationStage::COMPUTED
                                : ShaderInfo::ShaderCompilationStage::PENDING;
    REGISTER_TRACKED_DEPENDENCY(info._shaderRef);

    _shaderComputeQueue.pop_front();
}

void Material::getTextureData(ShaderProgram::TextureUsage slot,
                              TextureDataContainer& container) {
    U32 slotValue = to_uint(slot);
    Texture* crtTexture = _textures[slotValue];
    if (crtTexture && crtTexture->flushTextureState()) {
        TextureData& data = crtTexture->getData();
        data.setHandleLow(slotValue);
        vectorAlg::emplace_back(container, data);
    }
}

void Material::getTextureData(TextureDataContainer& textureData) {
    U32 textureCount = to_const_uint(ShaderProgram::TextureUsage::COUNT);

    if (!GFX_DEVICE.isDepthStage()) {
        textureData.reserve(textureCount + _customTextures.size());
        getTextureData(ShaderProgram::TextureUsage::UNIT0, textureData);
        getTextureData(ShaderProgram::TextureUsage::UNIT1, textureData);
        getTextureData(ShaderProgram::TextureUsage::OPACITY, textureData);
        getTextureData(ShaderProgram::TextureUsage::NORMALMAP, textureData);
        getTextureData(ShaderProgram::TextureUsage::SPECULAR, textureData);
        getTextureData(ShaderProgram::TextureUsage::REFLECTION, textureData);
        
        for (std::pair<Texture*, U8>& tex : _customTextures) {
            if (tex.first->flushTextureState()) {
                textureData.push_back(tex.first->getData());
                textureData.back().setHandleLow(to_uint(tex.second));
            }
        }
    } else {
        textureData.reserve(2);
        getTextureData(ShaderProgram::TextureUsage::NORMALMAP, textureData);
        for (Material::TranslucencySource source : _translucencySource) {
            if (source == TranslucencySource::OPACITY_MAP) {
                getTextureData(ShaderProgram::TextureUsage::OPACITY, textureData);
            }
            if (source == TranslucencySource::DIFFUSE_MAP) {
                getTextureData(ShaderProgram::TextureUsage::UNIT0, textureData);
            }
        }
    }
}

ShaderProgram* const Material::ShaderInfo::getProgram() const {
    return _shaderRef == nullptr
               ? ShaderManager::instance().getDefaultShader()
               : _shaderRef;
}

Material::ShaderInfo& Material::getShaderInfo(RenderStage renderStage) {
    return _shaderInfo[to_uint(renderStage)];
}

void Material::setBumpMethod(const BumpMethod& newBumpMethod) {
    _bumpMethod = newBumpMethod;
    recomputeShaders();
}

void Material::setShadingMode(const ShadingMode& mode) { 
    _shadingMode = mode;
    recomputeShaders();
}

bool Material::unload() {
    for (Texture*& tex : _textures) {
        if (tex != nullptr) {
            UNREGISTER_TRACKED_DEPENDENCY(tex);
            RemoveResource(tex);
        }
    }
    _customTextures.clear();
    for (ShaderInfo& info : _shaderInfo) {
        ShaderProgram* shader = FindResourceImpl<ShaderProgram>(info._shader);
        if (shader != nullptr) {
            UNREGISTER_TRACKED_DEPENDENCY(shader);
            RemoveResource(shader);
        }
    }
    return true;
}

void Material::setDoubleSided(const bool state, const bool useAlphaTest) {
    if (_doubleSided == state && _useAlphaTest == useAlphaTest) {
        return;
    }
    _doubleSided = state;
    _useAlphaTest = useAlphaTest;
    // Update all render states for this item
    if (_doubleSided) {
        for (U32 index = 0; index < to_const_uint(RenderStage::COUNT); ++index) {
            for (U8 variant = 0; variant < _defaultRenderStates[index].size(); ++variant) {
                U32 hash = _defaultRenderStates[index][variant];
                RenderStateBlock descriptor(GFX_DEVICE.getRenderStateBlock(hash));
                descriptor.setCullMode(CullMode::NONE);
                if (!_translucencySource.empty()) {
                    descriptor.setBlend(true);
                }
                setRenderStateBlock(descriptor.getHash(), static_cast<RenderStage>(index), variant);
            }
        }
    }

    _dirty = true;
    recomputeShaders();
}

bool Material::isTranslucent() {
    if (_translucencyCheck) {
        _translucencySource.clear();
        bool useAlphaTest = false;
        // In order of importance (less to more)!
        // diffuse channel alpha
        if (_shaderData._diffuse.a < 0.95f) {
            _translucencySource.push_back(TranslucencySource::DIFFUSE);
            useAlphaTest = (_shaderData._diffuse.a < 0.15f);
        }

        // base texture is translucent
        if (_textures[to_const_uint(ShaderProgram::TextureUsage::UNIT0)] &&
            _textures[to_const_uint(ShaderProgram::TextureUsage::UNIT0)]
                ->hasTransparency()) {
            _translucencySource.push_back(TranslucencySource::DIFFUSE_MAP);
            useAlphaTest = true;
        }

        // opacity map
        if (_textures[to_const_uint(ShaderProgram::TextureUsage::OPACITY)]) {
            _translucencySource.push_back(TranslucencySource::OPACITY_MAP);
            useAlphaTest = false;
        }

        _translucencyCheck = false;

        // Disable culling for translucent items
        if (!_translucencySource.empty()) {
            setDoubleSided(true, useAlphaTest);
        } else {
            recomputeShaders();
        }
    }

    return !_translucencySource.empty();
}

void Material::getSortKeys(I32& shaderKey, I32& textureKey) const {
    static const I16 invalidShaderKey = -std::numeric_limits<I16>::max();

    const ShaderInfo& info = _shaderInfo[to_const_uint(RenderStage::DISPLAY)];

    shaderKey = info._shaderRef ? info._shaderRef->getID()
                                : invalidShaderKey;

    Texture* albedoTex = getTexture(ShaderProgram::TextureUsage::UNIT0);
    textureKey = albedoTex ? albedoTex->getHandle() : invalidShaderKey;
}

void Material::getMaterialMatrix(mat4<F32>& retMatrix) const {
    retMatrix.setRow(0, _shaderData._diffuse);
    retMatrix.setRow(1, _shaderData._specular);
    retMatrix.setRow(2, vec4<F32>(_shaderData._emissive.rgb(), _shaderData._shininess));
    retMatrix.setRow(3, vec4<F32>(isTranslucent() ? 1.0f : 0.0f,  to_float(getTextureOperation()), to_float(getTextureCount()), getParallaxFactor()));
}
};