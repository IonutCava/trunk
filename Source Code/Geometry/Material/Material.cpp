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

Material::Material()
    : Resource("temp_material"),
      _parallaxFactor(1.0f),
      _dirty(false),
      _doubleSided(false),
      _shaderThreadedLoad(true),
      _hardwareSkinning(false),
      _useAlphaTest(false),
      _dumpToFile(true),
      _translucencyCheck(false),
      _shadingMode(ShadingMode::COUNT),
      _bumpMethod(BumpMethod::NONE) {
    _textures.resize(to_uint(ShaderProgram::TextureUsage::COUNT), nullptr);

    _operation = TextureOperation::REPLACE;

    /// Normal state for final rendering
    RenderStateBlockDescriptor stateDescriptor;
    _defaultRenderStates[to_uint(RenderStage::DISPLAY_STAGE)] =
        GFX_DEVICE.getOrCreateStateBlock(stateDescriptor);
    /// the reflection descriptor is the same as the normal descriptor
    RenderStateBlockDescriptor reflectorDescriptor(stateDescriptor);
    _defaultRenderStates[to_uint(RenderStage::REFLECTION_STAGE)] =
        GFX_DEVICE.getOrCreateStateBlock(reflectorDescriptor);
    /// the z-pre-pass descriptor does not process colors
    RenderStateBlockDescriptor zPrePassDescriptor(stateDescriptor);
    zPrePassDescriptor.setColorWrites(false, false, false, false);
    _defaultRenderStates[to_uint(RenderStage::Z_PRE_PASS_STAGE)] =
        GFX_DEVICE.getOrCreateStateBlock(zPrePassDescriptor);
    /// A descriptor used for rendering to depth map
    RenderStateBlockDescriptor shadowDescriptor(stateDescriptor);
    shadowDescriptor.setCullMode(CullMode::CULL_MODE_CCW);
    /// set a polygon offset
    // shadowDescriptor.setZBias(1.0f, 2.0f);
    /// ignore colors - Some shadowing techniques require drawing to the a color
    /// buffer
    shadowDescriptor.setColorWrites(true, true, false, false);
    _defaultRenderStates[to_uint(RenderStage::SHADOW_STAGE)] =
        GFX_DEVICE.getOrCreateStateBlock(shadowDescriptor);
    _computedShaderTextures = false;
}

Material::~Material() {
}

