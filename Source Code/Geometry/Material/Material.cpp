#include "config.h"

#include "Headers/Material.h"
#include "Headers/ShaderComputeQueue.h"

#include "Rendering/Headers/Renderer.h"
#include "Utility/Headers/Localization.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"

#include "Core/Headers/Console.h"

namespace Divide {

namespace {
    const char* g_DepthPassMaterialShaderName = "depthPass";
    const char* g_ForwardMaterialShaderName = "material";
    const char* g_DeferredMaterialShaderName = "DeferredShadingPass1";
    const char* g_PassThroughMaterialShaderName = "passThrough";
};

Material::Material(const stringImpl& name)
    : Resource(name),
      _parallaxFactor(1.0f),
      _dirty(false),
      _doubleSided(false),
      _shaderThreadedLoad(true),
      _hardwareSkinning(false),
      _useAlphaTest(false),
      _dumpToFile(true),
      _translucencyCheck(true),
      _highPriority(false),
      _reflectionIndex(-1),
      _refractionIndex(-1),
      _shadingMode(ShadingMode::COUNT),
      _bumpMethod(BumpMethod::NONE),
      _translucencySource(TranslucencySource::COUNT)
{
    _textures.fill(nullptr);
    _textureExtenalFlag.fill(false);
    _textureExtenalFlag[to_const_uint(ShaderProgram::TextureUsage::REFLECTION)] = true;
    _textureExtenalFlag[to_const_uint(ShaderProgram::TextureUsage::REFRACTION)] = true;
    defaultReflectionTexture(nullptr, 0);
    defaultRefractionTexture(nullptr, 0);

    _operation = TextureOperation::NONE;

    /// Normal state for final rendering
    RenderStateBlock stateDescriptor;
    stateDescriptor.setZFunc(ComparisonFunction::LEQUAL);
    setRenderStateBlock(stateDescriptor.getHash(), RenderStage::DISPLAY);
    setRenderStateBlock(stateDescriptor.getHash(), RenderStage::REFRACTION);
    /// the reflection descriptor is the same as the normal descriptor
    RenderStateBlock reflectorDescriptor(stateDescriptor);
    setRenderStateBlock(reflectorDescriptor.getHash(), RenderStage::REFLECTION);
    /// the z-pre-pass descriptor does not process colours
    RenderStateBlock zPrePassDescriptor(stateDescriptor);
    zPrePassDescriptor.setColourWrites(true, true, true, false);
    zPrePassDescriptor.setZFunc(ComparisonFunction::LESS);
    setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStage::Z_PRE_PASS);
    /// A descriptor used for rendering to depth map
    RenderStateBlock shadowDescriptor(stateDescriptor);
    shadowDescriptor.setCullMode(CullMode::CCW);
    /// set a polygon offset
    shadowDescriptor.setZBias(1.0f, 2.0f);
    /// ignore half of the colours 
    /// Some shadowing techniques require drawing to the a colour buffer
    shadowDescriptor.setColourWrites(true, true, false, false);
    setRenderStateBlock(shadowDescriptor.getHash(), RenderStage::SHADOW, 0);
    zPrePassDescriptor.setColourWrites(false, false, false, false);
    setRenderStateBlock(shadowDescriptor.getHash(), RenderStage::SHADOW, 1);
    setRenderStateBlock(shadowDescriptor.getHash(), RenderStage::SHADOW, 2);
}

Material::~Material()
{
}


