#include "stdafx.h"

#include "config.h"

#include "Headers/Material.h"
#include "Headers/ShaderComputeQueue.h"

#include "Rendering/Headers/Renderer.h"
#include "Utility/Headers/Localization.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

#include "ECS/Components/Headers/RenderingComponent.h"

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
      _isReflective(false),
      _isRefractive(false),
      _shaderThreadedLoad(true),
      _hardwareSkinning(false),
      _dumpToFile(true),
      _ignoreXMLData(false),
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

    RenderStateBlock oitDescriptor(stateDescriptor);
    oitDescriptor.depthTestEnabled(true);

    /// the reflection descriptor is the same as the normal descriptor
    RenderStateBlock reflectorDescriptor(stateDescriptor);
    RenderStateBlock reflectorOitDescriptor(stateDescriptor);
    reflectorOitDescriptor.depthTestEnabled(false);
    /// the z-pre-pass descriptor does not process colours
    RenderStateBlock zPrePassDescriptor(stateDescriptor);
    zPrePassDescriptor.setColourWrites(true, true, true, false);
    zPrePassDescriptor.setZFunc(ComparisonFunction::LESS);

    /// A descriptor used for rendering to depth map
    RenderStateBlock shadowDescriptor(stateDescriptor);
    shadowDescriptor.setCullMode(CullMode::CCW);
    /// set a polygon offset
    shadowDescriptor.setZBias(1.0f, 1.0f);
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

    setRenderStateBlock(oitDescriptor.getHash(), RenderStagePass(RenderStage::DISPLAY, RenderPassType::OIT_PASS));
    setRenderStateBlock(oitDescriptor.getHash(), RenderStagePass(RenderStage::REFRACTION, RenderPassType::OIT_PASS));
    setRenderStateBlock(reflectorOitDescriptor.getHash(), RenderStagePass(RenderStage::REFLECTION, RenderPassType::OIT_PASS));

    setRenderStateBlock(shadowDescriptor.getHash(), RenderStagePass(RenderStage::SHADOW, RenderPassType::OIT_PASS), 0);
    setRenderStateBlock(shadowDescriptorNoColour.getHash(), RenderStagePass(RenderStage::SHADOW, RenderPassType::OIT_PASS), 1);
    setRenderStateBlock(shadowDescriptorNoColour.getHash(), RenderStagePass(RenderStage::SHADOW, RenderPassType::OIT_PASS), 2);

    setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStagePass(RenderStage::DISPLAY, RenderPassType::DEPTH_PASS));
    setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStagePass(RenderStage::REFRACTION, RenderPassType::DEPTH_PASS));
    setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStagePass(RenderStage::REFLECTION, RenderPassType::DEPTH_PASS));

    setRenderStateBlock(shadowDescriptor.getHash(), RenderStagePass(RenderStage::SHADOW, RenderPassType::DEPTH_PASS), 0);
    setRenderStateBlock(shadowDescriptorNoColour.getHash(), RenderStagePass(RenderStage::SHADOW, RenderPassType::DEPTH_PASS), 1);
    setRenderStateBlock(shadowDescriptorNoColour.getHash(), RenderStagePass(RenderStage::SHADOW, RenderPassType::DEPTH_PASS), 2);

    addShaderDefine(RenderStage::SHADOW, "SHADOW_PASS", true);
    addShaderDefine(RenderPassType::DEPTH_PASS, "DEPTH_PASS", true);
    addShaderDefine(RenderPassType::OIT_PASS, "OIT_PASS", true);
}

Material::~Material()
{
}


