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
    constexpr size_t g_invalidStateHash = std::numeric_limits<size_t>::max();

    constexpr U16 g_numMipsToKeepFromAlphaTextures = 1;
    const char* g_PassThroughMaterialShaderName = "passThrough";
    
    constexpr TextureUsage g_materialTextures[] = {
        TextureUsage::UNIT0,
        TextureUsage::UNIT1,
        TextureUsage::NORMALMAP,
        TextureUsage::OPACITY,
        TextureUsage::SPECULAR,
    };

    constexpr U32 g_materialTexturesCount = sizeof(g_materialTextures) / sizeof(g_materialTextures[0]);
};

bool Material::onStartup() {
    return true;
}

bool Material::onShutdown() {
    return true;
}

void Material::ApplyDefaultStateBlocks(Material& target) {
    /// Normal state for final rendering
    RenderStateBlock stateDescriptor;
    stateDescriptor.setCullMode(CullMode::BACK);
    stateDescriptor.setZFunc(ComparisonFunction::EQUAL);

    RenderStateBlock oitDescriptor(stateDescriptor);
    oitDescriptor.setZFunc(ComparisonFunction::LEQUAL);
    oitDescriptor.depthTestEnabled(true);

    /// the reflection descriptor is the same as the normal descriptor
    RenderStateBlock reflectorDescriptor(stateDescriptor);
    RenderStateBlock reflectorOitDescriptor(oitDescriptor);
    //reflectorOitDescriptor.depthTestEnabled(false);

    /// the z-pre-pass descriptor does not process colours
    RenderStateBlock zPrePassDescriptor(stateDescriptor);
    zPrePassDescriptor.setColourWrites(true, true, true, true);
    zPrePassDescriptor.setZFunc(ComparisonFunction::LEQUAL);

    /// A descriptor used for rendering to depth map
    RenderStateBlock shadowDescriptor(stateDescriptor);
    //shadowDescriptor.setCullMode(CullMode::FRONT); //Do I need this?
    shadowDescriptor.setZFunc(ComparisonFunction::LESS);
    /// set a polygon offset
    shadowDescriptor.setZBias(1.0f, 1.0f);
    shadowDescriptor.setColourWrites(false, false, false, false);

    RenderStateBlock shadowDescriptorCSM(shadowDescriptor);
    shadowDescriptorCSM.setColourWrites(true, true, false, false);

    target.setRenderStateBlock(stateDescriptor.getHash(), RenderStage::DISPLAY, RenderPassType::MAIN_PASS, 0u);
    target.setRenderStateBlock(stateDescriptor.getHash(), RenderStage::REFRACTION, RenderPassType::MAIN_PASS, 0u);
    target.setRenderStateBlock(reflectorDescriptor.getHash(), RenderStage::REFLECTION, RenderPassType::MAIN_PASS, 0u);

    target.setRenderStateBlock(shadowDescriptor.getHash(), RenderStage::SHADOW, RenderPassType::MAIN_PASS, to_base(LightType::POINT));
    target.setRenderStateBlock(shadowDescriptor.getHash(), RenderStage::SHADOW, RenderPassType::MAIN_PASS, to_base(LightType::SPOT));
    target.setRenderStateBlock(shadowDescriptorCSM.getHash(), RenderStage::SHADOW, RenderPassType::MAIN_PASS, to_base(LightType::DIRECTIONAL));

    target.setRenderStateBlock(oitDescriptor.getHash(), RenderStage::DISPLAY, RenderPassType::OIT_PASS, 0u);
    target.setRenderStateBlock(oitDescriptor.getHash(), RenderStage::REFRACTION, RenderPassType::OIT_PASS, 0u);
    target.setRenderStateBlock(reflectorOitDescriptor.getHash(), RenderStage::REFLECTION, RenderPassType::OIT_PASS, 0u);

    target.setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStage::DISPLAY, RenderPassType::PRE_PASS, 0u);
    target.setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStage::REFRACTION, RenderPassType::PRE_PASS, 0u);
    target.setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStage::REFLECTION, RenderPassType::PRE_PASS, 0u);
}

Material::Material(GFXDevice& context, ResourceCache* parentCache, size_t descriptorHash, const Str128& name)
    : CachedResource(ResourceType::DEFAULT, descriptorHash, name),
      _context(context),
      _parentCache(parentCache),
      _textureKeyCache(-1),
      _needsNewShader(false)
{
    _textures.fill(nullptr);
    _textureUseForDepth.fill(false);

    const ShaderProgramInfo defaultShaderInfo = {};
    // Could just direct copy the arrays, but this looks cool
    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        ShaderVariantsPerPass& perPassInfo = _shaderInfo[s];
        StateVariantsPerPass& perPassStates = _defaultRenderStates[s];

        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            perPassInfo[p].fill(defaultShaderInfo);
            perPassStates[p].fill(g_invalidStateHash);
        }
    }

    ApplyDefaultStateBlocks(*this);
}

Material_ptr Material::clone(const Str128& nameSuffix) {
    DIVIDE_ASSERT(!nameSuffix.empty(),
                  "Material error: clone called without a valid name suffix!");

    const Material& base = *this;
    Material_ptr cloneMat = CreateResource<Material>(_parentCache, ResourceDescriptor(resourceName() + nameSuffix.c_str()));

    cloneMat->_properties = base._properties;
    cloneMat->_extraShaderDefines = base._extraShaderDefines;

    cloneMat->baseShaderData(base.baseShaderData());
    cloneMat->ignoreXMLData(base.ignoreXMLData());

    // Could just direct copy the arrays, but this looks cool
    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            for (U8 v = 0u; v < g_maxVariantsPerPass; ++v) {
                cloneMat->_shaderInfo[s][p][v] = _shaderInfo[s][p][v];
                cloneMat->_defaultRenderStates[s][p][v] = _defaultRenderStates[s][p][v];
            }
        }
    }

    for (U8 i = 0; i < to_U8(base._textures.size()); ++i) {
        const Texture_ptr& tex = base._textures[i];
        if (tex) {
            cloneMat->setTexture(static_cast<TextureUsage>(i), tex);
        }
    }

    return cloneMat;
}