Material_ptr Material::clone(const stringImpl& nameSuffix) {
    DIVIDE_ASSERT(!nameSuffix.empty(),
                  "Material error: clone called without a valid name suffix!");

    const Material& base = *this;
    Material_ptr cloneMat = CreateResource<Material>(ResourceDescriptor(getName() + nameSuffix));

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
    cloneMat->_reflectionIndex = base._reflectionIndex;
    cloneMat->_refractionIndex = base._refractionIndex;
    cloneMat->_defaultReflection = base._defaultReflection;
    cloneMat->_defaultRefraction = base._defaultRefraction;
    cloneMat->_translucencySource = base._translucencySource;

    for (U8 i = 0; i < to_const_uint(RenderStage::COUNT); i++) {
        cloneMat->_shaderModifier[i] = base._shaderModifier[i];
        cloneMat->_shaderInfo[i] = _shaderInfo[i];
        for (U8 j = 0; j < _defaultRenderStates[i].size(); ++j) {
            cloneMat->_defaultRenderStates[i][j] = _defaultRenderStates[i][j];
        }
    }
    
    for (U8 i = 0; i < to_ubyte(base._textures.size()); ++i) {
        ShaderProgram::TextureUsage usage = static_cast<ShaderProgram::TextureUsage>(i);
        if (!isExternalTexture(usage)) {
            Texture_ptr tex = base._textures[i];
            if (tex) {
                cloneMat->setTexture(usage, tex);
            }
        }
    }
    for (const std::pair<Texture_ptr, U8>& tex : base._customTextures) {
        if (tex.first) {
            cloneMat->addCustomTexture(tex.first, tex.second);
        }
    }

    cloneMat->_colourData = base._colourData;

    return cloneMat;
}

void Material::update(const U64 deltaTime) {
    for (ShaderProgramInfo& info : _shaderInfo) {
        if (info._shaderCompStage == ShaderProgramInfo::BuildStage::COMPUTED) {
            if (info._shaderRef->getState() == ResourceState::RES_LOADED) {
                info._shaderCompStage = ShaderProgramInfo::BuildStage::READY;
                _dirty = true;
            }
        }
    }

    clean();
}

size_t Material::getRenderStateBlock(RenderStage currentStage, I32 variant) {
    assert(variant >= 0 && variant < _defaultRenderStates[to_uint(currentStage)].size());
    return _defaultRenderStates[to_uint(currentStage)][variant];
}

// base = base texture
// second = second texture used for multitexturing
// bump = bump map
bool Material::setTexture(ShaderProgram::TextureUsage textureUsageSlot,
                          const Texture_ptr& texture,
                          const TextureOperation& op) {
    bool computeShaders = false;
    U32 slot = to_uint(textureUsageSlot);

    if (textureUsageSlot == ShaderProgram::TextureUsage::UNIT1) {
        _operation = op;
    }

    if (texture && textureUsageSlot == ShaderProgram::TextureUsage::OPACITY) {
        Texture_ptr& diffuseMap = _textures[to_const_uint(ShaderProgram::TextureUsage::UNIT0)];
        if (diffuseMap && texture->getGUID() == diffuseMap->getGUID()) {
            return false;
        }
    }

    if (!_translucencyCheck) {
         _translucencyCheck =
            (textureUsageSlot == ShaderProgram::TextureUsage::UNIT0 ||
             textureUsageSlot == ShaderProgram::TextureUsage::OPACITY);
    }

    if (!_textures[slot]) {
        if (textureUsageSlot != ShaderProgram::TextureUsage::REFLECTION &&
            textureUsageSlot != ShaderProgram::TextureUsage::REFRACTION) {
            // if we add a new type of texture recompute shaders
            computeShaders = true;
        }
    }

    _textures[slot] = texture;

    if (computeShaders) {
        recomputeShaders();
    }

    _dirty = textureUsageSlot != ShaderProgram::TextureUsage::REFLECTION &&
             textureUsageSlot != ShaderProgram::TextureUsage::REFRACTION;

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
    ShaderProgramInfo& info = _shaderInfo[stageIndex];
    // if we already had a shader assigned ...
    if (!info._shader.empty()) {
        // and we are trying to assign the same one again, return.
        info._shaderRef = FindResourceImpl<ShaderProgram>(info._shader);
        if (info._shader.compare(shader) != 0) {
            Console::printfn(Locale::get(_ID("REPLACE_SHADER")), info._shader.c_str(), shader.c_str());
        }
    }

    (!shader.empty()) ? info._shader = shader : info._shader = "NULL";

    ResourceDescriptor shaderDescriptor(info._shader);
    stringstreamImpl ss;
    if (!info._shaderDefines.empty()) {
        for (stringImpl& shaderDefine : info._shaderDefines) {
            ss << shaderDefine;
            ss << ",";
        }
    }
    ss << "DEFINE_PLACEHOLDER";
    shaderDescriptor.setPropertyList(ss.str());
    shaderDescriptor.setThreadedLoading(_shaderThreadedLoad);

    ShaderComputeQueue::ShaderQueueElement queueElement;
    queueElement._shaderData = &_shaderInfo[stageIndex];
    queueElement._shaderDescriptor = shaderDescriptor;
    
    ShaderComputeQueue& shaderQueue = SceneManager::instance().shaderComputeQueue();
    if (computeOnAdd) {
        shaderQueue.addToQueueFront(queueElement);
        shaderQueue.stepQueue();
    } else {
        shaderQueue.addToQueueBack(queueElement);
    }
    
    info._shaderCompStage = ShaderProgramInfo::BuildStage::QUEUED;
}

