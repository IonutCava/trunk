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
    const U16 g_numMipsToKeepFromAlphaTextures = 1;
    const char* g_PassThroughMaterialShaderName = "passThrough";

    stringImpl getDefinesHash(const vector<std::pair<stringImpl, bool>>& defines) {
        size_t hash = 17;
        for (auto entry : defines) {
            Util::Hash_combine(hash, _ID(entry.first.c_str()));
            Util::Hash_combine(hash, entry.second);
        }
        return to_stringImpl(hash);
    }
    
    constexpr ShaderProgram::TextureUsage g_materialTextures[] = {
        ShaderProgram::TextureUsage::UNIT0,
        ShaderProgram::TextureUsage::UNIT1,
        ShaderProgram::TextureUsage::NORMALMAP,
        ShaderProgram::TextureUsage::OPACITY,
        ShaderProgram::TextureUsage::SPECULAR,
    };

    constexpr U32 g_materialTexturesCount = 5;
};

SharedMutex Material::s_shaderDBLock;
hashMap<size_t, ShaderProgram_ptr> Material::s_shaderDB;

bool Material::onStartup() {
    return true;
}

bool Material::onShutdown() {
    UniqueLockShared w_lock(s_shaderDBLock);
    s_shaderDB.clear();
    return true;
}