bool Material::update(const U64 deltaTimeUS) {
    if (_needsNewShader) {
        recomputeShaders();
        _needsNewShader = false;
        return true;
    }

    return false;
}

// base = base texture
// second = second texture used for multitexturing
// bump = bump map
bool Material::setTexture(TextureUsage textureUsageSlot,
                          const Texture_ptr& texture,
                          const TextureOperation& op) {
    bool computeShaders = false;
    const U32 slot = to_U32(textureUsageSlot);

    if (textureUsageSlot == TextureUsage::UNIT1) {
        _properties._operation = op;
    }
    
    assert(textureUsageSlot != TextureUsage::REFLECTION_PLANAR &&
           textureUsageSlot != TextureUsage::REFRACTION_PLANAR &&
           textureUsageSlot != TextureUsage::REFLECTION_CUBE);

    {
        UniqueLock<SharedMutex> w_lock(_textureLock);
        if (!_textures[slot]) {
            // if we add a new type of texture recompute shaders
            computeShaders = true;
        }
        else {
            // Skip adding same texture
            if (texture != nullptr && _textures[slot]->getGUID() == texture->getGUID()) {
                return true;
            }
        }

        _textures[slot] = texture;


        if (textureUsageSlot == TextureUsage::NORMALMAP ||
            textureUsageSlot == TextureUsage::HEIGHTMAP)
        {
            const bool isHeight = textureUsageSlot == TextureUsage::HEIGHTMAP;

            // If we have the opacity texture is the albedo map, we don't need it. We can just use albedo alpha
            const Texture_ptr& otherTex = _textures[isHeight ? to_base(TextureUsage::NORMALMAP)
                                                             : to_base(TextureUsage::HEIGHTMAP)];

            if (otherTex != nullptr && texture != nullptr && otherTex->data().textureHandle() == texture->data().textureHandle()) {
                _textures[to_base(TextureUsage::HEIGHTMAP)] = nullptr;
            }
        }

        if (textureUsageSlot == TextureUsage::UNIT0 ||
            textureUsageSlot == TextureUsage::OPACITY)
        {
            bool isOpacity = true;
            if (textureUsageSlot == TextureUsage::UNIT0) {
                _textureKeyCache = texture == nullptr ? -1 : texture->data().textureHandle();
                isOpacity = false;
            }

            // If we have the opacity texture is the albedo map, we don't need it. We can just use albedo alpha
            const Texture_ptr& otherTex = _textures[isOpacity ? to_base(TextureUsage::UNIT0)
                                                              : to_base(TextureUsage::OPACITY)];

            if (otherTex != nullptr && texture != nullptr && otherTex->data().textureHandle() == texture->data().textureHandle()) {
                _textures[to_base(TextureUsage::OPACITY)] = nullptr;
            }

            updateTranslucency();
        }
    }

    _needsNewShader = true;

    return true;
}

void Material::setShaderProgramInternal(const ShaderProgram_ptr& shader,
                                        ShaderProgramInfo& shaderInfo,
                                        RenderStage stage,
                                        RenderPassType pass)
{
    shaderInfo._shaderRef = shader;
    // if we already have a different shader assigned ...
    if (shader != nullptr) {
        ShaderProgram* oldShader = shader.get();
        if (oldShader != nullptr) {
            const char* newShaderName = shader == nullptr ? nullptr : shader->resourceName().c_str();

            if (newShaderName == nullptr || strlen(newShaderName) == 0 || oldShader->resourceName().compare(newShaderName) != 0) {
                // We cannot replace a shader that is still loading in the background
                WAIT_FOR_CONDITION(oldShader->getState() == ResourceState::RES_LOADED);
                    Console::printfn(Locale::get(_ID("REPLACE_SHADER")),
                        oldShader->resourceName().c_str(),
                        newShaderName != nullptr ? newShaderName : "NULL",
                        TypeUtil::RenderStageToString(stage),
                        TypeUtil::RenderPassTypeToString(pass));
            }
        }
    }

    shaderInfo._shaderCompStage = (shader == nullptr || shader->getState() == ResourceState::RES_LOADED)
                                                     ? ShaderBuildStage::READY
                                                     : ShaderBuildStage::COMPUTED;
}

void Material::setShaderProgramInternal(const ResourceDescriptor& shaderDescriptor,
                                        const RenderStagePass& stagePass,
                                        const bool computeOnAdd) {
    OPTICK_EVENT();

    ShaderProgramInfo& info = shaderInfo(stagePass);
    // if we already have a different shader assigned ...
    if (info._shaderRef != nullptr && info._shaderRef->resourceName().compare(shaderDescriptor.resourceName()) != 0)
    {
        // We cannot replace a shader that is still loading in the background
        WAIT_FOR_CONDITION(info._shaderRef->getState() == ResourceState::RES_LOADED);
        Console::printfn(Locale::get(_ID("REPLACE_SHADER")),
            info._shaderRef->resourceName().c_str(),
            shaderDescriptor.resourceName().c_str(),
            TypeUtil::RenderStageToString(stagePass._stage),
            TypeUtil::RenderPassTypeToString(stagePass._passType));
    }

    ShaderComputeQueue::ShaderQueueElement shaderElement{ info._shaderRef, shaderDescriptor };
    if (computeOnAdd) {
        _context.shaderComputeQueue().process(shaderElement);
        info._shaderCompStage = ShaderBuildStage::COMPUTED;
    } else {
        _context.shaderComputeQueue().addToQueueBack(shaderElement);
        info._shaderCompStage = ShaderBuildStage::QUEUED;
    }
}