void Material::clean() {
    if (_dirty && _dumpToFile) {
        updateTranslucency();
#if !defined(DEBUG)
        XML::dumpMaterial(*this);
#endif
        _dirty = false;
    }
}

void Material::recomputeShaders() {
    for (ShaderProgramInfo& info : _shaderInfo) {
        if (!info._customShader) {
            info._shaderCompStage = ShaderProgramInfo::BuildStage::REQUESTED;
        }
    }
}

bool Material::canDraw(RenderStage renderStage) {
    for (U32 i = 0; i < to_const_uint(RenderStage::COUNT); ++i) {
        ShaderProgramInfo& info = _shaderInfo[i];
        if (info._shaderCompStage != ShaderProgramInfo::BuildStage::READY) {
            computeShader(static_cast<RenderStage>(i), _highPriority);
            return false;
        }
    }

    return true;
}

void Material::updateReflectionIndex(I32 index) {
    _reflectionIndex = index;
    if (_reflectionIndex > -1) {
        RenderTarget& reflectionTarget = GFX_DEVICE.renderTarget(RenderTargetID::REFLECTION, index);
        const Texture_ptr& refTex = reflectionTarget.getAttachment(RTAttachment::Type::Colour, 0).asTexture();
        setTexture(ShaderProgram::TextureUsage::REFLECTION, refTex);
    } else {
        setTexture(ShaderProgram::TextureUsage::REFLECTION, _defaultReflection.first);
    }
}

void Material::updateRefractionIndex(I32 index) {
    _refractionIndex = index;
    if (_refractionIndex > -1) {
        RenderTarget& refractionTarget = GFX_DEVICE.renderTarget(RenderTargetID::REFRACTION, index);
        const Texture_ptr& refTex = refractionTarget.getAttachment(RTAttachment::Type::Colour, 0).asTexture();
        setTexture(ShaderProgram::TextureUsage::REFRACTION, refTex);
    } else {
        setTexture(ShaderProgram::TextureUsage::REFRACTION, _defaultRefraction.first);
    }
}


void Material::defaultReflectionTexture(const Texture_ptr& reflectionPtr, U32 arrayIndex) {
    _defaultReflection.first = reflectionPtr;
    _defaultReflection.second = arrayIndex;
}

void Material::defaultRefractionTexture(const Texture_ptr& refractionPtr, U32 arrayIndex) {
    _defaultRefraction.first = refractionPtr;
    _defaultRefraction.second = arrayIndex;
}

