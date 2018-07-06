#include "config.h"

#include "Headers/Material.h"
#include "Headers/ShaderComputeQueue.h"

#include "Rendering/Headers/Renderer.h"
#include "Utility/Headers/Localization.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Core/Headers/Console.h"

namespace Divide {

namespace {
    const char* g_DepthPassMaterialShaderName = "depthPass";
    const char* g_ForwardMaterialShaderName = "material";
    const char* g_DeferredMaterialShaderName = "DeferredShadingPass1";
    const char* g_PassThroughMaterialShaderName = "passThrough";
};

Material::Material(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name)
    : CachedResource(ResourceType::DEFAULT, descriptorHash, name),
      _context(context),
      _parentCache(parentCache),
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
    _textureExtenalFlag[to_base(ShaderProgram::TextureUsage::REFLECTION_PLANAR)] = true;
    _textureExtenalFlag[to_base(ShaderProgram::TextureUsage::REFRACTION_PLANAR)] = true;
    _textureExtenalFlag[to_base(ShaderProgram::TextureUsage::REFLECTION_CUBE)] = true;
    _textureExtenalFlag[to_base(ShaderProgram::TextureUsage::REFRACTION_CUBE)] = true;
    _textureExtenalFlag[to_base(ShaderProgram::TextureUsage::DEPTH)] = true;
    _textureExtenalFlag[to_base(ShaderProgram::TextureUsage::DEPTH_PREV)] = true;
    defaultReflectionTexture(nullptr, 0);
    defaultRefractionTexture(nullptr, 0);

    _operation = TextureOperation::NONE;

    /// Normal state for final rendering
    RenderStateBlock stateDescriptor;
    stateDescriptor.setZFunc(ComparisonFunction::LEQUAL);
    /// the reflection descriptor is the same as the normal descriptor
    RenderStateBlock reflectorDescriptor(stateDescriptor);
    /// the z-pre-pass descriptor does not process colours
    RenderStateBlock zPrePassDescriptor(stateDescriptor);
    zPrePassDescriptor.setColourWrites(true, true, true, false);
    zPrePassDescriptor.setZFunc(ComparisonFunction::LESS);

    /// A descriptor used for rendering to depth map
    RenderStateBlock shadowDescriptor(stateDescriptor);
    shadowDescriptor.setCullMode(CullMode::CCW);
    /// set a polygon offset
    shadowDescriptor.setZBias(1.0f, 2.0f);
    /// ignore half of the colours 
    /// Some shadowing techniques require drawing to the a colour buffer
    shadowDescriptor.setColourWrites(true, true, false, false);
    RenderStateBlock shadowDescriptorNoColour(shadowDescriptor);
    shadowDescriptorNoColour.setColourWrites(false, false, false, false);

    setRenderStateBlock(stateDescriptor.getHash(), RenderStagePass(RenderStage::DISPLAY, RenderPassType::COLOUR_PASS));
    setRenderStateBlock(stateDescriptor.getHash(), RenderStagePass(RenderStage::REFRACTION, RenderPassType::COLOUR_PASS));
    setRenderStateBlock(reflectorDescriptor.getHash(), RenderStagePass(RenderStage::REFLECTION, RenderPassType::COLOUR_PASS));

    setRenderStateBlock(shadowDescriptor.getHash(), RenderStagePass(RenderStage::SHADOW, RenderPassType::COLOUR_PASS), 0);
    setRenderStateBlock(shadowDescriptorNoColour.getHash(), RenderStagePass(RenderStage::SHADOW, RenderPassType::COLOUR_PASS), 1);
    setRenderStateBlock(shadowDescriptorNoColour.getHash(), RenderStagePass(RenderStage::SHADOW, RenderPassType::COLOUR_PASS), 2);

    setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStagePass(RenderStage::DISPLAY, RenderPassType::DEPTH_PASS));
    setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStagePass(RenderStage::REFRACTION, RenderPassType::DEPTH_PASS));
    setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStagePass(RenderStage::REFLECTION, RenderPassType::DEPTH_PASS));
    setRenderStateBlock(shadowDescriptor.getHash(), RenderStagePass(RenderStage::SHADOW, RenderPassType::DEPTH_PASS), 0);
    setRenderStateBlock(shadowDescriptorNoColour.getHash(), RenderStagePass(RenderStage::SHADOW, RenderPassType::DEPTH_PASS), 1);
    setRenderStateBlock(shadowDescriptorNoColour.getHash(), RenderStagePass(RenderStage::SHADOW, RenderPassType::DEPTH_PASS), 2);
}

Material::~Material()
{
}


Material_ptr Material::clone(const stringImpl& nameSuffix) {
    DIVIDE_ASSERT(!nameSuffix.empty(),
                  "Material error: clone called without a valid name suffix!");

    const Material& base = *this;
    Material_ptr cloneMat = CreateResource<Material>(_parentCache, ResourceDescriptor(getName() + nameSuffix));

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

    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        for (U8 i = 0; i < to_base(RenderStage::COUNT); i++) {
            cloneMat->_shaderModifier[pass][i] = base._shaderModifier[pass][i];
            cloneMat->_shaderInfo[pass][i] = _shaderInfo[pass][i];
            for (U8 j = 0; j < _defaultRenderStates[pass][i].size(); ++j) {
                cloneMat->_defaultRenderStates[pass][i][j] = _defaultRenderStates[pass][i][j];
            }
        }
    }

    for (U8 i = 0; i < to_U8(base._textures.size()); ++i) {
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
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        for (ShaderProgramInfo& info : _shaderInfo[pass]) {
            if (info.update()) {
                _dirty = true;
            }
        }
    }

    clean();
}

size_t Material::getRenderStateBlock(const RenderStagePass& renderStagePass, I32 variant) {
    assert(variant >= 0 && variant < defaultRenderStates(renderStagePass).size());
    return defaultRenderStates(renderStagePass)[variant];
}

// base = base texture
// second = second texture used for multitexturing
// bump = bump map
bool Material::setTexture(ShaderProgram::TextureUsage textureUsageSlot,
                          const Texture_ptr& texture,
                          const TextureOperation& op) {
    bool computeShaders = false;
    U32 slot = to_U32(textureUsageSlot);

    if (textureUsageSlot == ShaderProgram::TextureUsage::UNIT1) {
        _operation = op;
    }

    if (texture && textureUsageSlot == ShaderProgram::TextureUsage::OPACITY) {
        Texture_ptr& diffuseMap = _textures[to_base(ShaderProgram::TextureUsage::UNIT0)];
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
        if (textureUsageSlot != ShaderProgram::TextureUsage::REFLECTION_PLANAR &&
            textureUsageSlot != ShaderProgram::TextureUsage::REFRACTION_PLANAR &&
            textureUsageSlot != ShaderProgram::TextureUsage::REFLECTION_CUBE &&
            textureUsageSlot != ShaderProgram::TextureUsage::REFRACTION_CUBE) {
            // if we add a new type of texture recompute shaders
            computeShaders = true;
        }
    }

    _textures[slot] = texture;

    if (computeShaders) {
        recomputeShaders();
    }

    _dirty = textureUsageSlot != ShaderProgram::TextureUsage::REFLECTION_PLANAR &&
             textureUsageSlot != ShaderProgram::TextureUsage::REFRACTION_PLANAR &&
             textureUsageSlot != ShaderProgram::TextureUsage::REFLECTION_CUBE &&
             textureUsageSlot != ShaderProgram::TextureUsage::REFRACTION_CUBE;

    return true;
}

// Here we set the shader's name
void Material::setShaderProgram(const stringImpl& shader,
                                const RenderStagePass& renderStagePass,
                                const bool computeOnAdd) {
    shaderInfo(renderStagePass)._customShader = true;
    setShaderProgramInternal(shader, renderStagePass, computeOnAdd);
}