void Material::recomputeShaders() {
    OPTICK_EVENT();

    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            RenderStagePass stagePass{ static_cast<RenderStage>(s), static_cast<RenderPassType>(p) };
            ShaderPerVariant& variantMap = _shaderInfo[s][p];

            for (U8 v = 0; v < g_maxVariantsPerPass; ++v) {
                ShaderProgramInfo& shaderInfo = variantMap[v];
                if (!shaderInfo._customShader && shaderInfo._shaderCompStage != ShaderBuildStage::COUNT) {
                    stagePass._variant = v;
                    shaderInfo._shaderCompStage = ShaderBuildStage::REQUESTED;
                    computeShader(stagePass);
                }
            }
        }
    }
}

I64 Material::getProgramGUID(RenderStagePass renderStagePass) const {
    const ShaderProgramInfo& info = shaderInfo(renderStagePass);

    if (info._shaderRef != nullptr && info._shaderRef->getState() == ResourceState::RES_LOADED) {
        return info._shaderRef->getGUID();
    }

    return ShaderProgram::defaultShader()->getGUID();
}

bool Material::canDraw(RenderStagePass renderStagePass) {
    OPTICK_EVENT();

    ShaderProgramInfo& info = shaderInfo(renderStagePass);
    if (info._shaderCompStage == ShaderBuildStage::QUEUED && info._shaderRef != nullptr) {
        info._shaderCompStage = ShaderBuildStage::COMPUTED;
    }

    if (info._shaderCompStage == ShaderBuildStage::COMPUTED) {
        if (info._shaderRef != nullptr && info._shaderRef->getState() == ResourceState::RES_LOADED) {
            info._shaderCompStage = ShaderBuildStage::READY;
            return false;
        }
    }

    if (info._shaderCompStage != ShaderBuildStage::READY) {
        return computeShader(renderStagePass);
    }

    return true;
}