Material* Material::clone(const stringImpl& nameSuffix) {
    DIVIDE_ASSERT(!nameSuffix.empty(),
                  "Material error: clone called without a valid name suffix!");

    const Material& base = *this;
    Material* cloneMat =
        CreateResource<Material>(ResourceDescriptor(getName() + nameSuffix));

    cloneMat->_shadingMode = base._shadingMode;
    cloneMat->_shaderModifier = base._shaderModifier;
    cloneMat->_translucencyCheck = base._translucencyCheck;
    cloneMat->_dumpToFile = base._dumpToFile;
    cloneMat->_useAlphaTest = base._useAlphaTest;
    cloneMat->_doubleSided = base._doubleSided;
    cloneMat->_hardwareSkinning = base._hardwareSkinning;
    cloneMat->_shaderThreadedLoad = base._shaderThreadedLoad;
    cloneMat->_computedShaderTextures = base._computedShaderTextures;
    cloneMat->_operation = base._operation;
    cloneMat->_bumpMethod = base._bumpMethod;
    cloneMat->_parallaxFactor = base._parallaxFactor;

    cloneMat->_translucencySource.clear();

    for (U8 i = 0; i < to_uint(RenderStage::COUNT); i++) {
        cloneMat->_shaderInfo[i] = _shaderInfo[i];
        cloneMat->_defaultRenderStates[i] = _defaultRenderStates[i];
    }

    for (const TranslucencySource& trans : base._translucencySource) {
        cloneMat->_translucencySource.push_back(trans);
    }

    for (U8 i = 0; i < static_cast<U8>(base._textures.size()); ++i) {
        Texture* const tex = base._textures[i];
        if (tex) {
            tex->AddRef();
            cloneMat->setTexture(static_cast<ShaderProgram::TextureUsage>(i),
                                 tex);
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
    // build one shader per frame
    computeShaderInternal();
    clean();
}

size_t Material::getRenderStateBlock(RenderStage currentStage) {
    return _defaultRenderStates[to_uint(currentStage)];
}

size_t Material::setRenderStateBlock(
    const RenderStateBlockDescriptor& descriptor,
    RenderStage renderStage) {
    size_t stateBlockHash = GFX_DEVICE.getOrCreateStateBlock(descriptor);
    _defaultRenderStates[to_uint(renderStage)] = stateBlockHash;

    return stateBlockHash;
}

// base = base texture
// second = second texture used for multitexturing
// bump = bump map
void Material::setTexture(ShaderProgram::TextureUsage textureUsageSlot,
                          Texture* const texture,
                          const TextureOperation& op) {
    bool computeShaders = false;
    U32 slot = to_uint(textureUsageSlot);
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
    }

    if (textureUsageSlot == ShaderProgram::TextureUsage::TEXTURE_UNIT1) {
        _operation = op;
    }

    _translucencyCheck =
        (textureUsageSlot == ShaderProgram::TextureUsage::TEXTURE_UNIT0 ||
         textureUsageSlot == ShaderProgram::TextureUsage::TEXTURE_OPACITY);

    if (computeShaders) {
        recomputeShaders();
    }
    _dirty = true;
}

// Here we set the shader's name
void Material::setShaderProgram(const stringImpl& shader,
                                RenderStage renderStage,
                                const bool computeOnAdd,
                                const DELEGATE_CBK<>& shaderCompileCallback) {
    _shaderInfo[to_uint(renderStage)]._shaderCompStage =
        ShaderInfo::ShaderCompilationStage::CUSTOM;
    setShaderProgramInternal(shader, renderStage, computeOnAdd,
                             shaderCompileCallback);
}

void Material::setShaderProgramInternal(
    const stringImpl& shader,
    RenderStage renderStage,
    const bool computeOnAdd,
    const DELEGATE_CBK<>& shaderCompileCallback) {
    U32 stageIndex = to_uint(renderStage);
    ShaderInfo& info = _shaderInfo[stageIndex];
    ShaderProgram* shaderReference = info._shaderRef;
    // if we already had a shader assigned ...
    if (!info._shader.empty()) {
        // and we are trying to assign the same one again, return.
        shaderReference = FindResourceImpl<ShaderProgram>(info._shader);
        if (info._shader.compare(shader) != 0) {
            Console::printfn(Locale::get("REPLACE_SHADER"),
                             info._shader.c_str(), shader.c_str());
            UNREGISTER_TRACKED_DEPENDENCY(shaderReference);
            RemoveResource(shaderReference);
        }
    }

    (!shader.empty()) ? info._shader = shader : info._shader = "NULL_SHADER";

    ResourceDescriptor shaderDescriptor(info._shader);
    std::stringstream ss;
    if (!info._shaderDefines.empty()) {
        for (stringImpl& shaderDefine : info._shaderDefines) {
            ss << stringAlg::fromBase(shaderDefine);
            ss << ",";
        }
    }
    ss << "DEFINE_PLACEHOLDER";
    shaderDescriptor.setPropertyList(stringAlg::toBase(ss.str()));
    shaderDescriptor.setThreadedLoading(_shaderThreadedLoad);

    _computedShaderTextures = true;
    if (computeOnAdd) {
        info._shaderRef = CreateResource<ShaderProgram>(shaderDescriptor);
        info._shaderCompStage = ShaderInfo::ShaderCompilationStage::PENDING;
        REGISTER_TRACKED_DEPENDENCY(info._shaderRef);
        if (shaderCompileCallback) {
            shaderCompileCallback();
        }
    } else {
        _shaderComputeQueue.push(std::make_tuple(stageIndex, shaderDescriptor,
                                                 shaderCompileCallback));
        info._shaderCompStage = ShaderInfo::ShaderCompilationStage::QUEUED;
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
        if (info._shaderCompStage !=
            ShaderInfo::ShaderCompilationStage::CUSTOM) {
            info._shaderCompStage =
                ShaderInfo::ShaderCompilationStage::REQUESTED;
        }
    }
}

/// If the current material doesn't have a shader associated with it, then add
/// the default ones.
bool Material::computeShader(RenderStage renderStage,
                             const bool computeOnAdd,
                             const DELEGATE_CBK<>& shaderCompileCallback) {
    ShaderInfo& info = _shaderInfo[to_uint(renderStage)];
    if (info._shaderCompStage !=
        ShaderInfo::ShaderCompilationStage::REQUESTED) {
        return info._shaderCompStage ==
               ShaderInfo::ShaderCompilationStage::COMPUTED;
    }

    U32 slot0 = to_uint(ShaderProgram::TextureUsage::TEXTURE_UNIT0);
    U32 slot1 = to_uint(ShaderProgram::TextureUsage::TEXTURE_UNIT1);
    U32 slotOpacity = to_uint(ShaderProgram::TextureUsage::TEXTURE_OPACITY);

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

    bool deferredPassShader = GFX_DEVICE.getRenderer().getType() !=
                              RendererType::RENDERER_FORWARD_PLUS;
    bool depthPassShader = renderStage == RenderStage::SHADOW_STAGE ||
                           renderStage == RenderStage::Z_PRE_PASS_STAGE;

    // the base shader is either for a Deferred Renderer or a Forward  one ...
    stringImpl shader =
        (deferredPassShader ? "DeferredShadingPass1"
                            : (depthPassShader ? "depthPass" : "lighting"));

    if (Config::Profile::DISABLE_SHADING) {
        shader = "passThrough";
    }
    if (depthPassShader) {
        renderStage == RenderStage::Z_PRE_PASS_STAGE ? shader += ".PrePass"
                                                     : shader += ".Shadow";
    }
    // What kind of effects do we need?
    if (_textures[slot0]) {
        // Bump mapping?
        if (_textures[to_uint(
                ShaderProgram::TextureUsage::TEXTURE_NORMALMAP)] &&
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

    if (_textures[to_uint(ShaderProgram::TextureUsage::TEXTURE_SPECULAR)]) {
        shader += ".Specular";
        setShaderDefines(renderStage, "USE_SPECULAR_MAP");
    }

    if (isTranslucent()) {
        for (Material::TranslucencySource source : _translucencySource) {
            if (source == TranslucencySource::DIFFUSE) {
                shader += ".DiffuseAlpha";
                setShaderDefines(renderStage, "USE_OPACITY_DIFFUSE");
            }
            if (source == TranslucencySource::OPACITY_MAP) {
                shader += ".OpacityMap";
                setShaderDefines(renderStage, "USE_OPACITY_MAP");
            }
            if (source == TranslucencySource::DIFFUSE_MAP) {
                shader += ".TextureAlpha";
                setShaderDefines(renderStage, "USE_OPACITY_DIFFUSE_MAP");
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
        shader += ",Skinned";  //<Use "," instead of "." will add a Vertex only
        // property
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
    if (!_shaderModifier.empty()) {
        shader += ".";
        shader += _shaderModifier;
    }

    setShaderProgramInternal(shader, renderStage, computeOnAdd,
                             shaderCompileCallback);

    return true;
}

void Material::computeShaderInternal() {
    for (ShaderInfo& info : _shaderInfo) {
        if (info._shaderCompStage ==
            ShaderInfo::ShaderCompilationStage::PENDING) {
            info._shaderCompStage =
                ShaderInfo::ShaderCompilationStage::COMPUTED;
        }
    }

    if (_shaderComputeQueue.empty() /* || Material::isShaderQueueLocked()*/) {
        return;
    }
    // Material::lockShaderQueue();

    const std::tuple<U32, ResourceDescriptor, DELEGATE_CBK<>>& currentItem =
        _shaderComputeQueue.front();
    const U32& renderStageIndex = std::get<0>(currentItem);
    const ResourceDescriptor& descriptor = std::get<1>(currentItem);
    const DELEGATE_CBK<>& callback = std::get<2>(currentItem);
    _dirty = true;

    ShaderInfo& info = _shaderInfo[renderStageIndex];
    info._shaderRef = CreateResource<ShaderProgram>(descriptor);
    info._shaderCompStage = ShaderInfo::ShaderCompilationStage::COMPUTED;
    if (callback) {
        callback();
    }
    REGISTER_TRACKED_DEPENDENCY(info._shaderRef);
    _shaderComputeQueue.pop();
}

void Material::getTextureData(ShaderProgram::TextureUsage slot,
                              TextureDataContainer& container) {
    U8 slotValue = to_uint(slot);
    Texture* crtTexture = _textures[slotValue];
    if (crtTexture) {
        TextureData data = crtTexture->getData();
        data.setHandleLow(static_cast<U32>(slotValue));
        container.push_back(data);
    }
}

void Material::getTextureData(TextureDataContainer& textureData) {
    textureData.reserve(to_uint(ShaderProgram::TextureUsage::COUNT) +
                        _customTextures.size());
    getTextureData(ShaderProgram::TextureUsage::TEXTURE_OPACITY, textureData);
    getTextureData(ShaderProgram::TextureUsage::TEXTURE_UNIT0, textureData);

    RenderStage currentStage = GFX_DEVICE.getRenderStage();
    if (!GFX_DEVICE.isDepthStage()) {
        getTextureData(ShaderProgram::TextureUsage::TEXTURE_UNIT1, textureData);
        getTextureData(ShaderProgram::TextureUsage::TEXTURE_NORMALMAP,
                       textureData);
        getTextureData(ShaderProgram::TextureUsage::TEXTURE_SPECULAR,
                       textureData);

        for (std::pair<Texture*, U8>& tex : _customTextures) {
            TextureData data = tex.first->getData();
            data.setHandleLow(static_cast<U32>(tex.second));
            textureData.push_back(data);
        }
    }
}

ShaderProgram* const Material::ShaderInfo::getProgram() const {
    return _shaderRef == nullptr
               ? ShaderManager::getInstance().getDefaultShader()
               : _shaderRef;
}

Material::ShaderInfo& Material::getShaderInfo(RenderStage renderStage) {
    return _shaderInfo[to_uint(renderStage)];
}

void Material::setBumpMethod(const BumpMethod& newBumpMethod) {
    _bumpMethod = newBumpMethod;
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

void Material::setDoubleSided(bool state, const bool useAlphaTest) {
    if (_doubleSided == state && _useAlphaTest == useAlphaTest) {
        return;
    }
    _doubleSided = state;
    _useAlphaTest = useAlphaTest;
    // Update all render states for this item
    if (_doubleSided) {
        for (U32 index = 0; index < to_uint(RenderStage::COUNT); index++) {
            size_t hash = _defaultRenderStates[index];
            RenderStateBlockDescriptor descriptor(
                GFX_DEVICE.getStateBlockDescriptor(hash));
            descriptor.setCullMode(CullMode::CULL_MODE_NONE);
            if (!_translucencySource.empty()) {
                descriptor.setBlend(true);
            }
            setRenderStateBlock(descriptor, static_cast<RenderStage>(index));
        }
    }

    _dirty = true;
    recomputeShaders();
}

bool Material::isTranslucent() {
    if (!_translucencyCheck) {
        _translucencySource.clear();
        bool useAlphaTest = false;
        // In order of importance (less to more)!
        // diffuse channel alpha
        if (_shaderData._diffuse.a < 0.95f) {
            _translucencySource.push_back(TranslucencySource::DIFFUSE);
            useAlphaTest = (_shaderData._diffuse.a < 0.15f);
        }

        // base texture is translucent
        if (_textures[to_uint(ShaderProgram::TextureUsage::TEXTURE_UNIT0)] &&
            _textures[to_uint(ShaderProgram::TextureUsage::TEXTURE_UNIT0)]
                ->hasTransparency()) {
            _translucencySource.push_back(TranslucencySource::DIFFUSE_MAP);
            useAlphaTest = true;
        }

        // opacity map
        if (_textures[to_uint(ShaderProgram::TextureUsage::TEXTURE_OPACITY)]) {
            _translucencySource.push_back(TranslucencySource::OPACITY_MAP);
            useAlphaTest = false;
        }

        // Disable culling for translucent items
        if (!_translucencySource.empty()) {
            setDoubleSided(true, useAlphaTest);
        }
        _translucencyCheck = true;
        recomputeShaders();
    }

    return !_translucencySource.empty();
}

void Material::getSortKeys(I32& shaderKey, I32& textureKey) const {
    const ShaderInfo& info = _shaderInfo[to_uint(RenderStage::DISPLAY_STAGE)];

    shaderKey = info._shaderRef ? info._shaderRef->getID()
                                : -std::numeric_limits<I8>::max();

    U32 textureSlot = to_uint(ShaderProgram::TextureUsage::TEXTURE_UNIT0);
    textureKey = _textures[textureSlot] ? _textures[textureSlot]->getHandle()
                                        : -std::numeric_limits<I8>::max();
}
};