void Material::setShaderProgram(const stringImpl& shader,
                                RenderStage stage,
                                const bool computeOnAdd) {
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        setShaderProgram(shader, RenderStagePass(stage, static_cast<RenderPassType>(pass)), computeOnAdd);
    }
}

void Material::setShaderProgram(const stringImpl& shader,
                                RenderPassType passType,
                                const bool computeOnAdd) {
    for (U8 stage = 0; stage < to_base(RenderStage::COUNT); ++stage) {
        setShaderProgram(shader, RenderStagePass(static_cast<RenderStage>(stage), passType), computeOnAdd);
    }
}

void Material::setShaderProgramInternal(const stringImpl& shader,
                                        const RenderStagePass& renderStagePass,
                                        const bool computeOnAdd) {
    ShaderProgramInfo& info = shaderInfo(renderStagePass);

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

    // if we already had a shader assigned ...
    if (!info._shader.empty()) {
        // and we are trying to assign the same one again, return.
        info._shaderRef = FindResourceImpl<ShaderProgram>(_parentCache, shaderDescriptor.getHash());
        if (info._shader.compare(shader) != 0) {
            Console::printfn(Locale::get(_ID("REPLACE_SHADER")), info._shader.c_str(), shader.c_str());
        }
    }

    ShaderComputeQueue::ShaderQueueElement queueElement(shaderDescriptor);
    queueElement._shaderData = &shaderInfo(renderStagePass);
    
    ShaderComputeQueue& shaderQueue = _context.shaderComputeQueue();
    if (computeOnAdd) {
        shaderQueue.addToQueueFront(queueElement);
        shaderQueue.stepQueue();
    } else {
        shaderQueue.addToQueueBack(queueElement);
    }
}

void Material::clean() {
    if (_dirty && _dumpToFile) {
        updateTranslucency();
        if (!Config::Build::IS_DEBUG_BUILD) {
            //XML::dumpMaterial(_context, *this);
        }
        _dirty = false;
    }
}

void Material::recomputeShaders() {
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        for (ShaderProgramInfo& info : _shaderInfo[pass]) {
            if (!info._customShader) {
                info.computeStage(ShaderProgramInfo::BuildStage::REQUESTED);
            }
        }
    }
}

bool Material::canDraw(const RenderStagePass& renderStage) {
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        const std::array<ShaderProgramInfo, to_base(RenderStage::COUNT)>& passInfo = _shaderInfo[pass];
        for (U32 i = 0; i < to_base(RenderStage::COUNT); ++i) {
            const ShaderProgramInfo& info = passInfo[i];
            if (info.computeStage() != ShaderProgramInfo::BuildStage::READY) {
                computeShader(RenderStagePass(static_cast<RenderStage>(i), static_cast<RenderPassType>(pass)), _highPriority);
                return false;
            }
        }
    }

    return true;
}

void Material::updateReflectionIndex(ReflectorType type, I32 index) {
    _reflectionIndex = index;
    if (_reflectionIndex > -1) {
        RenderTarget& reflectionTarget =
            _context.renderTarget(RenderTargetID(type == ReflectorType::PLANAR_REFLECTOR
                                                       ? RenderTargetUsage::REFLECTION_PLANAR
                                                       : RenderTargetUsage::REFLECTION_CUBE,
                                  index));
        const Texture_ptr& refTex = reflectionTarget.getAttachment(RTAttachment::Type::Colour, 0).asTexture();
        setTexture(type == ReflectorType::PLANAR_REFLECTOR
                         ? ShaderProgram::TextureUsage::REFLECTION_PLANAR
                         : ShaderProgram::TextureUsage::REFLECTION_CUBE,
                   refTex);
    } else {
        setTexture(type == ReflectorType::PLANAR_REFLECTOR
                         ? ShaderProgram::TextureUsage::REFLECTION_PLANAR
                         : ShaderProgram::TextureUsage::REFLECTION_CUBE, 
                   _defaultReflection.first);
    }
}