/// If the current material doesn't have a shader associated with it, then add the default ones.
bool Material::computeShader(RenderStagePass renderStagePass) {
    OPTICK_EVENT();

    const bool isDepthPass = renderStagePass.isDepthPass();
    const bool isShadowPass = renderStagePass._stage == RenderStage::SHADOW;

    ShaderProgramInfo& info = shaderInfo(renderStagePass);
    // If shader's invalid, try to request a recompute as it might fix it
    if (info._shaderCompStage == ShaderBuildStage::COUNT) {
        info._shaderCompStage = ShaderBuildStage::REQUESTED;
        return false;
    }

    // If the shader is valid and a recompute wasn't requested, just return true
    if (info._shaderCompStage != ShaderBuildStage::REQUESTED) {
        return info._shaderCompStage == ShaderBuildStage::READY;
    }

    // At this point, only computation requests are processed
    assert(info._shaderCompStage == ShaderBuildStage::REQUESTED);

    constexpr U32 slot0 = to_base(TextureUsage::UNIT0);
    constexpr U32 slot1 = to_base(TextureUsage::UNIT1);
    constexpr U32 slotOpacity = to_base(TextureUsage::OPACITY);

    DIVIDE_ASSERT(_properties._shadingMode != ShadingMode::COUNT, "Material computeShader error: Invalid shading mode specified!");


    ModuleDefines vertDefines = {};
    ModuleDefines fragDefines = {};
    ModuleDefines globalDefines = {};

    vertDefines.insert(std::cend(vertDefines),
                       std::cbegin(_extraShaderDefines[to_base(ShaderType::VERTEX)]),
                       std::cend(_extraShaderDefines[to_base(ShaderType::VERTEX)]));

    fragDefines.insert(std::cend(fragDefines),
                       std::cbegin(_extraShaderDefines[to_base(ShaderType::FRAGMENT)]),
                       std::cend(_extraShaderDefines[to_base(ShaderType::FRAGMENT)]));
    
    vertDefines.emplace_back("USE_CUSTOM_CLIP_PLANES", true);

    if (_textures[slot1]) {
        if (!_textures[slot0]) {
            std::swap(_textures[slot0], _textures[slot1]);
            updateTranslucency();
        }
    }
    const Str64 vertSource = isDepthPass ? baseShaderData()._depthShaderVertSource : baseShaderData()._colourShaderVertSource;
    const Str64 fragSource = isDepthPass ? baseShaderData()._depthShaderFragSource : baseShaderData()._colourShaderFragSource;

    Str32 vertVariant = isDepthPass 
                            ? isShadowPass 
                                ? baseShaderData()._shadowShaderVertVariant
                                : baseShaderData()._depthShaderVertVariant
                            : baseShaderData()._colourShaderVertVariant;
    Str32 fragVariant = isDepthPass ? baseShaderData()._depthShaderFragVariant : baseShaderData()._colourShaderFragVariant;
    Str128 shaderName = vertSource + "_" + fragSource;


    if (isDepthPass) {
        if (renderStagePass._stage == RenderStage::SHADOW) {
            shaderName += ".SHDW";
            vertVariant += "Shadow";
            fragVariant += "Shadow";
            if (renderStagePass._variant == to_U8(LightType::DIRECTIONAL)) {
                fragVariant += ".VSM";
            }
            globalDefines.emplace_back("SHADOW_PASS", true);
        } else {
            shaderName += ".PP";
            vertVariant += "PrePass";
            fragVariant += "PrePass";
            globalDefines.emplace_back("PRE_PASS", true);
        }
    }

    if (renderStagePass._passType == RenderPassType::OIT_PASS) {
        shaderName += ".OIT";
        fragDefines.emplace_back("OIT_PASS", true);
    }

    if (!_textures[slot0]) {
        fragDefines.emplace_back("SKIP_TEX0", true);
        shaderName += ".NTex0";
    }

    // Display pre-pass caches normal maps in a GBuffer, so it's the only exception
    if ((!isDepthPass || renderStagePass._stage == RenderStage::DISPLAY) &&
        _textures[to_base(TextureUsage::NORMALMAP)] != nullptr &&
        _properties._bumpMethod != BumpMethod::NONE) 
    {
        // Bump mapping?
        globalDefines.emplace_back("COMPUTE_TBN", true);
    }

    if (!isDepthPass && _textures[to_base(TextureUsage::SPECULAR)] != nullptr) {
        if (isPBRMaterial()) {
            shaderName += ".MRgh";
            fragDefines.emplace_back("USE_METALLIC_ROUGHNESS_MAP", true);
        } else {
            shaderName += ".SMap";
            fragDefines.emplace_back("USE_SPECULAR_MAP", true);
        }
    }

    updateTranslucency();

    if (_properties._translucencySource != TranslucencySource::COUNT && renderStagePass._passType != RenderPassType::OIT_PASS) {
        shaderName += ".AD";
        fragDefines.emplace_back("USE_ALPHA_DISCARD", true);
    }

    switch (_properties._translucencySource) {
        case TranslucencySource::OPACITY_MAP_A:
        case TranslucencySource::OPACITY_MAP_R: {
            fragDefines.emplace_back("USE_OPACITY_MAP", true);
            if (_properties._translucencySource == TranslucencySource::OPACITY_MAP_R) {
                shaderName += ".OMapR";
                fragDefines.emplace_back("USE_OPACITY_MAP_RED_CHANNEL", true);
            } else {
                shaderName += ".OMapA";
            }
        } break;
        case TranslucencySource::ALBEDO: {
            shaderName += ".AAlpha";
            fragDefines.emplace_back("USE_ALBEDO_ALPHA", true);
        } break;
        default: break;
    };

    if (isDoubleSided()) {
        shaderName += ".2Sided";
        fragDefines.emplace_back("USE_DOUBLE_SIDED", true);
    }

    if (!receivesShadows() || !_context.context().config().rendering.shadowMapping.enabled) {
        shaderName += ".NSHDW";
        fragDefines.emplace_back("DISABLE_SHADOW_MAPPING", true);
    }

    if (!_properties._isStatic) {
        shaderName += ".D";
        vertDefines.emplace_back("NODE_DYNAMIC", true);
        fragDefines.emplace_back("NODE_DYNAMIC", true);
    } else {
        vertDefines.emplace_back("NODE_STATIC", true);
        fragDefines.emplace_back("NODE_STATIC", true);
    }

    if (!isDepthPass) {
        switch (_properties._shadingMode) {
            default:
            case ShadingMode::FLAT: {
                fragDefines.emplace_back("USE_SHADING_FLAT", true);
                shaderName += ".Flat";
            } break;
            /*case ShadingMode::PHONG: {
                fragDefines.emplace_back("USE_SHADING_PHONG", true);
                shaderName += ".Phong";
            } break;*/
            case ShadingMode::PHONG:
            case ShadingMode::BLINN_PHONG: {
                fragDefines.emplace_back("USE_SHADING_BLINN_PHONG", true);
                shaderName += ".Blinn";
            } break;
            case ShadingMode::TOON: {
                fragDefines.emplace_back("USE_SHADING_TOON", true);
                shaderName += ".Toon";
            } break;
            case ShadingMode::OREN_NAYAR: {
                fragDefines.emplace_back("USE_SHADING_OREN_NAYAR", true);
                shaderName += ".OrenN";
            } break;
            case ShadingMode::COOK_TORRANCE: {
                fragDefines.emplace_back("USE_SHADING_COOK_TORRANCE", true);
                shaderName += ".CookT";
            } break;
        }
    }

    // Add the GPU skinning module to the vertex shader?
    if (_properties._hardwareSkinning) {
        vertDefines.emplace_back("USE_GPU_SKINNING", true);
        shaderName += ".Sknd";  //<Use "," instead of "." will add a Vertex only property
    }

    if (renderStagePass._stage == RenderStage::DISPLAY) {
        if (!isDepthPass) {
            fragDefines.emplace_back("USE_SSAO", true);
            shaderName += "SSAO";
        }
        fragDefines.emplace_back("USE_DEFERRED_NORMALS", true);
        shaderName += ".DNrmls";
    }

    globalDefines.emplace_back("DEFINE_PLACEHOLDER", false);

    vertDefines.insert(std::cend(vertDefines), std::cbegin(globalDefines), std::cend(globalDefines));
    fragDefines.insert(std::cend(fragDefines), std::cbegin(globalDefines), std::cend(globalDefines));

    if (!vertDefines.empty()) {
        shaderName.append(Util::StringFormat("_%zu", ShaderProgram::definesHash(vertDefines)).c_str());
    }

    if (!fragDefines.empty()) {
        shaderName.append(Util::StringFormat("_%zu", ShaderProgram::definesHash(fragDefines)).c_str());
    }

    ShaderModuleDescriptor vertModule = {};
    vertModule._variant = vertVariant;
    vertModule._sourceFile = vertSource + ".glsl";
    vertModule._batchSameFile = false;
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._defines = vertDefines;

    ShaderModuleDescriptor fragModule = {};
    fragModule._variant = fragVariant;
    fragModule._sourceFile = fragSource + ".glsl";
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._defines = fragDefines;

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);

    if(!Config::Lighting::USE_SEPARATE_VSM_PASS ||
        (renderStagePass._stage != RenderStage::SHADOW || hasTransparency()))
    {
        shaderDescriptor._modules.push_back(fragModule);
    }

    ResourceDescriptor shaderResDescriptor(shaderName);
    shaderResDescriptor.propertyDescriptor(shaderDescriptor);
    shaderResDescriptor.threaded(true);
    shaderResDescriptor.flag(true);

    setShaderProgramInternal(shaderResDescriptor, renderStagePass, false);

    return false;
}

bool Material::getTextureData(TextureUsage slot, TextureDataContainer<>& container) {
    const U8 slotValue = to_U8(slot);

    TextureData data = {};
    {
        SharedLock<SharedMutex> r_lock(_textureLock);
        const Texture_ptr& crtTexture = _textures[slotValue];
        if (crtTexture != nullptr) {
            data = crtTexture->data();
        }
    }

    if (data.type() != TextureType::COUNT) {
        return container.setTexture(data, slotValue) != TextureUpdateState::NOTHING;
    }

    return false;
}