Material::Material(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name)
    : CachedResource(ResourceType::DEFAULT, descriptorHash, name),
      _context(context),
      _parentCache(parentCache),
      _textureKeyCache(-1),
      _parallaxFactor(1.0f),
      _needsNewShader(false),
      _doubleSided(false),
      _translucent(false),
      _receivesShadows(true),
      _isReflective(false),
      _isRefractive(false),
      _hardwareSkinning(false),
      _ignoreXMLData(false),
      _useTriangleStrip(true),
      _shadingMode(ShadingMode::COUNT),
      _bumpMethod(BumpMethod::NONE),
      _translucencySource(TranslucencySource::COUNT)
{
    _textures.fill(nullptr);
    _baseShaderName[0] = "material";
    _baseShaderName[1] = "depthPass";

    _textureExtenalFlag.fill(false);
    _textureExtenalFlag[to_base(ShaderProgram::TextureUsage::REFLECTION_PLANAR)] = true;
    _textureExtenalFlag[to_base(ShaderProgram::TextureUsage::REFRACTION_PLANAR)] = true;
    _textureExtenalFlag[to_base(ShaderProgram::TextureUsage::REFLECTION_CUBE)] = true;
    _textureExtenalFlag[to_base(ShaderProgram::TextureUsage::REFRACTION_CUBE)] = true;
    _textureExtenalFlag[to_base(ShaderProgram::TextureUsage::DEPTH)] = true;
    _textureExtenalFlag[to_base(ShaderProgram::TextureUsage::DEPTH_PREV)] = true;

    _operation = TextureOperation::NONE;

    /// Normal state for final rendering
    RenderStateBlock stateDescriptor;
    stateDescriptor.setZFunc(ComparisonFunction::LEQUAL);

    RenderStateBlock oitDescriptor(stateDescriptor);
    oitDescriptor.depthTestEnabled(true);

    /// the reflection descriptor is the same as the normal descriptor
    RenderStateBlock reflectorDescriptor(stateDescriptor);
    RenderStateBlock reflectorOitDescriptor(stateDescriptor);
    //reflectorOitDescriptor.depthTestEnabled(false);

    /// the z-pre-pass descriptor does not process colours
    RenderStateBlock zPrePassDescriptor(stateDescriptor);
    zPrePassDescriptor.setColourWrites(false, false, false, false);
    zPrePassDescriptor.setZFunc(ComparisonFunction::LESS);

    /// A descriptor used for rendering to depth map
    RenderStateBlock shadowDescriptor(stateDescriptor);

    shadowDescriptor.setCullMode(CullMode::CCW);
    shadowDescriptor.setZFunc(ComparisonFunction::LESS);
    /// set a polygon offset
    shadowDescriptor.setZBias(1.0f, 1.0f);

    RenderStateBlock shadowDescriptorNoColour(shadowDescriptor);
    shadowDescriptorNoColour.setColourWrites(false, false, false, false);

    RenderStateBlock shadowDescriptorCSM(shadowDescriptor);
    shadowDescriptorCSM.setCullMode(CullMode::CW);
    shadowDescriptorCSM.setZFunc(ComparisonFunction::LESS);
    shadowDescriptorCSM.setColourWrites(true, true, false, false);

    setRenderStateBlock(stateDescriptor.getHash(), RenderStagePass(RenderStage::DISPLAY, RenderPassType::MAIN_PASS));
    setRenderStateBlock(stateDescriptor.getHash(), RenderStagePass(RenderStage::REFRACTION, RenderPassType::MAIN_PASS));
    setRenderStateBlock(reflectorDescriptor.getHash(), RenderStagePass(RenderStage::REFLECTION, RenderPassType::MAIN_PASS));

    setRenderStateBlock(shadowDescriptorNoColour.getHash(), RenderStage::SHADOW, to_base(LightType::POINT));
    setRenderStateBlock(shadowDescriptorNoColour.getHash(), RenderStage::SHADOW, to_base(LightType::SPOT));
    setRenderStateBlock(shadowDescriptorCSM.getHash(), RenderStage::SHADOW, to_base(LightType::DIRECTIONAL));

    setRenderStateBlock(oitDescriptor.getHash(), RenderStagePass(RenderStage::DISPLAY, RenderPassType::OIT_PASS));
    setRenderStateBlock(oitDescriptor.getHash(), RenderStagePass(RenderStage::REFRACTION, RenderPassType::OIT_PASS));
    setRenderStateBlock(reflectorOitDescriptor.getHash(), RenderStagePass(RenderStage::REFLECTION, RenderPassType::OIT_PASS));

    setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStagePass(RenderStage::DISPLAY, RenderPassType::PRE_PASS));
    setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStagePass(RenderStage::REFRACTION, RenderPassType::PRE_PASS));
    setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStagePass(RenderStage::REFLECTION, RenderPassType::PRE_PASS));
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
    cloneMat->_useTriangleStrip = base._useTriangleStrip;
    cloneMat->_doubleSided = base._doubleSided;
    cloneMat->_translucent = base._translucent;
    cloneMat->_receivesShadows = base._receivesShadows;
    cloneMat->_isReflective = base._isReflective;
    cloneMat->_isRefractive = base._isRefractive;
    cloneMat->_hardwareSkinning = base._hardwareSkinning;
    cloneMat->_operation = base._operation;
    cloneMat->_bumpMethod = base._bumpMethod;
    cloneMat->_ignoreXMLData = base._ignoreXMLData;
    cloneMat->_parallaxFactor = base._parallaxFactor;
    cloneMat->_translucencySource = base._translucencySource;
    cloneMat->_baseShaderName = base._baseShaderName;
    cloneMat->_extraShaderDefines = base._extraShaderDefines;

    for (RenderStagePass::StagePassIndex i = 0; i < RenderStagePass::count(); ++i) {
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
    if (_needsNewShader) {
        recomputeShaders();
        _needsNewShader = false;
    }
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
    
    assert(textureUsageSlot != ShaderProgram::TextureUsage::REFLECTION_PLANAR &&
           textureUsageSlot != ShaderProgram::TextureUsage::REFRACTION_PLANAR &&
           textureUsageSlot != ShaderProgram::TextureUsage::REFLECTION_CUBE &&
           textureUsageSlot != ShaderProgram::TextureUsage::REFRACTION_CUBE);

    {
        UniqueLockShared w_lock(_textureLock);
        if (!_textures[slot]) {
            // if we add a new type of texture recompute shaders
            computeShaders = true;
        } else {
            // Skip adding same texture
            if (texture != nullptr && _textures[slot]->getGUID() == texture->getGUID()) {
                return true;
            }
        }
    
        _textures[slot] = texture;
    }

    if (textureUsageSlot == ShaderProgram::TextureUsage::UNIT0 ||
        textureUsageSlot == ShaderProgram::TextureUsage::OPACITY)
    {
        if (textureUsageSlot == ShaderProgram::TextureUsage::UNIT0) {
            _textureKeyCache = texture == nullptr ? -1 : texture->getHandle();
        }

        // If we have the opacity texture is the albedo map, we don't need it. We can just use albedo alpha
        const Texture_ptr& opacityTex = _textures[to_base(ShaderProgram::TextureUsage::OPACITY)];
        if (opacityTex != nullptr && texture != nullptr && opacityTex->getData() == texture->getData()) {
            _textures[to_base(ShaderProgram::TextureUsage::OPACITY)] = nullptr;
        }

        updateTranslucency();
    }

    _needsNewShader = true;

    return true;
}