Material_ptr Material::clone(const stringImpl& nameSuffix) {
    DIVIDE_ASSERT(!nameSuffix.empty(),
                  "Material error: clone called without a valid name suffix!");

    const Material& base = *this;
    Material_ptr cloneMat = CreateResource<Material>(_parentCache, ResourceDescriptor(resourceName() + nameSuffix));

    cloneMat->_shadingMode = base._shadingMode;
    cloneMat->_translucencyCheck = base._translucencyCheck;
    cloneMat->_dumpToFile = base._dumpToFile;
    cloneMat->_doubleSided = base._doubleSided;
    cloneMat->_isReflective = base._isReflective;
    cloneMat->_isRefractive = base._isRefractive;
    cloneMat->_hardwareSkinning = base._hardwareSkinning;
    cloneMat->_shaderThreadedLoad = base._shaderThreadedLoad;
    cloneMat->_operation = base._operation;
    cloneMat->_bumpMethod = base._bumpMethod;
    cloneMat->_ignoreXMLData = base._ignoreXMLData;
    cloneMat->_parallaxFactor = base._parallaxFactor;
    cloneMat->_reflectionIndex = base._reflectionIndex;
    cloneMat->_refractionIndex = base._refractionIndex;
    cloneMat->_defaultReflection = base._defaultReflection;
    cloneMat->_defaultRefraction = base._defaultRefraction;
    cloneMat->_translucencySource = base._translucencySource;

    for (RenderStagePass::PassIndex i = 0; i < RenderStagePass::count(); ++i) {
        cloneMat->_shaderModifier[i] = base._shaderModifier[i];
        cloneMat->_shaderInfo[i] = _shaderInfo[i];
        for (U8 j = 0; j < _defaultRenderStates[i].size(); ++j) {
            cloneMat->_defaultRenderStates[i][j] = _defaultRenderStates[i][j];
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
    for (const ExternalTexture& tex : base._externalTextures) {
        if (tex._texture) {
            cloneMat->addExternalTexture(tex._texture, tex._bindSlot, tex._activeForDepth);
        }
    }

    cloneMat->_colourData = base._colourData;

    return cloneMat;
}

void Material::update(const U64 deltaTimeUS) {
    for (ShaderProgramInfo& info : _shaderInfo) {
        _dirty = _dirty || info.update();
    }

    clean();
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
    } else {
        // Skip adding same texture
        if (_textures[slot]->getGUID() == texture->getGUID()) {
            return true;
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


void Material::setShaderProgramInternal(const ShaderProgram_ptr& shader,
                                        RenderStagePass renderStagePass) {
    ShaderProgramInfo& info = shaderInfo(renderStagePass);
    if (shader != nullptr) {
        info._customShader = true;
        info._shaderRef = shader;
        info.computeStage(ShaderProgramInfo::BuildStage::COMPUTED);
    } else {
        setShaderProgramInternal("", renderStagePass, true);
    }
}

void Material::setShaderProgramInternal(const stringImpl& shader,
                                        RenderStagePass renderStagePass,
                                        const bool computeOnAdd) {
    ShaderProgramInfo& info = shaderInfo(renderStagePass);

    stringImpl shaderName = shader.empty() ? "NULL" : shader;
    ResourceDescriptor shaderDescriptor(shaderName);
    ShaderProgramDescriptor shaderPropertyDescriptor;
    shaderPropertyDescriptor._defines = info._shaderDefines;
    shaderPropertyDescriptor._defines.push_back(std::make_pair("DEFINE_PLACEHOLDER", false));
    shaderDescriptor.setPropertyDescriptor(shaderPropertyDescriptor);
    shaderDescriptor.setThreadedLoading(_shaderThreadedLoad);

    // if we already have a different shader assigned ...
    if (info._shaderRef != nullptr && info._shaderRef->resourceName().compare(shader) != 0)
    {
        // We cannot replace a shader that is still loading in the background
        WAIT_FOR_CONDITION(info._shaderRef->getState() == ResourceState::RES_LOADED);
        Console::printfn(Locale::get(_ID("REPLACE_SHADER")), info._shaderRef->resourceName().c_str(), shader.c_str());
    }

    ShaderComputeQueue::ShaderQueueElement queueElement(shaderDescriptor);
    queueElement._shaderData = &shaderInfo(renderStagePass);
    
    ShaderComputeQueue& shaderQueue = _context.shaderComputeQueue();
    if (computeOnAdd)
    {
        shaderQueue.addToQueueFront(queueElement);
        shaderQueue.stepQueue();
    }
    else
    {
        shaderQueue.addToQueueBack(queueElement);
    }
    
}

void Material::clean() {
    if (_dirty && _dumpToFile) {
        updateTranslucency();
        if (!Config::Build::IS_DEBUG_BUILD) {
            //ToDo: Save to XML ... 
        }
    }

    _dirty = false;
}

void Material::recomputeShaders() {
    for (ShaderProgramInfo& info : _shaderInfo) {
        if (!info._customShader) {
            info.computeStage(ShaderProgramInfo::BuildStage::REQUESTED);
        }
    }
}

bool Material::canDraw(RenderStagePass renderStage) {
    for (U8 i = 0; i < to_U8(RenderPassType::COUNT); ++i) {
        U8 passIndex = RenderStagePass::index(renderStage._stage, static_cast<RenderPassType>(i));

        if (_shaderInfo[passIndex].computeStage() != ShaderProgramInfo::BuildStage::READY) {
            computeShader(RenderStagePass::stagePass(passIndex), _highPriority);
            return false;
        }
    }

    return true;
}

void Material::updateReflectionIndex(ReflectorType type, I32 index) {
    _reflectionIndex = index;
    if (_reflectionIndex > -1) {
        RenderTarget& reflectionTarget =
            _context.renderTargetPool().renderTarget(RenderTargetID(type == ReflectorType::PLANAR_REFLECTOR
                                                                          ? RenderTargetUsage::REFLECTION_PLANAR
                                                                          : RenderTargetUsage::REFLECTION_CUBE,
                                                     index));
        const Texture_ptr& refTex = reflectionTarget.getAttachment(RTAttachmentType::Colour, 0).texture();
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
            _context.renderTargetPool().renderTarget(RenderTargetID(type == ReflectorType::PLANAR_REFLECTOR
                                                                          ? RenderTargetUsage::REFRACTION_PLANAR
                                                                          : RenderTargetUsage::REFRACTION_CUBE,
                                                     index));
        const Texture_ptr& refTex = refractionTarget.getAttachment(RTAttachmentType::Colour, 0).texture();
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
bool Material::computeShader(RenderStagePass renderStagePass, const bool computeOnAdd){
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

    //info._shaderDefines.clear();

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

    if (depthPassShader) {
        shader += renderStagePass._stage == RenderStage::SHADOW ? ".Shadow" : ".PrePass";
    }

    if (renderStagePass._passType == RenderPassType::OIT_PASS) {
        shader += ".OIT";
    }

    // What kind of effects do we need?
    if (_textures[slot0]) {
        // Bump mapping?
        if (_textures[to_base(ShaderProgram::TextureUsage::NORMALMAP)] &&  _bumpMethod != BumpMethod::NONE) {
            addShaderDefine(renderStagePass, "COMPUTE_TBN");
            shader += ".Bump";  // Normal Mapping
            if (_bumpMethod == BumpMethod::PARALLAX) {
                shader += ".Parallax";
                addShaderDefine(renderStagePass, "USE_PARALLAX_MAPPING");
            } else if (_bumpMethod == BumpMethod::RELIEF) {
                shader += ".Relief";
                addShaderDefine(renderStagePass, "USE_RELIEF_MAPPING");
            }
        } else {
            // Or simple texture mapping?
            shader += ".Texture";
        }
    } else {
        addShaderDefine(renderStagePass, "SKIP_TEXTURES");
        shader += ".NoTexture";
    }

    if (_textures[to_base(ShaderProgram::TextureUsage::SPECULAR)]) {
        shader += ".Specular";
        addShaderDefine(renderStagePass, "USE_SPECULAR_MAP");
    }

    updateTranslucency(false);

    switch (_translucencySource) {
        case TranslucencySource::OPACITY_MAP: {
            shader += ".OpacityMap";
            addShaderDefine(renderStagePass, "USE_OPACITY_MAP");
        } break;
        case TranslucencySource::DIFFUSE: {
            shader += ".DiffuseAlpha";
            addShaderDefine(renderStagePass, "USE_OPACITY_DIFFUSE");
        } break;
        case TranslucencySource::DIFFUSE_MAP: {
            shader += ".DiffuseMapAlpha";
            addShaderDefine(renderStagePass, "USE_OPACITY_DIFFUSE_MAP");
        } break;
        default: break;
    };

    if (_translucencySource != TranslucencySource::COUNT && renderStagePass._passType != RenderPassType::OIT_PASS) {
        shader += ".AlphaDiscard";
        addShaderDefine(renderStagePass, "USE_ALPHA_DISCARD");
    }

    if (isDoubleSided()) {
        shader += ".DoubleSided";
        addShaderDefine(renderStagePass, "USE_DOUBLE_SIDED");
    }

    if (isRefractive()) {
        shader += ".Refractive";
        addShaderDefine(renderStagePass, "IS_REFRACTIVE");
    }

    if (isReflective()) {
        shader += ".Reflective";
        addShaderDefine(renderStagePass, "IS_REFLECTIVE");
    }

    if (!_context.parent().platformContext().config().rendering.shadowMapping.enabled) {
        shader += ".NoShadows";
        addShaderDefine(renderStagePass, "DISABLE_SHADOW_MAPPING");
    }

    // Add the GPU skinning module to the vertex shader?
    if (_hardwareSkinning) {
        addShaderDefine(renderStagePass, "USE_GPU_SKINNING");
        shader += ",Skinned";  //<Use "," instead of "." will add a Vertex only property
    }

    switch (_shadingMode) {
        default:
        case ShadingMode::FLAT: {
            addShaderDefine(renderStagePass, "USE_SHADING_FLAT");
            shader += ".Flat";
        } break;
        case ShadingMode::PHONG: {
            addShaderDefine(renderStagePass, "USE_SHADING_PHONG");
            shader += ".Phong";
        } break;
        case ShadingMode::BLINN_PHONG: {
            addShaderDefine(renderStagePass, "USE_SHADING_BLINN_PHONG");
            shader += ".BlinnPhong";
        } break;
        case ShadingMode::TOON: {
            addShaderDefine(renderStagePass, "USE_SHADING_TOON");
            shader += ".Toon";
        } break;
        case ShadingMode::OREN_NAYAR: {
            addShaderDefine(renderStagePass, "USE_SHADING_OREN_NAYAR");
            shader += ".OrenNayar";
        } break;
        case ShadingMode::COOK_TORRANCE: {
            addShaderDefine(renderStagePass, "USE_SHADING_COOK_TORRANCE");
            shader += ".CookTorrance";
        } break;
    }

    // Add any modifiers you wish
    stringImpl& modifier = _shaderModifier[renderStagePass.index()];
    if (!modifier.empty()) {
        shader += ".";
        shader += modifier;
    }

    setShaderProgramInternal(shader, renderStagePass, computeOnAdd);

    return false;
}

/// Remove the custom texture assigned to the specified offset
bool Material::removeCustomTexture(U8 bindslot) {
    vector<ExternalTexture>::iterator it =
        std::find_if(std::begin(_externalTextures),
                    std::end(_externalTextures),
                    [&bindslot](const ExternalTexture& tex)
                    -> bool {
                        return tex._bindSlot == bindslot;
                    });

    if (it == std::end(_externalTextures)) {
        return false;
    }

    _externalTextures.erase(it);

    return true;
}

bool Material::getTextureData(ShaderProgram::TextureUsage slot,
                              TextureDataContainer& container) {
    U8 slotValue = to_U8(slot);
    Texture_ptr& crtTexture = _textures[slotValue];
    if (crtTexture) {
        return container.addTexture(crtTexture->getData(), slotValue);
    }

    return false;
}

bool Material::getTextureData(RenderStagePass renderStagePass, TextureDataContainer& textureData) {
    const U32 textureCount = to_base(ShaderProgram::TextureUsage::COUNT);
    const bool depthstage = renderStagePass.isDepthPass();

    bool ret = false;
    ret = ret || getTextureData(ShaderProgram::TextureUsage::UNIT0, textureData);
    ret = ret || getTextureData(ShaderProgram::TextureUsage::OPACITY, textureData);
    ret = ret || getTextureData(ShaderProgram::TextureUsage::NORMALMAP, textureData);

    if (!depthstage) {
        ret = ret || getTextureData(ShaderProgram::TextureUsage::UNIT1, textureData);
        ret = ret || getTextureData(ShaderProgram::TextureUsage::SPECULAR, textureData);
        ret = ret || getTextureData(ShaderProgram::TextureUsage::REFLECTION_PLANAR, textureData);
        ret = ret || getTextureData(ShaderProgram::TextureUsage::REFRACTION_PLANAR, textureData);
        ret = ret || getTextureData(ShaderProgram::TextureUsage::REFLECTION_CUBE, textureData);
        ret = ret || getTextureData(ShaderProgram::TextureUsage::REFRACTION_CUBE, textureData);
    }

    for (const ExternalTexture& tex : _externalTextures) {
        if (!depthstage || (depthstage && tex._activeForDepth)) {
            ret = ret || textureData.addTexture(tex._texture->getData(), to_U8(tex._bindSlot));
        }
    }

    return ret;
}

bool Material::unload() {
    _textures.fill(nullptr);
    _externalTextures.clear();
    _shaderInfo.fill(ShaderProgramInfo());

    return true;
}

void Material::setReflective(const bool state) {
    if (_isReflective == state) {
        return;
    }

    _isReflective = state;

    _dirty = true;
    recomputeShaders();
}

void Material::setRefractive(const bool state) {
    if (_isRefractive == state) {
        return;
    }

    _isRefractive = state;

    _dirty = true;
    recomputeShaders();
}

void Material::setDoubleSided(const bool state) {
    if (_doubleSided == state) {
        return;
    }

    _doubleSided = state;
    // Update all render states for this item
    if (_doubleSided) {
        for (RenderStagePass::PassIndex i = 0; i < RenderStagePass::count(); ++i) {
            const U8 variantCount = to_U8(_defaultRenderStates[i].size());

            for (U8 variant = 0; variant < variantCount; ++variant) {
                const size_t hash = _defaultRenderStates[i][variant];
                RenderStateBlock descriptor(RenderStateBlock::get(hash));
                descriptor.setCullMode(CullMode::NONE);
                setRenderStateBlock(descriptor.getHash(), RenderStagePass::stagePass(i), variant);
            }
        }
    }

    _dirty = true;
    recomputeShaders();
}

void Material::updateTranslucency(bool requestRecomputeShaders) {
    if (_translucencyCheck) {
        _translucencySource = TranslucencySource::COUNT;

        // In order of importance (less to more)!
        // diffuse channel alpha
        if (_colourData._diffuse.a < 0.95f) {
            _translucencySource = TranslucencySource::DIFFUSE;
        }

        // base texture is translucent
        Texture_ptr albedo = _textures[to_base(ShaderProgram::TextureUsage::UNIT0)];
        if (albedo && albedo->hasTransparency()) {
            _translucencySource = TranslucencySource::DIFFUSE_MAP;
        }

        // opacity map
        Texture_ptr opacity = _textures[to_base(ShaderProgram::TextureUsage::OPACITY)];
        if (opacity && opacity->hasTransparency()) {
            _translucencySource = TranslucencySource::OPACITY_MAP;
        }

        _translucencyCheck = false;

        // Disable culling for translucent items
        if (_translucencySource != TranslucencySource::COUNT) {
            setDoubleSided(true);
        } else {
            if (requestRecomputeShaders) {
                recomputeShaders();
            }
        }
    }
}

void Material::getSortKeys(RenderStagePass renderStagePass, I32& shaderKey, I32& textureKey) const {
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
    retMatrix.setRow(2, FColour(_colourData._emissive.rgb(), _colourData._shininess));
    retMatrix.setRow(3, vec4<F32>(to_F32(getTextureOperation()), getParallaxFactor(), 0.0, 0.0));
}

void Material::rebuild() {
    recomputeShaders();

    for (RenderStagePass::PassIndex i = 0; i < RenderStagePass::count(); ++i) {
        computeShader(RenderStagePass::stagePass(i), _highPriority);
        _shaderInfo[i]._shaderRef->recompile();
    }
}

namespace {
    const char* getTexUsageName(ShaderProgram::TextureUsage texUsage) {
        switch (texUsage) {
            case ShaderProgram::TextureUsage::UNIT0      : return "UNIT0";
            case ShaderProgram::TextureUsage::UNIT1      : return "UNIT1";
            case ShaderProgram::TextureUsage::NORMALMAP  : return "NORMALMAP";
            case ShaderProgram::TextureUsage::OPACITY    : return "OPACITY";
            case ShaderProgram::TextureUsage::SPECULAR   : return "SPECULAR";
            case ShaderProgram::TextureUsage::PROJECTION : return "PROJECTION";
        };

        return "";
    }

    const char *getBumpMethodName(Material::BumpMethod bumpMethod) {
        switch(bumpMethod) {
            case Material::BumpMethod::NORMAL   : return "NORMAL";
            case Material::BumpMethod::PARALLAX : return "PARALLAX";
            case Material::BumpMethod::RELIEF   : return "RELIEF";
        }

        return "NONE";
    }

    Material::BumpMethod getBumpMethodByName(const stringImpl& name) {
        if (Util::CompareIgnoreCase(name, "NORMAL")) {
            return Material::BumpMethod::NORMAL;
        } else if (Util::CompareIgnoreCase(name, "PARALLAX")) {
            return Material::BumpMethod::PARALLAX;
        } else if (Util::CompareIgnoreCase(name, "RELIEF")) {
            return Material::BumpMethod::RELIEF;
        }

        return Material::BumpMethod::COUNT;
    }

    const char *getShadingModeName(Material::ShadingMode shadingMode) {
        switch (shadingMode) {
            case Material::ShadingMode::FLAT          : return "FLAT";
            case Material::ShadingMode::PHONG         : return "PHONG";
            case Material::ShadingMode::BLINN_PHONG   : return "BLINN_PHONG";
            case Material::ShadingMode::TOON          : return "TOON";
            case Material::ShadingMode::OREN_NAYAR    : return "OREN_NAYAR";
            case Material::ShadingMode::COOK_TORRANCE : return "COOK_TORRANCE";
        }

        return "NONE";
    }

    Material::ShadingMode getShadingModeByName(const stringImpl& name) {
        if (Util::CompareIgnoreCase(name, "FLAT")) {
            return Material::ShadingMode::FLAT;
        } else if (Util::CompareIgnoreCase(name, "PHONG")) {
            return Material::ShadingMode::PHONG;
        } else if (Util::CompareIgnoreCase(name, "BLINN_PHONG")) {
            return Material::ShadingMode::BLINN_PHONG;
        } else if (Util::CompareIgnoreCase(name, "TOON")) {
             return Material::ShadingMode::TOON;
        } else if (Util::CompareIgnoreCase(name, "OREN_NAYAR")) {
             return Material::ShadingMode::OREN_NAYAR;
        } else if (Util::CompareIgnoreCase(name, "COOK_TORRANCE")) {
             return Material::ShadingMode::COOK_TORRANCE;
        }

        return Material::ShadingMode::COUNT;
    }

    Material::TextureOperation getTextureOperationByName(const stringImpl& operation) {
        if (Util::CompareIgnoreCase(operation, "TEX_OP_MULTIPLY")) {
            return Material::TextureOperation::MULTIPLY;
        } else if (Util::CompareIgnoreCase(operation, "TEX_OP_DECAL")) {
            return Material::TextureOperation::DECAL;
        } else if (Util::CompareIgnoreCase(operation, "TEX_OP_ADD")) {
            return Material::TextureOperation::ADD;
        } else if (Util::CompareIgnoreCase(operation, "TEX_OP_SMOOTH_ADD")) {
            return Material::TextureOperation::SMOOTH_ADD;
        } else if (Util::CompareIgnoreCase(operation, "TEX_OP_SIGNED_ADD")) {
            return Material::TextureOperation::SIGNED_ADD;
        } else if (Util::CompareIgnoreCase(operation, "TEX_OP_DIVIDE")) {
            return Material::TextureOperation::DIVIDE;
        } else if (Util::CompareIgnoreCase(operation, "TEX_OP_SUBTRACT")) {
            return Material::TextureOperation::SUBTRACT;
        }

        return Material::TextureOperation::REPLACE;
    }

    const char *getTextureOperationName(Material::TextureOperation textureOp) {
        switch(textureOp) {
            case Material::TextureOperation::MULTIPLY   : return "TEX_OP_MULTIPLY";
            case Material::TextureOperation::DECAL      : return "TEX_OP_DECAL";
            case Material::TextureOperation::ADD        : return "TEX_OP_ADD";
            case Material::TextureOperation::SMOOTH_ADD : return "TEX_OP_SMOOTH_ADD";
            case Material::TextureOperation::SIGNED_ADD : return "TEX_OP_SIGNED_ADD";
            case Material::TextureOperation::DIVIDE     : return "TEX_OP_DIVIDE";
            case Material::TextureOperation::SUBTRACT   : return "TEX_OP_SUBTRACT";
        }

        return "TEX_OP_REPLACE";
    }

    const char *getWrapModeName(TextureWrap wrapMode) {
        switch(wrapMode) {
            case TextureWrap::CLAMP           : return "CLAMP";
            case TextureWrap::CLAMP_TO_EDGE   : return "CLAMP_TO_EDGE";
            case TextureWrap::CLAMP_TO_BORDER : return "CLAMP_TO_BORDER";
            case TextureWrap::DECAL           : return "DECAL";
        }

        return "REPEAT";
    }

    TextureWrap getWrapModeByName(const stringImpl& wrapMode) {
        if (Util::CompareIgnoreCase(wrapMode, "CLAMP")) {
            return TextureWrap::CLAMP;
        } else if (Util::CompareIgnoreCase(wrapMode, "CLAMP_TO_EDGE")) {
            return TextureWrap::CLAMP_TO_EDGE;
        } else if (Util::CompareIgnoreCase(wrapMode, "CLAMP_TO_BORDER")) {
            return TextureWrap::CLAMP_TO_BORDER;
        } else if (Util::CompareIgnoreCase(wrapMode, "DECAL")) {
            return TextureWrap::DECAL;
        }

        return TextureWrap::REPEAT;
    }

    const char *getFilterName(TextureFilter filter) {
        switch(filter) {
            case TextureFilter::LINEAR: return "LINEAR";
            case TextureFilter::NEAREST_MIPMAP_NEAREST : return "NEAREST_MIPMAP_NEAREST";
            case TextureFilter::LINEAR_MIPMAP_NEAREST : return "LINEAR_MIPMAP_NEAREST";
            case TextureFilter::NEAREST_MIPMAP_LINEAR : return "NEAREST_MIPMAP_LINEAR";
            case TextureFilter::LINEAR_MIPMAP_LINEAR : return "LINEAR_MIPMAP_LINEAR";
        }

        return "NEAREST";
    }


    TextureFilter getFilterByName(const stringImpl& filter) {
        if (Util::CompareIgnoreCase(filter, "LINEAR")) {
            return TextureFilter::LINEAR;
        } else if (Util::CompareIgnoreCase(filter, "NEAREST_MIPMAP_NEAREST")) {
            return TextureFilter::NEAREST_MIPMAP_NEAREST;
        } else if (Util::CompareIgnoreCase(filter, "LINEAR_MIPMAP_NEAREST")) {
            return TextureFilter::LINEAR_MIPMAP_NEAREST;
        } else if (Util::CompareIgnoreCase(filter, "NEAREST_MIPMAP_LINEAR")) {
            return TextureFilter::NEAREST_MIPMAP_LINEAR;
        } else if (Util::CompareIgnoreCase(filter, "LINEAR_MIPMAP_LINEAR")) {
            return TextureFilter::LINEAR_MIPMAP_LINEAR;
        }

        return TextureFilter::NEAREST;
    }

    Texture_ptr loadTextureXML(ResourceCache& targetCache,
                               const stringImpl &textureNode,
                               const stringImpl &textureName,
                               const boost::property_tree::ptree& pt)
    {
        stringImpl img_name(textureName.substr(textureName.find_last_of('/') + 1));
        stringImpl pathName(textureName.substr(0, textureName.rfind("/")));

        SamplerDescriptor sampDesc = {};

        sampDesc._wrapU = getWrapModeByName(pt.get<stringImpl>(textureNode + ".Map.<xmlattr>.U", "REPEAT"));
        sampDesc._wrapV = getWrapModeByName(pt.get<stringImpl>(textureNode + ".Map.<xmlattr>.V", "REPEAT"));
        sampDesc._wrapW = getWrapModeByName(pt.get<stringImpl>(textureNode + ".Map.<xmlattr>.W", "REPEAT"));
        sampDesc._minFilter = getFilterByName(pt.get<stringImpl>(textureNode + ".Filter.<xmlattr>.min", "LINEAR"));
        sampDesc._magFilter = getFilterByName(pt.get<stringImpl>(textureNode + ".Filter.<xmlattr>.mag", "LINEAR"));
        sampDesc._anisotropyLevel = to_U8(pt.get(textureNode + ".anisotropy", 0U));

        TextureDescriptor texDesc(TextureType::TEXTURE_2D);
        texDesc.setSampler(sampDesc);

        ResourceDescriptor texture(pathName + "/" + img_name);
        texture.assetName(img_name);
        texture.assetLocation(pathName);
        texture.setPropertyDescriptor(texDesc);
        texture.setFlag(!pt.get(textureNode + ".flipped", false));

        return CreateResource<Texture>(targetCache, texture);
    }
};

void Material::saveToXML(const stringImpl& entryName, boost::property_tree::ptree& pt) const {
    pt.put(entryName + ".colour.<xmlattr>.r", getColourData()._diffuse.r);
    pt.put(entryName + ".colour.<xmlattr>.g", getColourData()._diffuse.g);
    pt.put(entryName + ".colour.<xmlattr>.b", getColourData()._diffuse.b);
    pt.put(entryName + ".colour.<xmlattr>.a", getColourData()._diffuse.a);

    pt.put(entryName + ".emissive.<xmlattr>.r", getColourData()._emissive.r);
    pt.put(entryName + ".emissive.<xmlattr>.g", getColourData()._emissive.g);
    pt.put(entryName + ".emissive.<xmlattr>.b", getColourData()._emissive.b);

    pt.put(entryName + ".specular.<xmlattr>.r", getColourData()._specular.r);
    pt.put(entryName + ".specular.<xmlattr>.g", getColourData()._specular.g);
    pt.put(entryName + ".specular.<xmlattr>.b", getColourData()._specular.b);

    pt.put(entryName + ".shininess", getColourData()._shininess);

    pt.put(entryName + ".doubleSided", isDoubleSided());

    pt.put(entryName + ".bumpMethod", getBumpMethodName(getBumpMethod()));

    pt.put(entryName + ".shadingMode", getShadingModeName(getShadingMode()));

    pt.put(entryName + ".parallaxFactor", getParallaxFactor());

    for (U8 i = 0; i <= to_U8(ShaderProgram::TextureUsage::PROJECTION); ++i) {
        ShaderProgram::TextureUsage usage = static_cast<ShaderProgram::TextureUsage>(i);
        Texture_wptr tex = getTexture(usage);
        if (!tex.expired()) {
            Texture_ptr texture = tex.lock();

            const SamplerDescriptor &sampler = texture->getCurrentSampler();

            stringImpl textureNode = entryName + ".texture.";
            textureNode += getTexUsageName(usage);

            pt.put(textureNode + ".name", texture->assetName());
            pt.put(textureNode + ".path", texture->assetLocation());
            pt.put(textureNode + ".flipped", texture->flipped());

            if (usage == ShaderProgram::TextureUsage::UNIT1) {
                pt.put(textureNode + ".usage", getTextureOperationName(_operation));
            }
            pt.put(textureNode + ".Map.<xmlattr>.U", getWrapModeName(sampler._wrapU));
            pt.put(textureNode + ".Map.<xmlattr>.V", getWrapModeName(sampler._wrapV));
            pt.put(textureNode + ".Map.<xmlattr>.W", getWrapModeName(sampler._wrapW));
            pt.put(textureNode + ".Filter.<xmlattr>.min", getFilterName(sampler._minFilter));
            pt.put(textureNode + ".Filter.<xmlattr>.mag", getFilterName(sampler._magFilter));
            pt.put(textureNode + ".anisotropy", to_U32(sampler._anisotropyLevel));
        }
    }
}

void Material::loadFromXML(const stringImpl& entryName, const boost::property_tree::ptree& pt) {
    if (ignoreXMLData()) {
        return;
    }

    setDiffuse(FColour(pt.get<F32>(entryName + ".colour.<xmlattr>.r", 0.6f),
                       pt.get<F32>(entryName + ".colour.<xmlattr>.g", 0.6f),
                       pt.get<F32>(entryName + ".colour.<xmlattr>.b", 0.6f),
                       pt.get<F32>(entryName + ".colour.<xmlattr>.a", 1.f)));

    setEmissive(FColour(pt.get<F32>(entryName + ".emissive.<xmlattr>.r", 0.f),
                        pt.get<F32>(entryName + ".emissive.<xmlattr>.g", 0.f),
                        pt.get<F32>(entryName + ".emissive.<xmlattr>.b", 0.f)));

    setSpecular(FColour(pt.get<F32>(entryName + ".specular.<xmlattr>.r", 1.f),
                        pt.get<F32>(entryName + ".specular.<xmlattr>.g", 1.f),
                        pt.get<F32>(entryName + ".specular.<xmlattr>.b", 1.f)));

    setShininess(pt.get<F32>(entryName + ".shininess", 0.0f));

    setDoubleSided(pt.get<bool>(entryName + ".doubleSided", false));

    setBumpMethod(getBumpMethodByName(pt.get<stringImpl>(entryName + ".bumpMethod", "NORMAL")));

    setShadingMode(getShadingModeByName(pt.get<stringImpl>(entryName + ".shadingMode", "FLAT")));

    setParallaxFactor(pt.get<F32>(entryName + ".parallaxFactor", 1.0f));


    STUBBED("ToDo: Set texture is currently disabled!");

    for (U8 i = 0; i <= to_U8(ShaderProgram::TextureUsage::PROJECTION); ++i) {
        ShaderProgram::TextureUsage usage = static_cast<ShaderProgram::TextureUsage>(i);

        if (auto child = pt.get_child_optional(((entryName + ".texture.") + getTexUsageName(usage)) + ".name")) {
            stringImpl textureNode = entryName + ".texture.";
            textureNode += getTexUsageName(usage);

            stringImpl texName = pt.get<stringImpl>(textureNode + ".name", "");
            if (!texName.empty()) {
                stringImpl texLocation = pt.get<stringImpl>(textureNode + ".path", "");
                Texture_ptr tex = loadTextureXML(_context.parent().resourceCache(), textureNode, texLocation + "/" + texName, pt);
                if (tex != nullptr) {
                    if (usage == ShaderProgram::TextureUsage::UNIT1) {
                        TextureOperation op = getTextureOperationByName(pt.get<stringImpl>(textureNode + ".usage", "REPLACE"));
                        setTexture(usage, tex, op);
                    } else {
                        setTexture(usage, tex);
                    }
                }
            }
        }
    }
}

};