void Material::updateRefractionIndex(ReflectorType type, I32 index) {
    _refractionIndex = index;
    if (_refractionIndex > -1) {
        RenderTarget& refractionTarget =
            _context.renderTarget(RenderTargetID(type == ReflectorType::PLANAR_REFLECTOR
                                                       ? RenderTargetUsage::REFRACTION_PLANAR
                                                       : RenderTargetUsage::REFRACTION_CUBE,
                                  index));
        const Texture_ptr& refTex = refractionTarget.getAttachment(RTAttachment::Type::Colour, 0).asTexture();
        setTexture(type == ReflectorType::PLANAR_REFLECTOR
                         ? ShaderProgram::TextureUsage::REFRACTION_PLANAR
                         : ShaderProgram::TextureUsage::REFRACTION_CUBE,
                   refTex);
    } else {
        setTexture(type == ReflectorType::PLANAR_REFLECTOR
                         ? ShaderProgram::TextureUsage::REFRACTION_PLANAR
                         : ShaderProgram::TextureUsage::REFRACTION_CUBE,
                   _defaultRefraction.first);
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

/// If the current material doesn't have a shader associated with it, then add the default ones.
bool Material::computeShader(const RenderStagePass& renderStagePass, const bool computeOnAdd){
    ShaderProgramInfo& info = shaderInfo(renderStagePass);
    // If shader's invalid, try to request a recompute as it might fix it
    if (info.computeStage() == ShaderProgramInfo::BuildStage::COUNT) {
        info.computeStage(ShaderProgramInfo::BuildStage::REQUESTED);
        return false;
    }

    // If the shader is valid and a recompute wasn't requested, just return true
    if (info.computeStage() != ShaderProgramInfo::BuildStage::REQUESTED) {
        return info.computeStage() == ShaderProgramInfo::BuildStage::READY;
    }

    // At this point, only computation requests are processed
    assert(info.computeStage() == ShaderProgramInfo::BuildStage::REQUESTED);

    const U32 slot0 = to_base(ShaderProgram::TextureUsage::UNIT0);
    const U32 slot1 = to_base(ShaderProgram::TextureUsage::UNIT1);
    const U32 slotOpacity = to_base(ShaderProgram::TextureUsage::OPACITY);

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

    bool deferredPassShader = _context.getRenderer().getType() !=
                              RendererType::RENDERER_TILED_FORWARD_SHADING;
    bool depthPassShader = renderStagePass._stage == RenderStage::SHADOW ||
                           renderStagePass._passType == RenderPassType::DEPTH_PASS;

    // the base shader is either for a Deferred Renderer or a Forward  one ...
    stringImpl shader =
        (deferredPassShader ? g_DeferredMaterialShaderName
                            : (depthPassShader ? g_DepthPassMaterialShaderName : g_ForwardMaterialShaderName));

    if (Config::Profile::DISABLE_SHADING) {
        shader = g_PassThroughMaterialShaderName;
        setShaderProgramInternal(shader, renderStagePass, computeOnAdd);
        return false;
    }

    if (depthPassShader && renderStagePass._stage == RenderStage::SHADOW) {
        setShaderDefines(renderStagePass, "SHADOW_PASS");
        shader += ".Shadow";
    } else {
        setShaderDefines(renderStagePass, "DEPTH_PASS");
    }

    // What kind of effects do we need?
    if (_textures[slot0]) {
        // Bump mapping?
        if (_textures[to_base(ShaderProgram::TextureUsage::NORMALMAP)] &&  _bumpMethod != BumpMethod::NONE) {
            setShaderDefines(renderStagePass, "COMPUTE_TBN");
            shader += ".Bump";  // Normal Mapping
            if (_bumpMethod == BumpMethod::PARALLAX) {
                shader += ".Parallax";
                setShaderDefines(renderStagePass, "USE_PARALLAX_MAPPING");
            } else if (_bumpMethod == BumpMethod::RELIEF) {
                shader += ".Relief";
                setShaderDefines(renderStagePass, "USE_RELIEF_MAPPING");
            }
        } else {
            // Or simple texture mapping?
            shader += ".Texture";
        }
    } else {
        setShaderDefines(renderStagePass, "SKIP_TEXTURES");
        shader += ".NoTexture";
    }

    if (_textures[to_base(ShaderProgram::TextureUsage::SPECULAR)]) {
        shader += ".Specular";
        setShaderDefines(renderStagePass, "USE_SPECULAR_MAP");
    }

    if (updateTranslucency()) {
        switch (_translucencySource) {
            case TranslucencySource::OPACITY_MAP: {
                shader += ".OpacityMap";
                setShaderDefines(renderStagePass, "USE_OPACITY_MAP");
            } break;
            case TranslucencySource::DIFFUSE: {
                shader += ".DiffuseAlpha";
                setShaderDefines(renderStagePass, "USE_OPACITY_DIFFUSE");
            } break;
            case TranslucencySource::DIFFUSE_MAP: {
                shader += ".DiffuseMapAlpha";
                setShaderDefines(renderStagePass, "USE_OPACITY_DIFFUSE_MAP");
            } break;
        };
    }

    if (_doubleSided) {
        shader += ".DoubleSided";
        setShaderDefines(renderStagePass, "USE_DOUBLE_SIDED");
    }

    // Add the GPU skinning module to the vertex shader?
    if (_hardwareSkinning) {
        setShaderDefines(renderStagePass, "USE_GPU_SKINNING");
        shader += ",Skinned";  //<Use "," instead of "." will add a Vertex only property
    }

    switch (_shadingMode) {
        default:
        case ShadingMode::FLAT: {
            setShaderDefines(renderStagePass, "USE_SHADING_FLAT");
            shader += ".Flat";
        } break;
        case ShadingMode::PHONG: {
            setShaderDefines(renderStagePass, "USE_SHADING_PHONG");
            shader += ".Phong";
        } break;
        case ShadingMode::BLINN_PHONG: {
            setShaderDefines(renderStagePass, "USE_SHADING_BLINN_PHONG");
            shader += ".BlinnPhong";
        } break;
        case ShadingMode::TOON: {
            setShaderDefines(renderStagePass, "USE_SHADING_TOON");
            shader += ".Toon";
        } break;
        case ShadingMode::OREN_NAYAR: {
            setShaderDefines(renderStagePass, "USE_SHADING_OREN_NAYAR");
            shader += ".OrenNayar";
        } break;
        case ShadingMode::COOK_TORRANCE: {
            setShaderDefines(renderStagePass, "USE_SHADING_COOK_TORRANCE");
            shader += ".CookTorrance";
        } break;
    }
    // Add any modifiers you wish
    stringImpl& modifier = _shaderModifier[to_U32(renderStagePass._passType)][to_U32(renderStagePass._stage)];
    if (!modifier.empty()) {
        shader += ".";
        shader += modifier;
    }

    setShaderProgramInternal(shader, renderStagePass, computeOnAdd);

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
    U32 slotValue = to_U32(slot);
    Texture_ptr& crtTexture = _textures[slotValue];
    if (crtTexture && crtTexture->flushTextureState()) {
        TextureData& data = crtTexture->getData();
        data.setHandleLow(slotValue);
        container.addTexture(data);
    }
}

void Material::getTextureData(TextureDataContainer& textureData) {
    const U32 textureCount = to_base(ShaderProgram::TextureUsage::COUNT);

    if (!_context.isDepthStage()) {
        getTextureData(ShaderProgram::TextureUsage::UNIT0, textureData);
        getTextureData(ShaderProgram::TextureUsage::UNIT1, textureData);
        getTextureData(ShaderProgram::TextureUsage::OPACITY, textureData);
        getTextureData(ShaderProgram::TextureUsage::NORMALMAP, textureData);
        getTextureData(ShaderProgram::TextureUsage::SPECULAR, textureData);
        getTextureData(ShaderProgram::TextureUsage::REFLECTION_PLANAR, textureData);
        getTextureData(ShaderProgram::TextureUsage::REFRACTION_PLANAR, textureData);
        getTextureData(ShaderProgram::TextureUsage::REFLECTION_CUBE, textureData);
        getTextureData(ShaderProgram::TextureUsage::REFRACTION_CUBE, textureData);

        for (std::pair<Texture_ptr, U8>& tex : _customTextures) {
            if (tex.first->flushTextureState()) {
                TextureData& data = tex.first->getData();
                data.setHandleLow(to_U32(tex.second));
                textureData.addTexture(data);
            }
        }
    } else {
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

ShaderProgramInfo& Material::getShaderInfo(const RenderStagePass& renderStagePass) {
    return shaderInfo(renderStagePass);
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
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        _shaderInfo[pass].fill(ShaderProgramInfo());
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
        for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
            for (U32 index = 0; index < to_base(RenderStage::COUNT); ++index) {
                for (U8 variant = 0; variant < to_U8(_defaultRenderStates[pass][index].size()); ++variant) {
                    size_t hash = _defaultRenderStates[pass][index][variant];
                    RenderStateBlock descriptor(RenderStateBlock::get(hash));
                    descriptor.setCullMode(CullMode::NONE);
                    if (_translucencySource != TranslucencySource::COUNT) {
                        descriptor.setBlend(true);
                    }
                    setRenderStateBlock(descriptor.getHash(), RenderStagePass(static_cast<RenderStage>(index), static_cast<RenderPassType>(pass)), variant);
                }
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
        if (_textures[to_base(ShaderProgram::TextureUsage::UNIT0)] &&
            _textures[to_base(ShaderProgram::TextureUsage::UNIT0)]->hasTransparency()) {
            _translucencySource = TranslucencySource::DIFFUSE_MAP;
            useAlphaTest = true;
        }

        // opacity map
        if (_textures[to_base(ShaderProgram::TextureUsage::OPACITY)]) {
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

void Material::getSortKeys(const RenderStagePass& renderStagePass, I32& shaderKey, I32& textureKey) const {
    static const I16 invalidShaderKey = -std::numeric_limits<I16>::max();

    const ShaderProgramInfo& info = shaderInfo(renderStagePass);

    shaderKey = info._shaderRef ? info._shaderRef->getID()
                                : invalidShaderKey;

    std::weak_ptr<Texture> albedoTex = getTexture(ShaderProgram::TextureUsage::UNIT0);
    textureKey = albedoTex.expired() ? invalidShaderKey : albedoTex.lock()->getHandle();
}

void Material::getMaterialMatrix(mat4<F32>& retMatrix) const {
    retMatrix.setRow(0, _colourData._diffuse);
    retMatrix.setRow(1, _colourData._specular);
    retMatrix.setRow(2, vec4<F32>(_colourData._emissive.rgb(), _colourData._shininess));
    retMatrix.setRow(3, vec4<F32>(isTranslucent() ? 1.0f : 0.0f,  to_F32(getTextureOperation()), getParallaxFactor(), 0.0));
}

void Material::rebuild() {
    recomputeShaders();

    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        for (U32 i = 0; i < to_base(RenderStage::COUNT); ++i) {
            computeShader(RenderStagePass(static_cast<RenderStage>(i), static_cast<RenderPassType>(pass)), _highPriority);
            _shaderInfo[pass][i]._shaderRef->recompile();
        }
    }
}

};