void Material::waitForShader(const ShaderProgram_ptr& shader, RenderStagePass stagePass, const char* newShader) {
    if (shader == nullptr) {
        return;
    }

    ShaderProgram* oldShader = shader.get();
    /*{
        SharedLock r_lock(s_shaderDBLock);
        auto it = s_shaderDB.find(shaderHash);
        if (it != std::cend(s_shaderDB)) {
            oldShader = it->second.get();
        }
    }*/

    if (oldShader == nullptr) {
        return;
    }

    if (newShader == nullptr || strlen(newShader) == 0 || oldShader->resourceName().compare(newShader) != 0) {
        // We cannot replace a shader that is still loading in the background
        WAIT_FOR_CONDITION(oldShader->getState() == ResourceState::RES_LOADED);
        Console::printfn(Locale::get(_ID("REPLACE_SHADER")),
            oldShader->resourceName().c_str(),
            newShader != nullptr ? newShader : "NULL",
            TypeUtil::renderStageToString(stagePass._stage),
            TypeUtil::renderPassTypeToString(stagePass._passType));
    }
}

void Material::setShaderProgramInternal(const ShaderProgram_ptr& shader,
                                        RenderStagePass renderStagePass) {
    ShaderProgramInfo& info = shaderInfo(renderStagePass);
    info._shaderRef = shader;
    // if we already have a different shader assigned ...
    waitForShader(info._shaderRef, renderStagePass, shader == nullptr ? nullptr : shader->resourceName().c_str());

    //info._shaderRefHash = shader == nullptr ? 0 : shader->getDescriptorHash();

    /*if (info._shaderRefHash != 0) {
        UniqueLockShared w_lock(s_shaderDBLock);
        hashAlg::insert(s_shaderDB, info._shaderRefHash, shader);
    }*/

    info._shaderCompStage = ShaderProgramInfo::BuildStage::COMPUTED;
    if (shader == nullptr || shader->getState() == ResourceState::RES_LOADED) {
        info._shaderCompStage = ShaderProgramInfo::BuildStage::READY;
        info._shaderCache = shader.get();
    }
}

void Material::setShaderProgramInternal(const ResourceDescriptor& shaderDescriptor,
                                        RenderStagePass renderStagePass,
                                        const bool computeOnAdd) {
    ShaderProgramInfo& info = shaderInfo(renderStagePass);
    // if we already have a different shader assigned ...
    if (info._shaderRef != nullptr && info._shaderRef->resourceName().compare(shaderDescriptor.resourceName()) != 0)
    {
        // We cannot replace a shader that is still loading in the background
        WAIT_FOR_CONDITION(info._shaderRef->getState() == ResourceState::RES_LOADED);
        Console::printfn(Locale::get(_ID("REPLACE_SHADER")),
            info._shaderRef->resourceName().c_str(), 
            shaderDescriptor.resourceName().c_str());
    }

    //UniqueLockShared w_lock(s_shaderDBLock);
    //auto ret = hashAlg::insert(s_shaderDB, info._shaderRefHash, ShaderProgram_ptr());

    ShaderComputeQueue& shaderQueue = _context.shaderComputeQueue();
    
    if (computeOnAdd)
    {
        shaderQueue.addToQueueFront(ShaderComputeQueue::ShaderQueueElement{info._shaderRef/*ret.first->second*/, shaderDescriptor});
        shaderQueue.stepQueue();
        info._shaderCompStage = ShaderProgramInfo::BuildStage::COMPUTED;
    }
    else
    {
        shaderQueue.addToQueueBack(ShaderComputeQueue::ShaderQueueElement{ info._shaderRef/*ret.first->second*/, shaderDescriptor });
        info._shaderCompStage = ShaderProgramInfo::BuildStage::QUEUED;
    }
}