bool Material::getTextureData(RenderStagePass renderStagePass, TextureDataContainer<>& textureData) {
    OPTICK_EVENT();

    if (textureData.empty()) {
        return getTextureDataFast(renderStagePass, textureData);
    }

    const bool isDepthPass = renderStagePass.isDepthPass();

    bool ret = false;
    if (!isDepthPass || hasTransparency() || _textureUseForDepth[to_base(TextureUsage::UNIT0)]) {
        ret = getTextureData(TextureUsage::UNIT0, textureData) || ret;
    }

    if (!isDepthPass || hasTransparency() || _textureUseForDepth[to_base(TextureUsage::OPACITY)]) {
        ret = getTextureData(TextureUsage::OPACITY, textureData) || ret;
    }

    if (!isDepthPass || _textureUseForDepth[to_base(TextureUsage::HEIGHTMAP)]) {
        ret = getTextureData(TextureUsage::HEIGHTMAP, textureData) || ret;
    }
    if (!isDepthPass || _textureUseForDepth[to_base(TextureUsage::PROJECTION)]) {
        ret = getTextureData(TextureUsage::PROJECTION, textureData) || ret;
    }

    if (!isDepthPass || _textureUseForDepth[to_base(TextureUsage::UNIT1)]) {
        ret = getTextureData(TextureUsage::UNIT1, textureData) || ret;
    }

    if (!isDepthPass || _textureUseForDepth[to_base(TextureUsage::SPECULAR)]) {
        ret = getTextureData(TextureUsage::SPECULAR, textureData) || ret;
    }

    if (renderStagePass._stage != RenderStage::SHADOW && // not shadow pass
       !(renderStagePass._stage == RenderStage::DISPLAY && renderStagePass._passType != RenderPassType::PRE_PASS)) //not Display::PrePass
    {
        ret = getTextureData(TextureUsage::NORMALMAP, textureData) || ret;
    }

    return ret;
}

bool Material::getTextureDataFast(RenderStagePass renderStagePass, TextureDataContainer<>& textureData) {
    OPTICK_EVENT();

    constexpr U8 transparentSlots[] = {
        to_base(TextureUsage::UNIT0),
        to_base(TextureUsage::OPACITY)
    };

    constexpr U8 extraSlots[] = {
        to_base(TextureUsage::UNIT1),
        to_base(TextureUsage::SPECULAR),
        to_base(TextureUsage::HEIGHTMAP),
        to_base(TextureUsage::PROJECTION)
    };

    bool ret = false;
    SharedLock<SharedMutex> r_lock(_textureLock);

    const bool depthStage = renderStagePass.isDepthPass();
    for (const U8 slot : transparentSlots) {
        if (!depthStage || hasTransparency() || _textureUseForDepth[slot]) {
            const Texture_ptr& crtTexture = _textures[slot];
            if (crtTexture != nullptr) {
                textureData.setTexture(crtTexture->data(), slot);
                ret = true;
            }
        }
    }

    for (const U8 slot : extraSlots) {
        if (!depthStage || _textureUseForDepth[slot]) {
            const Texture_ptr& crtTexture = _textures[slot];
            if (crtTexture != nullptr) {
                textureData.setTexture(crtTexture->data(), slot);
                ret = true;
            }
        }
    }

    if (renderStagePass._stage != RenderStage::SHADOW && // not shadow pass
        !(renderStagePass._stage == RenderStage::DISPLAY && renderStagePass._passType != RenderPassType::PRE_PASS)) //everything apart from Display::PrePass
    {
        constexpr U8 normalSlot = to_base(TextureUsage::NORMALMAP);
        const Texture_ptr& crtNormalTexture = _textures[normalSlot];
        if (crtNormalTexture != nullptr) {
            textureData.setTexture(crtNormalTexture->data(), normalSlot);
            ret = true;
        }
    }

    return ret;
}

bool Material::unload() noexcept {
    _textures.fill(nullptr);

    static ShaderProgramInfo defaultShaderInfo = {};

    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        ShaderVariantsPerPass& passMapShaders = _shaderInfo[s];
        StateVariantsPerPass& passMapStates = _defaultRenderStates[s];
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            passMapShaders[p].fill(defaultShaderInfo);
            passMapStates[p].fill(g_invalidStateHash);
        }
    }
    

    return true;
}

void Material::setReflective(const bool state) {
    if (_properties._isReflective == state) {
        return;
    }

    _properties._isReflective = state;
    _needsNewShader = true;
}

void Material::setRefractive(const bool state) {
    if (_properties._isRefractive == state) {
        return;
    }

    _properties._isRefractive = state;
    _needsNewShader = true;
}

void Material::setDoubleSided(const bool state) {
    if (_properties._doubleSided == state) {
        return;
    }

    _properties._doubleSided = state;
    _needsNewShader = true;
}

void Material::setReceivesShadows(const bool state) {
    if (_properties._receivesShadows == state) {
        return;
    }

    _properties._receivesShadows = state;
    _needsNewShader = true;
}

void Material::setStatic(const bool state) {
    if (_properties._isStatic == state) {
        return;
    }

    _properties._isStatic = state;
    _needsNewShader = true;
}