/// If the current material doesn't have a shader associated with it, then add
/// the default ones.
bool Material::computeShader(RenderStage renderStage, const bool computeOnAdd){

    ShaderProgramInfo& info = _shaderInfo[to_uint(renderStage)];
    // If shader's invalid, try to request a recompute as it might fix it
    if (info._shaderCompStage == ShaderProgramInfo::BuildStage::COUNT) {
        info._shaderCompStage = ShaderProgramInfo::BuildStage::REQUESTED;
        return false;
    }

    // If the shader is valid and a recomput wasn't requested, just return true
    if (info._shaderCompStage != ShaderProgramInfo::BuildStage::REQUESTED) {
        return info._shaderCompStage == ShaderProgramInfo::BuildStage::READY;
    }

    // At this point, only computation requests are processed
    assert(info._shaderCompStage == ShaderProgramInfo::BuildStage::REQUESTED);

    const U32 slot0 = to_const_uint(ShaderProgram::TextureUsage::UNIT0);
    const U32 slot1 = to_const_uint(ShaderProgram::TextureUsage::UNIT1);
    const U32 slotOpacity = to_const_uint(ShaderProgram::TextureUsage::OPACITY);

    if ((_textures[slot0] && _textures[slot0]->getState() != ResourceState::RES_LOADED) ||
        (_textures[slotOpacity] && _textures[slotOpacity]->getState() != ResourceState::RES_LOADED)) {
        return false;
    }

    DIVIDE_ASSERT(_shadingMode != ShadingMode::COUNT,
                  "Material computeShader error: Invalid shading mode specified!");

    info._shaderDefines.clear();

    if (_textures[slot1]) {
        if (!_textures[slot0]) {
            std::swap(_textures[slot0], _textures[slot1]);
            _translucencyCheck = true;
        }
    }

    bool deferredPassShader = SceneManager::instance().getRenderer().getType() !=
                              RendererType::RENDERER_FORWARD_PLUS;
    bool depthPassShader = renderStage == RenderStage::SHADOW ||
                           renderStage == RenderStage::Z_PRE_PASS;

    // the base shader is either for a Deferred Renderer or a Forward  one ...
    stringImpl shader =
        (deferredPassShader ? g_DeferredMaterialShaderName
                            : (depthPassShader ? g_DepthPassMaterialShaderName : g_ForwardMaterialShaderName));

    if (Config::Profile::DISABLE_SHADING) {
        shader = g_PassThroughMaterialShaderName;
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
        if (_textures[to_const_uint(ShaderProgram::TextureUsage::NORMALMAP)] &&  _bumpMethod != BumpMethod::NONE) {
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

    if (updateTranslucency()) {
        switch (_translucencySource) {
            case TranslucencySource::OPACITY_MAP: {
                shader += ".OpacityMap";
                setShaderDefines(renderStage, "USE_OPACITY_MAP");
            } break;
            case TranslucencySource::DIFFUSE: {
                shader += ".DiffuseAlpha";
                setShaderDefines(renderStage, "USE_OPACITY_DIFFUSE");
            } break;
            case TranslucencySource::DIFFUSE_MAP: {
                shader += ".DiffuseMapAlpha";
                setShaderDefines(renderStage, "USE_OPACITY_DIFFUSE_MAP");
            } break;
        };
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

/// Add a texture <-> bind slot pair to be bound with the default textures
/// on each "bindTexture" call
void Material::addCustomTexture(const Texture_ptr& texture, U8 offset) {
    // custom textures are not material dependencies!
    _customTextures.push_back(std::make_pair(texture, offset));
}

/// Remove the custom texture assigned to the specified offset
bool Material::removeCustomTexture(U8 index) {
    vectorImpl<std::pair<Texture_ptr, U8>>::iterator it =
        std::find_if(std::begin(_customTextures), std::end(_customTextures),
            [&index](const std::pair<Texture_ptr, U8>& tex)
            -> bool { return tex.second == index; });
    if (it == std::end(_customTextures)) {
        return false;
    }

    _customTextures.erase(it);

    return true;
}

void Material::getTextureData(ShaderProgram::TextureUsage slot,
                              TextureDataContainer& container) {
    U32 slotValue = to_uint(slot);
    Texture_ptr& crtTexture = _textures[slotValue];
    if (crtTexture && crtTexture->flushTextureState()) {
        TextureData& data = crtTexture->getData();
        data.setHandleLow(slotValue);
        vectorAlg::emplace_back(container, data);
    }
}

void Material::getTextureData(TextureDataContainer& textureData) {
    const U32 textureCount = to_const_uint(ShaderProgram::TextureUsage::COUNT);

    if (!GFX_DEVICE.isDepthStage()) {
        textureData.reserve(textureCount + _customTextures.size());
        getTextureData(ShaderProgram::TextureUsage::UNIT0, textureData);
        getTextureData(ShaderProgram::TextureUsage::UNIT1, textureData);
        getTextureData(ShaderProgram::TextureUsage::OPACITY, textureData);
        getTextureData(ShaderProgram::TextureUsage::NORMALMAP, textureData);
        getTextureData(ShaderProgram::TextureUsage::SPECULAR, textureData);
        getTextureData(ShaderProgram::TextureUsage::REFLECTION, textureData);
        getTextureData(ShaderProgram::TextureUsage::REFRACTION, textureData);

        for (std::pair<Texture_ptr, U8>& tex : _customTextures) {
            if (tex.first->flushTextureState()) {
                textureData.push_back(tex.first->getData());
                textureData.back().setHandleLow(to_uint(tex.second));
            }
        }
    } else {
        textureData.reserve(2);
        getTextureData(ShaderProgram::TextureUsage::NORMALMAP, textureData);
        switch(_translucencySource) {
            case TranslucencySource::OPACITY_MAP : {
                getTextureData(ShaderProgram::TextureUsage::OPACITY, textureData);
            } break;
            case TranslucencySource::DIFFUSE_MAP: {
                getTextureData(ShaderProgram::TextureUsage::UNIT0, textureData);
            } break;
        };
    }
}

ShaderProgramInfo& Material::getShaderInfo(RenderStage renderStage) {
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

    _textures.fill(nullptr);
    _customTextures.clear();
    _shaderInfo.fill(ShaderProgramInfo());

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
                size_t hash = _defaultRenderStates[index][variant];
                RenderStateBlock descriptor(GFX_DEVICE.getRenderStateBlock(hash));
                descriptor.setCullMode(CullMode::NONE);
                if (_translucencySource != TranslucencySource::COUNT) {
                    descriptor.setBlend(true);
                }
                setRenderStateBlock(descriptor.getHash(), static_cast<RenderStage>(index), variant);
            }
        }
    }

    _dirty = true;
    recomputeShaders();
}

bool Material::updateTranslucency() {
    if (_translucencyCheck) {
        bool useAlphaTest = false;
        _translucencySource = TranslucencySource::COUNT;
        // In order of importance (less to more)!
        // diffuse channel alpha
        if (_colourData._diffuse.a < 0.95f) {
            _translucencySource = TranslucencySource::DIFFUSE;
            useAlphaTest = (_colourData._diffuse.a < 0.15f);
        }

        // base texture is translucent
        if (_textures[to_const_uint(ShaderProgram::TextureUsage::UNIT0)] &&
            _textures[to_const_uint(ShaderProgram::TextureUsage::UNIT0)]->hasTransparency()) {
            _translucencySource = TranslucencySource::DIFFUSE_MAP;
            useAlphaTest = true;
        }

        // opacity map
        if (_textures[to_const_uint(ShaderProgram::TextureUsage::OPACITY)]) {
            _translucencySource = TranslucencySource::OPACITY_MAP;
            useAlphaTest = false;
        }

        _translucencyCheck = false;

        // Disable culling for translucent items
        if (_translucencySource != TranslucencySource::COUNT) {
            setDoubleSided(true, useAlphaTest);
        } else {
            recomputeShaders();
        }
    }

    return isTranslucent();
}

void Material::getSortKeys(I32& shaderKey, I32& textureKey) const {
    static const I16 invalidShaderKey = -std::numeric_limits<I16>::max();

    const ShaderProgramInfo& info = _shaderInfo[to_const_uint(RenderStage::DISPLAY)];

    shaderKey = info._shaderRef ? info._shaderRef->getID()
                                : invalidShaderKey;

    std::weak_ptr<Texture> albedoTex = getTexture(ShaderProgram::TextureUsage::UNIT0);
    textureKey = albedoTex.expired() ? invalidShaderKey : albedoTex.lock()->getHandle();
}

void Material::getMaterialMatrix(mat4<F32>& retMatrix) const {
    retMatrix.setRow(0, _colourData._diffuse);
    retMatrix.setRow(1, _colourData._specular);
    retMatrix.setRow(2, vec4<F32>(_colourData._emissive.rgb(), _colourData._shininess));
    retMatrix.setRow(3, vec4<F32>(isTranslucent() ? 1.0f : 0.0f,  to_float(getTextureOperation()), getParallaxFactor(), 0.0));
}

void Material::rebuild() {
    recomputeShaders();
    for (U32 i = 0; i < to_const_uint(RenderStage::COUNT); ++i) {
        computeShader(static_cast<RenderStage>(i), _highPriority);
        _shaderInfo[i]._shaderRef->recompile();
    }
}

};