void Material::recomputeShaders() {
    for (RenderStagePass::StagePassIndex i = 0; i < RenderStagePass::count(); ++i) {
        ShaderProgramInfo& info = _shaderInfo[i];
        if (!info._customShader) {
            info._shaderCompStage = ShaderProgramInfo::BuildStage::REQUESTED;
            computeShader(RenderStagePass::stagePass(i));
        }
    }
}

U32 Material::getProgramID(RenderStagePass renderStagePass) const {
    const ShaderProgramInfo& info = shaderInfo(renderStagePass);
    /*if (info._shaderCache == nullptr) {
        return info._shaderCache->getID();
    }*/
    if (info._shaderRef != nullptr && info._shaderRef->getState() == ResourceState::RES_LOADED) {
        return info._shaderRef->getID();
    }
    /*size_t hash = shaderInfo(renderStagePass)._shaderRefHash;

    SharedLock r_lock(s_shaderDBLock);
    auto it = s_shaderDB.find(hash);
    if (it != std::cend(s_shaderDB)) {
        return it->second->getID();
    }*/

    return ShaderProgram::defaultShader()->getID();
}

bool Material::canDraw(RenderStagePass renderStagePass) {
    for (U8 i = 0; i < to_U8(RenderPassType::COUNT); ++i) {
        RenderStagePass stagePassIt(renderStagePass._stage, static_cast<RenderPassType>(i));
        ShaderProgramInfo& info = shaderInfo(stagePassIt);

        if (info._shaderCompStage == ShaderProgramInfo::BuildStage::QUEUED && info._shaderRef != nullptr) {
            info._shaderCompStage = ShaderProgramInfo::BuildStage::COMPUTED;
        }

        if (info._shaderCompStage == ShaderProgramInfo::BuildStage::COMPUTED) {
            if (info._shaderRef != nullptr && info._shaderRef->getState() == ResourceState::RES_LOADED) {
                info._shaderCompStage = ShaderProgramInfo::BuildStage::READY;
                return false;
            }
        }

        if (info._shaderCompStage != ShaderProgramInfo::BuildStage::READY) {
            return computeShader(stagePassIt);
        }
    }

    return true;
}