void Material::updateTranslucency() {
    if (_properties._translucencyDisabled) {
        return;
    }

    bool wasTranslucent = _properties._translucent;
    const TranslucencySource oldSource = _properties._translucencySource;
    _properties._translucencySource = TranslucencySource::COUNT;

    // In order of importance (less to more)!
    // diffuse channel alpha
    if (_properties._colourData.baseColour().a < 0.95f) {
        _properties._translucencySource = TranslucencySource::ALBEDO;
        _properties._translucent = true;
    }

    bool usingAlbedoTexAlpha = false;
    bool usingOpacityTexAlpha = false;
    // base texture is translucent
    const Texture_ptr& albedo = _textures[to_base(TextureUsage::UNIT0)];
    if (albedo && albedo->hasTransparency()) {
        _properties._translucencySource = TranslucencySource::ALBEDO;
        _properties._translucent = albedo->hasTranslucency();

        usingAlbedoTexAlpha = oldSource != _properties._translucencySource;
    }

    // opacity map
    const Texture_ptr& opacity = _textures[to_base(TextureUsage::OPACITY)];
    if (opacity && opacity->hasTransparency()) {
        const U8 channelCount = opacity->descriptor().numChannels();
        DIVIDE_ASSERT(channelCount == 1 || channelCount == 4, "Material::updateTranslucency: Opacity textures must be either single-channel or RGBA!");

        _properties._translucencySource = channelCount == 4 ? TranslucencySource::OPACITY_MAP_A : TranslucencySource::OPACITY_MAP_R;
        _properties._translucent = opacity->hasTranslucency();
        usingAlbedoTexAlpha = false;
        usingOpacityTexAlpha = oldSource != _properties._translucencySource || wasTranslucent != _properties._translucent;
    }

    if (usingAlbedoTexAlpha || usingOpacityTexAlpha) {
        Texture* tex = (usingOpacityTexAlpha ? opacity.get() : albedo.get());

        const U16 baseLevel = tex->getBaseMipLevel();
        const U16 maxLevel = tex->getMaxMipLevel();
        const U16 baseOffset = maxLevel > g_numMipsToKeepFromAlphaTextures ? g_numMipsToKeepFromAlphaTextures : maxLevel;
        STUBBED("HACK! Limit mip range for textures that have alpha values used by the material -Ionut");
        if (tex->getMipCount() == maxLevel) {
            tex->setMipMapRange(baseLevel, baseOffset);
        }
    }

    if (oldSource != _properties._translucencySource || wasTranslucent != _properties._translucent) {
        _needsNewShader = true;
    }
}

size_t Material::getRenderStateBlock(RenderStagePass renderStagePass) {
    size_t ret = _context.getDefaultStateBlock(false);

    const StatesPerVariant& variantMap = _defaultRenderStates[to_base(renderStagePass._stage)][to_base(renderStagePass._passType)];

    assert(renderStagePass._variant < g_maxVariantsPerPass);

    ret = variantMap[renderStagePass._variant];
    // If we haven't defined a state for this variant, use the default one
    if (ret == g_invalidStateHash) {
        ret = variantMap[0u];
    }

    if (_properties._doubleSided && renderStagePass._stage != RenderStage::SHADOW) {
        RenderStateBlock tempBlock = RenderStateBlock::get(ret);
        tempBlock.setCullMode(CullMode::NONE);
        ret = tempBlock.getHash();
    }

    return ret;
}

void Material::getSortKeys(RenderStagePass renderStagePass, I64& shaderKey, I32& textureKey) const {
    textureKey = _textureKeyCache == -1 ? std::numeric_limits<I32>::lowest() : _textureKeyCache;

    const ShaderProgramInfo& info = shaderInfo(renderStagePass);
    if (info._shaderCompStage == ShaderBuildStage::READY) {
        shaderKey = info._shaderRef->getGUID();
    } else {
        shaderKey = std::numeric_limits<I64>::lowest();
    }
}

void Material::getMaterialMatrix(mat4<F32>& retMatrix) const {
    retMatrix.setRow(0, _properties._colourData._data[0]);
    retMatrix.setRow(1, _properties._colourData._data[1]);
    retMatrix.setRow(2, _properties._colourData._data[2]);
    retMatrix.setRow(3, vec4<F32>(0.0f, getParallaxFactor(), 0.0f, 0.0f));
}

void Material::rebuild() {
    recomputeShaders();

    // Alternatively we could just copy the maps directly
    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            ShaderPerVariant& shaders = _shaderInfo[s][p];
            for (ShaderProgramInfo& info : shaders) {
                if (info._shaderRef != nullptr && info._shaderRef->getState() == ResourceState::RES_LOADED) {
                    info._shaderRef->recompile(true);
                }
            }
        }
    }
}

const char* getTexUsageName(TextureUsage texUsage) noexcept {
    switch (texUsage) {
        case TextureUsage::UNIT0      : return "UNIT0";
        case TextureUsage::NORMALMAP: return "NORMALMAP";
        case TextureUsage::HEIGHTMAP  : return "HEIGHT";
        case TextureUsage::OPACITY: return "OPACITY";
        case TextureUsage::SPECULAR: return "SPECULAR";
        case TextureUsage::UNIT1      : return "UNIT1";
        case TextureUsage::PROJECTION : return "PROJECTION";
    };

    return "";
}

TextureUsage getTexUsageByName(const stringImpl& name) {
    if (Util::CompareIgnoreCase(name, "UNIT0")) {
        return TextureUsage::UNIT0;
    } else if (Util::CompareIgnoreCase(name, "NORMALMAP")) {
        return TextureUsage::NORMALMAP;
    } else if (Util::CompareIgnoreCase(name, "HEIGHT")) {
        return TextureUsage::HEIGHTMAP;
    } else if (Util::CompareIgnoreCase(name, "OPACITY")) {
        return TextureUsage::OPACITY;
    } else if (Util::CompareIgnoreCase(name, "SPECULAR")) {
        return TextureUsage::SPECULAR;
    } else if (Util::CompareIgnoreCase(name, "UNIT1")) {
        return TextureUsage::UNIT1;
    } else if (Util::CompareIgnoreCase(name, "PROJECTION")) {
        return TextureUsage::PROJECTION;
    }

    return TextureUsage::COUNT;
}

const char *getBumpMethodName(BumpMethod bumpMethod) noexcept {
    switch(bumpMethod) {
        case BumpMethod::NORMAL   : return "NORMAL";
        case BumpMethod::PARALLAX : return "PARALLAX";
        case BumpMethod::PARALLAX_OCCLUSION: return "PARALLAX_OCCLUSION";

        default:break;
    }

    return "NONE";
}

BumpMethod getBumpMethodByName(const stringImpl& name) {
    if (Util::CompareIgnoreCase(name, "NORMAL")) {
        return BumpMethod::NORMAL;
    } else if (Util::CompareIgnoreCase(name, "PARALLAX")) {
        return BumpMethod::PARALLAX;
    } else if (Util::CompareIgnoreCase(name, "PARALLAX_OCCLUSION")) {
        return BumpMethod::PARALLAX_OCCLUSION;
    }

    return BumpMethod::COUNT;
}

const char *getShadingModeName(ShadingMode shadingMode) noexcept {
    switch (shadingMode) {
        case ShadingMode::FLAT          : return "FLAT";
        case ShadingMode::PHONG         : return "PHONG";
        case ShadingMode::BLINN_PHONG   : return "BLINN_PHONG";
        case ShadingMode::TOON          : return "TOON";
        case ShadingMode::OREN_NAYAR    : return "OREN_NAYAR";
        case ShadingMode::COOK_TORRANCE : return "COOK_TORRANCE";
        default:break;
    }

    return "NONE";
}

ShadingMode getShadingModeByName(const stringImpl& name) {
    if (Util::CompareIgnoreCase(name, "FLAT")) {
        return ShadingMode::FLAT;
    } else if (Util::CompareIgnoreCase(name, "PHONG")) {
        return ShadingMode::PHONG;
    } else if (Util::CompareIgnoreCase(name, "BLINN_PHONG")) {
        return ShadingMode::BLINN_PHONG;
    } else if (Util::CompareIgnoreCase(name, "TOON")) {
            return ShadingMode::TOON;
    } else if (Util::CompareIgnoreCase(name, "OREN_NAYAR")) {
            return ShadingMode::OREN_NAYAR;
    } else if (Util::CompareIgnoreCase(name, "COOK_TORRANCE")) {
            return ShadingMode::COOK_TORRANCE;
    }

    return ShadingMode::COUNT;
}

TextureOperation getTextureOperationByName(const stringImpl& operation) {
    if (Util::CompareIgnoreCase(operation, "TEX_OP_MULTIPLY")) {
        return TextureOperation::MULTIPLY;
    } else if (Util::CompareIgnoreCase(operation, "TEX_OP_DECAL")) {
        return TextureOperation::DECAL;
    } else if (Util::CompareIgnoreCase(operation, "TEX_OP_ADD")) {
        return TextureOperation::ADD;
    } else if (Util::CompareIgnoreCase(operation, "TEX_OP_SMOOTH_ADD")) {
        return TextureOperation::SMOOTH_ADD;
    } else if (Util::CompareIgnoreCase(operation, "TEX_OP_SIGNED_ADD")) {
        return TextureOperation::SIGNED_ADD;
    } else if (Util::CompareIgnoreCase(operation, "TEX_OP_DIVIDE")) {
        return TextureOperation::DIVIDE;
    } else if (Util::CompareIgnoreCase(operation, "TEX_OP_SUBTRACT")) {
        return TextureOperation::SUBTRACT;
    }

    return TextureOperation::REPLACE;
}

const char *getTextureOperationName(TextureOperation textureOp) noexcept {
    switch(textureOp) {
        case TextureOperation::MULTIPLY   : return "TEX_OP_MULTIPLY";
        case TextureOperation::DECAL      : return "TEX_OP_DECAL";
        case TextureOperation::ADD        : return "TEX_OP_ADD";
        case TextureOperation::SMOOTH_ADD : return "TEX_OP_SMOOTH_ADD";
        case TextureOperation::SIGNED_ADD : return "TEX_OP_SIGNED_ADD";
        case TextureOperation::DIVIDE     : return "TEX_OP_DIVIDE";
        case TextureOperation::SUBTRACT   : return "TEX_OP_SUBTRACT";
    }

    return "TEX_OP_REPLACE";
}