/// If the current material doesn't have a shader associated with it, then add the default ones.
bool Material::computeShader(RenderStagePass renderStagePass) {

    ShaderProgramInfo& info = shaderInfo(renderStagePass);
    // If shader's invalid, try to request a recompute as it might fix it
    if (info._shaderCompStage == ShaderProgramInfo::BuildStage::COUNT) {
        info._shaderCompStage = ShaderProgramInfo::BuildStage::REQUESTED;
        return false;
    }

    // If the shader is valid and a recompute wasn't requested, just return true
    if (info._shaderCompStage != ShaderProgramInfo::BuildStage::REQUESTED) {
        return info._shaderCompStage == ShaderProgramInfo::BuildStage::READY;
    }

    // At this point, only computation requests are processed
    assert(info._shaderCompStage == ShaderProgramInfo::BuildStage::REQUESTED);

    if (Config::Profile::DISABLE_SHADING) {
        setShaderProgramInternal(ResourceDescriptor(g_PassThroughMaterialShaderName), renderStagePass, false);
        return false;
    }


    const U32 slot0 = to_base(ShaderProgram::TextureUsage::UNIT0);
    const U32 slot1 = to_base(ShaderProgram::TextureUsage::UNIT1);
    const U32 slotOpacity = to_base(ShaderProgram::TextureUsage::OPACITY);

    DIVIDE_ASSERT(_shadingMode != ShadingMode::COUNT,
                  "Material computeShader error: Invalid shading mode specified!");

    ShaderProgramDescriptor shaderPropertyDescriptor;

    shaderPropertyDescriptor._defines.insert(std::cbegin(shaderPropertyDescriptor._defines), std::cbegin(_extraShaderDefines), std::cend(_extraShaderDefines));


    if (_textures[slot1]) {
        if (!_textures[slot0]) {
            std::swap(_textures[slot0], _textures[slot1]);
            updateTranslucency();
        }
    }

    stringImpl shader = _baseShaderName[renderStagePass.isDepthPass() ? 1 : 0];

    if (renderStagePass.isDepthPass()) {
        if (renderStagePass._stage == RenderStage::SHADOW) {
            shader += ".Shadow";
            shaderPropertyDescriptor._defines.push_back(std::make_pair("SHADOW_PASS", true));
        } else {
            shader += ".PrePass";
            shaderPropertyDescriptor._defines.push_back(std::make_pair("PRE_PASS", true));
        }
    }

    if (renderStagePass._passType == RenderPassType::OIT_PASS) {
        shader += ".OIT";
        shaderPropertyDescriptor._defines.push_back(std::make_pair("OIT_PASS", true));
    }

    if (!renderStagePass.isDepthPass()) {
        // What kind of effects do we need?
        if (_textures[slot0]) {
            // Bump mapping?
            if (_textures[to_base(ShaderProgram::TextureUsage::NORMALMAP)] &&  _bumpMethod != BumpMethod::NONE) {
                shaderPropertyDescriptor._defines.push_back(std::make_pair("COMPUTE_TBN", true));
                shader += ".NormalMap";  // Normal Mapping
                if (_bumpMethod == BumpMethod::PARALLAX) {
                    shaderPropertyDescriptor._defines.push_back(std::make_pair("USE_PARALLAX_MAPPING", true));
                    shader += ".ParallaxMap";
                } else if (_bumpMethod == BumpMethod::RELIEF) {
                    shaderPropertyDescriptor._defines.push_back(std::make_pair("USE_RELIEF_MAPPING", true));
                    shader += ".ReliefMap";
                }
            }
        } else {
            shaderPropertyDescriptor._defines.push_back(std::make_pair("SKIP_TEXTURES", true));
            shader += ".NoTexture";
        }

        if (_textures[to_base(ShaderProgram::TextureUsage::SPECULAR)]) {
            shader += ".SpecularMap";
            shaderPropertyDescriptor._defines.push_back(std::make_pair("USE_SPECULAR_MAP", true));
        }
    }

    updateTranslucency();

    if (_translucencySource != TranslucencySource::COUNT && renderStagePass._passType != RenderPassType::OIT_PASS) {
        shader += ".AlphaDiscard";
        shaderPropertyDescriptor._defines.push_back(std::make_pair("USE_ALPHA_DISCARD", true));
    }

    switch (_translucencySource) {
        case TranslucencySource::OPACITY_MAP: {
            shader += ".OpacityMap";
            shaderPropertyDescriptor._defines.push_back(std::make_pair("USE_OPACITY_MAP", true));
        } break;
        case TranslucencySource::ALBEDO: {
            shader += ".AlbedoAlpha";
            shaderPropertyDescriptor._defines.push_back(std::make_pair("USE_ALBEDO_ALPHA", true));
        } break;
        default: break;
    };

    if (isDoubleSided()) {
        shader += ".DoubleSided";
        shaderPropertyDescriptor._defines.push_back(std::make_pair("USE_DOUBLE_SIDED", true));
    }

    if (!receivesShadows() || !_context.context().config().rendering.shadowMapping.enabled) {
        shader += ".NoShadows";
        shaderPropertyDescriptor._defines.push_back(std::make_pair("DISABLE_SHADOW_MAPPING", true));
    }

    // Add the GPU skinning module to the vertex shader?
    if (_hardwareSkinning) {
        shaderPropertyDescriptor._defines.push_back(std::make_pair("USE_GPU_SKINNING", true));
        shader += ",Skinned";  //<Use "," instead of "." will add a Vertex only property
    }

    switch (_shadingMode) {
        default:
        case ShadingMode::FLAT: {
            shaderPropertyDescriptor._defines.push_back(std::make_pair("USE_SHADING_FLAT", true));
            shader += ".Flat";
        } break;
        /*case ShadingMode::PHONG: {
            shaderPropertyDescriptor._defines.push_back(std::make_pair("USE_SHADING_PHONG", true));
            shader += ".Phong";
        } break;*/
        case ShadingMode::PHONG:
        case ShadingMode::BLINN_PHONG: {
            shaderPropertyDescriptor._defines.push_back(std::make_pair("USE_SHADING_BLINN_PHONG", true));
            shader += ".BlinnPhong";
        } break;
        case ShadingMode::TOON: {
            shaderPropertyDescriptor._defines.push_back(std::make_pair("USE_SHADING_TOON", true));
            shader += ".Toon";
        } break;
        case ShadingMode::OREN_NAYAR: {
            shaderPropertyDescriptor._defines.push_back(std::make_pair("USE_SHADING_OREN_NAYAR", true));
            shader += ".OrenNayar";
        } break;
        case ShadingMode::COOK_TORRANCE: {
            shaderPropertyDescriptor._defines.push_back(std::make_pair("USE_SHADING_COOK_TORRANCE", true));
            shader += ".CookTorrance";
        } break;
    }

    
    stringImpl shaderName = shader;
    if (!shaderPropertyDescriptor._defines.empty()) {
        shaderName.append(getDefinesHash(shaderPropertyDescriptor._defines));
    }

    ResourceDescriptor shaderDescriptor(shader);
    shaderDescriptor.assetName(shaderName);
    shaderPropertyDescriptor._defines.push_back(std::make_pair("DEFINE_PLACEHOLDER", false));
    shaderDescriptor.setPropertyDescriptor(shaderPropertyDescriptor);
    shaderDescriptor.setThreadedLoading(true);

    setShaderProgramInternal(shaderDescriptor, renderStagePass, false);

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

bool Material::getTextureData(ShaderProgram::TextureUsage slot, TextureDataContainer& container, bool force) {
    const U8 slotValue = to_U8(slot);

    SharedLock r_lock(_textureLock);
    const Texture_ptr& crtTexture = _textures[slotValue];

    return crtTexture != nullptr && container.setTexture(crtTexture->getData(), slotValue, force) != TextureDataContainer::UpdateState::NOTHING;
}

bool Material::getTextureData(RenderStagePass renderStagePass, TextureDataContainer& textureData) {
    if (textureData.empty()) {
        return getTextureDataFast(renderStagePass, textureData);
    }

    const bool depthStage = renderStagePass.isDepthPass();

    bool ret = false;
    if (!depthStage || hasTransparency()) {
        ret = getTextureData(ShaderProgram::TextureUsage::UNIT0, textureData) || ret;
        ret = getTextureData(ShaderProgram::TextureUsage::OPACITY, textureData) || ret;
    }

    ret = getTextureData(ShaderProgram::TextureUsage::HEIGHTMAP, textureData) || ret;
    
    if (!depthStage) {
        ret = getTextureData(ShaderProgram::TextureUsage::NORMALMAP, textureData) || ret;
        ret = getTextureData(ShaderProgram::TextureUsage::UNIT1, textureData) || ret;
        ret = getTextureData(ShaderProgram::TextureUsage::SPECULAR, textureData) || ret;
    }

    for (const ExternalTexture& tex : _externalTextures) {
        if (!depthStage || (depthStage && tex._activeForDepth)) {
            if (textureData.setTexture(tex._texture->getData(), to_U8(tex._bindSlot), true) != TextureDataContainer::UpdateState::NOTHING) {
                ret = true;
            }
        }
    }

    return ret;
}

bool Material::getTextureDataFast(RenderStagePass renderStagePass, TextureDataContainer& textureData) {
    bool ret = false;

    TextureDataContainer::DataEntries& textures = textureData.textures();

    constexpr U8 transparentSlots[] = {
        to_base(ShaderProgram::TextureUsage::UNIT0),
        to_base(ShaderProgram::TextureUsage::OPACITY)
    };

    constexpr U8 extraSlots[] = {
        to_base(ShaderProgram::TextureUsage::NORMALMAP),
        to_base(ShaderProgram::TextureUsage::UNIT1),
        to_base(ShaderProgram::TextureUsage::SPECULAR)
    };

    {
        U8 heightSlot = to_base(ShaderProgram::TextureUsage::HEIGHTMAP);
        SharedLock r_lock(_textureLock);
        const Texture_ptr& crtTexture = _textures[heightSlot];
        if (crtTexture != nullptr) {
            textures[heightSlot] = crtTexture->getData();
            ret = true;
        }
    }

    const bool depthStage = renderStagePass.isDepthPass();
    if (!depthStage || hasTransparency()) {
        SharedLock r_lock(_textureLock);
        for (U8 slot : transparentSlots) {
            const Texture_ptr& crtTexture = _textures[slot];
            if (crtTexture != nullptr) {
                textures[slot] = crtTexture->getData();
                ret = true;
            }
        }
    }

    if (!depthStage) {
        SharedLock r_lock(_textureLock);
        for (U8 slot : extraSlots) {
            const Texture_ptr& crtTexture = _textures[slot];
            if (crtTexture != nullptr) {
                textures[slot] = crtTexture->getData();
                ret = true;
            }
        }
    }

    for (const ExternalTexture& tex : _externalTextures) {
        if (!depthStage || (depthStage && tex._activeForDepth)) {
            textures[to_U8(tex._bindSlot)] = tex._texture->getData();
            ret = true;
        }
    }

    return ret;
}

bool Material::unload() noexcept {
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
    _needsNewShader = true;
}

void Material::setRefractive(const bool state) {
    if (_isRefractive == state) {
        return;
    }

    _isRefractive = state;
    _needsNewShader = true;
}

void Material::useTriangleStrip(const bool state) {
    if (_useTriangleStrip == state) {
        return;
    }
    _useTriangleStrip = state;
    _needsNewShader = true;
}

void Material::setDoubleSided(const bool state) {
    if (_doubleSided == state) {
        return;
    }

    _doubleSided = state;

    _needsNewShader = true;
}

void Material::setReceivesShadows(const bool state) {
    if (_receivesShadows == state) {
        return;
    }

    _receivesShadows = state;
    _needsNewShader = true;
}

void Material::updateTranslucency() {
    bool wasTranslucent = _translucent;
    TranslucencySource oldSource = _translucencySource;
    _translucencySource = TranslucencySource::COUNT;

    // In order of importance (less to more)!
    // diffuse channel alpha
    if (_colourData._diffuse.a < 0.95f) {
        _translucencySource = TranslucencySource::ALBEDO;
        _translucent = true;
    }

    // base texture is translucent
    Texture_ptr& albedo = _textures[to_base(ShaderProgram::TextureUsage::UNIT0)];
    if (albedo && albedo->hasTransparency()) {
        _translucencySource = TranslucencySource::ALBEDO;
        _translucent = albedo->hasTranslucency();

        if (oldSource != _translucencySource) {
            const U16 baseLevel = albedo->getBaseMipLevel();
            const U16 maxLevel = albedo->getMaxMipLevel();
            const U16 baseOffset = maxLevel > g_numMipsToKeepFromAlphaTextures ? g_numMipsToKeepFromAlphaTextures : maxLevel;

            STUBBED("HACK! Limit mip range for textures that have alpha values used by the material -Ionut");
            if (albedo->getMipCount() == maxLevel) {
                albedo->setMipMapRange(baseLevel, baseOffset);
            }
        }
    }

    // opacity map
    const Texture_ptr& opacity = _textures[to_base(ShaderProgram::TextureUsage::OPACITY)];
    if (opacity && opacity->hasTransparency()) {
        _translucencySource = TranslucencySource::OPACITY_MAP;
        _translucent = opacity->hasTranslucency();
    }

    if (oldSource != _translucencySource || wasTranslucent != _translucent) {
        _needsNewShader = true;
    }
}

size_t Material::getRenderStateBlock(RenderStagePass renderStagePass) {
    size_t ret = defaultRenderState(renderStagePass);
    if (_doubleSided) {
        RenderStateBlock tempBlock = RenderStateBlock::get(ret);
        tempBlock.setCullMode(CullMode::NONE);
        return tempBlock.getHash();
    }
    return ret;
}

void Material::getSortKeys(RenderStagePass renderStagePass, I32& shaderKey, I32& textureKey) const {
    static const I16 invalidKey = -std::numeric_limits<I16>::max();
    shaderKey = invalidKey;
    textureKey = _textureKeyCache == -1 ? invalidKey : _textureKeyCache;

    const ShaderProgramInfo& info = shaderInfo(renderStagePass);
    if (info._shaderCompStage == ShaderProgramInfo::BuildStage::READY && info._shaderRef) {
        shaderKey = info._shaderRef->getID();
    }
}

void Material::getMaterialMatrix(mat4<F32>& retMatrix) const {
    retMatrix.setRow(0, _colourData._diffuse);
    retMatrix.setRow(1, _colourData._specular);
    retMatrix.setRow(2, FColour(_colourData._emissive.rgb(), _colourData._shininess));
    retMatrix.setRow(3, vec4<F32>(to_F32(getTextureOperation()), getParallaxFactor(), 0.0, 0.0));
}

void Material::rebuild() {
    recomputeShaders();

    for (ShaderProgramInfo& info : _shaderInfo) {
        if (info._shaderRef != nullptr && info._shaderRef->getState() == ResourceState::RES_LOADED) {
           info._shaderRef->recompile();
        }
    }
}

namespace {
    const char* getTexUsageName(ShaderProgram::TextureUsage texUsage) {
        switch (texUsage) {
            case ShaderProgram::TextureUsage::UNIT0      : return "UNIT0";
            case ShaderProgram::TextureUsage::HEIGHTMAP  : return "HEIGHT";
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

    pt.put(entryName + ".receivesShadows", receivesShadows());

    pt.put(entryName + ".bumpMethod", getBumpMethodName(getBumpMethod()));

    pt.put(entryName + ".shadingMode", getShadingModeName(getShadingMode()));

    pt.put(entryName + ".parallaxFactor", getParallaxFactor());


    for (U8 i = 0; i <= g_materialTexturesCount; ++i) {
        ShaderProgram::TextureUsage usage = g_materialTextures[i];

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

    setReceivesShadows(pt.get<bool>(entryName + ".receivesShadows", true));

    setBumpMethod(getBumpMethodByName(pt.get<stringImpl>(entryName + ".bumpMethod", "NORMAL")));

    setShadingMode(getShadingModeByName(pt.get<stringImpl>(entryName + ".shadingMode", "FLAT")));

    setParallaxFactor(pt.get<F32>(entryName + ".parallaxFactor", 1.0f));


    STUBBED("ToDo: Set texture is currently disabled!");

    for (U8 i = 0; i <= g_materialTexturesCount; ++i) {
        ShaderProgram::TextureUsage usage = g_materialTextures[i];

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