const char *getWrapModeName(TextureWrap wrapMode) noexcept {
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

const char *getFilterName(TextureFilter filter) noexcept {
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

Texture_ptr loadTextureXML(ResourceCache* targetCache,
                            const stringImpl &textureNode,
                            const stringImpl &textureName,
                            const boost::property_tree::ptree& pt)
{
    const Str64 img_name(textureName.substr(textureName.find_last_of('/') + 1).c_str());
    const Str256 pathName(textureName.substr(0, textureName.rfind("/")).c_str());

    SamplerDescriptor sampDesc = {};

    sampDesc.wrapU(getWrapModeByName(pt.get<stringImpl>(textureNode + ".Map.<xmlattr>.U", "REPEAT")));
    sampDesc.wrapV(getWrapModeByName(pt.get<stringImpl>(textureNode + ".Map.<xmlattr>.V", "REPEAT")));
    sampDesc.wrapW(getWrapModeByName(pt.get<stringImpl>(textureNode + ".Map.<xmlattr>.W", "REPEAT")));
    sampDesc.minFilter(getFilterByName(pt.get<stringImpl>(textureNode + ".Filter.<xmlattr>.min", "LINEAR")));
    sampDesc.magFilter(getFilterByName(pt.get<stringImpl>(textureNode + ".Filter.<xmlattr>.mag", "LINEAR")));
    sampDesc.anisotropyLevel(to_U8(pt.get(textureNode + ".anisotropy", 0U)));

    TextureDescriptor texDesc(TextureType::TEXTURE_2D);
    texDesc.samplerDescriptor(sampDesc);

    ResourceDescriptor texture(img_name);
    texture.assetName(img_name);
    texture.assetLocation(pathName);
    texture.propertyDescriptor(texDesc);
    texture.waitForReady(false);
    texture.flag(!pt.get(textureNode + ".flipped", false));

    return CreateResource<Texture>(targetCache, texture);
}

void Material::saveToXML(const stringImpl& entryName, boost::property_tree::ptree& pt) const {
    pt.put(entryName + ".shadingMode", getShadingModeName(getShadingMode()));

    pt.put(entryName + ".colour.<xmlattr>.r", getColourData().baseColour().r);
    pt.put(entryName + ".colour.<xmlattr>.g", getColourData().baseColour().g);
    pt.put(entryName + ".colour.<xmlattr>.b", getColourData().baseColour().b);
    pt.put(entryName + ".colour.<xmlattr>.a", getColourData().baseColour().a);

    pt.put(entryName + ".emissive.<xmlattr>.r", getColourData().emissive().r);
    pt.put(entryName + ".emissive.<xmlattr>.g", getColourData().emissive().g);
    pt.put(entryName + ".emissive.<xmlattr>.b", getColourData().emissive().b);

    if (!isPBRMaterial())
    {
        pt.put(entryName + ".specular.<xmlattr>.r", getColourData().specular().r);
        pt.put(entryName + ".specular.<xmlattr>.g", getColourData().specular().g);
        pt.put(entryName + ".specular.<xmlattr>.b", getColourData().specular().b);

        pt.put(entryName + ".shininess", getColourData().shininess());
    } else {
        pt.put(entryName + ".metallic", getColourData().metallic());
        pt.put(entryName + ".reflectivity", getColourData().reflectivity());
        pt.put(entryName + ".roughness", getColourData().roughness());
    }

    pt.put(entryName + ".doubleSided", isDoubleSided());

    pt.put(entryName + ".receivesShadows", receivesShadows());

    pt.put(entryName + ".bumpMethod", getBumpMethodName(getBumpMethod()));

    pt.put(entryName + ".parallaxFactor", getParallaxFactor());


    for (U8 i = 0; i < g_materialTexturesCount; ++i) {
        TextureUsage usage = g_materialTextures[i];

        Texture_wptr tex = getTexture(usage);
        if (!tex.expired()) {
            Texture_ptr texture = tex.lock();

            const SamplerDescriptor &sampler = texture->getCurrentSampler();

            stringImpl textureNode = entryName + ".texture.";
            textureNode += getTexUsageName(usage);

            pt.put(textureNode + ".name", texture->assetName().c_str());
            pt.put(textureNode + ".path", texture->assetLocation());
            pt.put(textureNode + ".flipped", texture->flipped());

            if (usage == TextureUsage::UNIT1) {
                pt.put(textureNode + ".usage", getTextureOperationName(_properties._operation));
            }
            pt.put(textureNode + ".Map.<xmlattr>.U", getWrapModeName(sampler.wrapU()));
            pt.put(textureNode + ".Map.<xmlattr>.V", getWrapModeName(sampler.wrapV()));
            pt.put(textureNode + ".Map.<xmlattr>.W", getWrapModeName(sampler.wrapW()));
            pt.put(textureNode + ".Filter.<xmlattr>.min", getFilterName(sampler.minFilter()));
            pt.put(textureNode + ".Filter.<xmlattr>.mag", getFilterName(sampler.magFilter()));
            pt.put(textureNode + ".anisotropy", to_U32(sampler.anisotropyLevel()));
        }
    }
}

void Material::loadFromXML(const stringImpl& entryName, const boost::property_tree::ptree& pt) {
    if (ignoreXMLData()) {
        return;
    }
    setShadingMode(getShadingModeByName(pt.get<stringImpl>(entryName + ".shadingMode", "FLAT")));

    _properties._colourData.baseColour(
        FColour4(pt.get<F32>(entryName + ".colour.<xmlattr>.r", 0.6f),
                 pt.get<F32>(entryName + ".colour.<xmlattr>.g", 0.6f),
                 pt.get<F32>(entryName + ".colour.<xmlattr>.b", 0.6f),
                 pt.get<F32>(entryName + ".colour.<xmlattr>.a", 1.f))
    );

    _properties._colourData.emissive(
        FColour3(pt.get<F32>(entryName + ".emissive.<xmlattr>.r", 0.f),
                 pt.get<F32>(entryName + ".emissive.<xmlattr>.g", 0.f),
                 pt.get<F32>(entryName + ".emissive.<xmlattr>.b", 0.f))
    );

    if (!isPBRMaterial()) {
        _properties._colourData.specular(
            FColour3(pt.get<F32>(entryName + ".specular.<xmlattr>.r", 1.f),
                     pt.get<F32>(entryName + ".specular.<xmlattr>.g", 1.f),
                     pt.get<F32>(entryName + ".specular.<xmlattr>.b", 1.f))
        );

        _properties._colourData.shininess(pt.get<F32>(entryName + ".shininess", 0.0f));
    } else {
        _properties._colourData.metallic(pt.get<F32>(entryName + ".metallic", 0.0f));
        _properties._colourData.reflectivity(pt.get<F32>(entryName + ".reflectivity", 0.0f));
        _properties._colourData.roughness(pt.get<F32>(entryName + ".roughness", 0.0f));
    }

    setDoubleSided(pt.get<bool>(entryName + ".doubleSided", false));

    setReceivesShadows(pt.get<bool>(entryName + ".receivesShadows", true));

    setBumpMethod(getBumpMethodByName(pt.get<stringImpl>(entryName + ".bumpMethod", "NORMAL")));

    setParallaxFactor(pt.get<F32>(entryName + ".parallaxFactor", 1.0f));


    STUBBED("ToDo: Set texture is currently disabled!");

    for (U8 i = 0; i < g_materialTexturesCount; ++i) {
        const TextureUsage usage = g_materialTextures[i];

        if (auto child = pt.get_child_optional(((entryName + ".texture.") + getTexUsageName(usage)) + ".name")) {
            stringImpl textureNode = entryName + ".texture.";
            textureNode += getTexUsageName(usage);

            stringImpl texName = pt.get<stringImpl>(textureNode + ".name", "");
            if (!texName.empty()) {
                stringImpl texLocation = pt.get<stringImpl>(textureNode + ".path", "");
                Texture_ptr tex = loadTextureXML(_context.parent().resourceCache(), textureNode, texLocation + "/" + texName, pt);
                if (tex != nullptr) {
                    if (usage == TextureUsage::UNIT1) {
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